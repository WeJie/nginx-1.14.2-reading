
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_BUF_H_INCLUDED_
#define _NGX_BUF_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


typedef void *            ngx_buf_tag_t;

typedef struct ngx_buf_s  ngx_buf_t;

/*
 * 缓冲区 ngx_buf_s 是 Nginx 中处理大数据的数据结构
 * 这个数据结构既应用于内存数据也应用与磁盘数据
 */
struct ngx_buf_s {
    /* post 与 last 之间，表示的是 Nginx 这次要处理的内存数据 */
    u_char          *pos;           /* 通常表示将要处理的内存的位置 */
    u_char          *last;          /* 通常表示将要处理的内存的截止位置 */

    /* file_post 与 file_last 之间，表示的是 Nginx 这次要处理的文件数据 */
    off_t            file_pos;      /* 通常表示将要处理的文件的位置 */
    off_t            file_last;     /* 通常表示将要处理的文件的截止位置 */

    u_char          *start;         /* start of buffer */
    u_char          *end;           /* end of buffer */

    /* 表示当前缓冲区的类型，由哪个模块使用就是指向这个模块的 ngx_module_t 变量的地址 */
    ngx_buf_tag_t    tag;
    ngx_file_t      *file;          /* 引用的文件 */

    /* 缓冲区的影子缓冲区，该成员很少使用，使用方式较为复杂，要谨慎使用 */
    ngx_buf_t       *shadow;


    /* the buf's content could be changed */
    unsigned         temporary:1;

    /*
     * the buf's content is in a memory cache or in a read only memory
     * and must not be changed
     */
    unsigned         memory:1;

    /* the buf's content is mmap()ed and must not be changed */
    unsigned         mmap:1;

    /* 标志位，为 1 时表示可回收 */
    unsigned         recycled:1;
    /* 标志位，为 1 时表示这段缓冲区处理的是文件不是内存 */
    unsigned         in_file:1;
    /* 标志位，为 1 时表示需要执行 flush 操作 */
    unsigned         flush:1;
    /*
     * 标志位，对这个缓冲区的处理是否要使用同步的方式
     * 同步方式有可能阻塞 Nginx，异步处理是 Nginx 能做到高并发的关键，需要谨慎使用这个标志
     */
    unsigned         sync:1;
    /*
     * 标志位，是否为最后一块缓冲区
     * ngx_buf_t 可以用 ngx_chain_t 链表串起来
     * 当为 1 时，表示当前时最后一块待处理的缓冲区
     */
    unsigned         last_buf:1;
    /* 标志位，为 1 时表示 ngx_chain_t 中最后一块缓冲区 */
    unsigned         last_in_chain:1;

    /* 标志位，是否最后一个影子缓存块，和 shadow 配合使用 */
    unsigned         last_shadow:1;
    /* 标志位，当前缓冲区是后临时文件 */
    unsigned         temp_file:1;

    /* STUB */ int   num;
};


struct ngx_chain_s {
    ngx_buf_t    *buf;       /* 指向当前缓冲区 */
    ngx_chain_t  *next;      /* 指向下一个 ngx_chain_t */
};


typedef struct {
    ngx_int_t    num;
    size_t       size;
} ngx_bufs_t;


typedef struct ngx_output_chain_ctx_s  ngx_output_chain_ctx_t;

typedef ngx_int_t (*ngx_output_chain_filter_pt)(void *ctx, ngx_chain_t *in);

typedef void (*ngx_output_chain_aio_pt)(ngx_output_chain_ctx_t *ctx,
    ngx_file_t *file);

struct ngx_output_chain_ctx_s {
    ngx_buf_t                   *buf;
    ngx_chain_t                 *in;
    ngx_chain_t                 *free;
    ngx_chain_t                 *busy;

    unsigned                     sendfile:1;
    unsigned                     directio:1;
    unsigned                     unaligned:1;
    unsigned                     need_in_memory:1;
    unsigned                     need_in_temp:1;
    unsigned                     aio:1;

#if (NGX_HAVE_FILE_AIO || NGX_COMPAT)
    ngx_output_chain_aio_pt      aio_handler;
#if (NGX_HAVE_AIO_SENDFILE || NGX_COMPAT)
    ssize_t                    (*aio_preload)(ngx_buf_t *file);
#endif
#endif

#if (NGX_THREADS || NGX_COMPAT)
    ngx_int_t                  (*thread_handler)(ngx_thread_task_t *task,
                                                 ngx_file_t *file);
    ngx_thread_task_t           *thread_task;
#endif

    off_t                        alignment;

    ngx_pool_t                  *pool;
    ngx_int_t                    allocated;
    ngx_bufs_t                   bufs;
    ngx_buf_tag_t                tag;

    ngx_output_chain_filter_pt   output_filter;
    void                        *filter_ctx;
};


typedef struct {
    ngx_chain_t                 *out;
    ngx_chain_t                **last;
    ngx_connection_t            *connection;
    ngx_pool_t                  *pool;
    off_t                        limit;
} ngx_chain_writer_ctx_t;


#define NGX_CHAIN_ERROR     (ngx_chain_t *) NGX_ERROR


#define ngx_buf_in_memory(b)        (b->temporary || b->memory || b->mmap)
#define ngx_buf_in_memory_only(b)   (ngx_buf_in_memory(b) && !b->in_file)

#define ngx_buf_special(b)                                                   \
    ((b->flush || b->last_buf || b->sync)                                    \
     && !ngx_buf_in_memory(b) && !b->in_file)

#define ngx_buf_sync_only(b)                                                 \
    (b->sync                                                                 \
     && !ngx_buf_in_memory(b) && !b->in_file && !b->flush && !b->last_buf)

#define ngx_buf_size(b)                                                      \
    (ngx_buf_in_memory(b) ? (off_t) (b->last - b->pos):                      \
                            (b->file_last - b->file_pos))

ngx_buf_t *ngx_create_temp_buf(ngx_pool_t *pool, size_t size);
ngx_chain_t *ngx_create_chain_of_bufs(ngx_pool_t *pool, ngx_bufs_t *bufs);


#define ngx_alloc_buf(pool)  ngx_palloc(pool, sizeof(ngx_buf_t))
#define ngx_calloc_buf(pool) ngx_pcalloc(pool, sizeof(ngx_buf_t))

ngx_chain_t *ngx_alloc_chain_link(ngx_pool_t *pool);
#define ngx_free_chain(pool, cl)                                             \
    cl->next = pool->chain;                                                  \
    pool->chain = cl



ngx_int_t ngx_output_chain(ngx_output_chain_ctx_t *ctx, ngx_chain_t *in);
ngx_int_t ngx_chain_writer(void *ctx, ngx_chain_t *in);

ngx_int_t ngx_chain_add_copy(ngx_pool_t *pool, ngx_chain_t **chain,
    ngx_chain_t *in);
ngx_chain_t *ngx_chain_get_free_buf(ngx_pool_t *p, ngx_chain_t **free);
void ngx_chain_update_chains(ngx_pool_t *p, ngx_chain_t **free,
    ngx_chain_t **busy, ngx_chain_t **out, ngx_buf_tag_t tag);

off_t ngx_chain_coalesce_file(ngx_chain_t **in, off_t limit);

ngx_chain_t *ngx_chain_update_sent(ngx_chain_t *in, off_t sent);

#endif /* _NGX_BUF_H_INCLUDED_ */
