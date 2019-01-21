
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_LIST_H_INCLUDED_
#define _NGX_LIST_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


typedef struct ngx_list_part_s  ngx_list_part_t;

struct ngx_list_part_s {
    /* 数组起始元素的地址 */
    void             *elts;
    /*
     * 数组中元素的数量
     * nelts 必须小于 ngx_list_t 中的 nalloc
     */
    ngx_uint_t        nelts;
    /* 链表下一个元素的地址 */
    ngx_list_part_t  *next;
};


/*
 * Nginx 封装的链表容器
 * 链表的元素是数组
 * HTTP 的头部就是用 ngx_list_t 来存储的
 */
typedef struct {
    /* 指向链表的最后一个数组元素 */
    ngx_list_part_t  *last;
    /* 链表的首个数组元素 */
    ngx_list_part_t   part;
    /* ngx_list_part_t 中每个数组元素占用空间的大小 */
    size_t            size;
    /* 每个 ngx_list_part_t 数组的容量 */
    ngx_uint_t        nalloc;
    /* 链表汇总管理内存分配的内存池对象。用户要存放的数据的内存都是由 pool 分配 */
    ngx_pool_t       *pool;
} ngx_list_t;


ngx_list_t *ngx_list_create(ngx_pool_t *pool, ngx_uint_t n, size_t size);

static ngx_inline ngx_int_t
ngx_list_init(ngx_list_t *list, ngx_pool_t *pool, ngx_uint_t n, size_t size)
{
    list->part.elts = ngx_palloc(pool, n * size);
    if (list->part.elts == NULL) {
        return NGX_ERROR;
    }

    list->part.nelts = 0;
    list->part.next = NULL;
    list->last = &list->part;
    list->size = size;
    list->nalloc = n;
    list->pool = pool;

    return NGX_OK;
}


/*
 *
 *  the iteration through the list:
 *
 *  part = &list.part;
 *  data = part->elts;
 *
 *  for (i = 0 ;; i++) {
 *
 *      if (i >= part->nelts) {
 *          if (part->next == NULL) {
 *              break;
 *          }
 *
 *          part = part->next;
 *          data = part->elts;
 *          i = 0;
 *      }
 *
 *      ...  data[i] ...
 *
 *  }
 */


void *ngx_list_push(ngx_list_t *list);


#endif /* _NGX_LIST_H_INCLUDED_ */
