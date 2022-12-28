#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


static char *
ngx_http_mytest(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

static ngx_int_t ngx_http_mytest_handler(ngx_http_request_t *r);



static ngx_command_t  ngx_http_mytest_commands[] =
{

    {
        ngx_string("mytest"),
        NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_HTTP_LMT_CONF | NGX_CONF_NOARGS,
        ngx_http_mytest,
        NGX_HTTP_LOC_CONF_OFFSET,
        0,
        NULL
    },

    ngx_null_command
};

static ngx_http_module_t  ngx_http_mytest_module_ctx =
{
    NULL,                              /* preconfiguration */
    NULL,                  		/* postconfiguration */

    NULL,                              /* create main configuration */
    NULL,                              /* init main configuration */

    NULL,                              /* create server configuration */
    NULL,                              /* merge server configuration */

    NULL,       			/* create location configuration */
    NULL         			/* merge location configuration */
};

ngx_module_t  ngx_http_mytest_module =
{
    NGX_MODULE_V1,
    &ngx_http_mytest_module_ctx,           /* module context */
    ngx_http_mytest_commands,              /* module directives */
    NGX_HTTP_MODULE,                       /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    NULL,                                  /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};


static char *
ngx_http_mytest(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_core_loc_conf_t  *clcf;

    /*
     * 首先找到 mytest 配置项所属的配置块
     * clcf 看上去是 location 块的数据结构，其实还可以是 main、srv、loc 级别的配置项
     * 因为在每个 http{} 和 server{} 内也都有一个 ngx_http_core_loc_conf_t 结构体
     */
    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);

    /*
     * HTTP 框架处理用户请求在进行到 NGX_HTTP_FIND_CONFIG_PHASE 阶段时，
     * 如果请求的主机域名、URI 与 mytest 配置项所在的配置块相匹配，
     * 就将调用 ngx_http_mytest_handler 处理这个请求
     */
    clcf->handler = ngx_http_mytest_handler;

    return NGX_CONF_OK;
}


static ngx_int_t ngx_http_mytest_handler(ngx_http_request_t *r)
{
    //������GET����HEAD���������򷵻�405 Not Allowed
    if (!(r->method & (NGX_HTTP_GET | NGX_HTTP_HEAD)))
    {
        return NGX_HTTP_NOT_ALLOWED;
    }

    //���������еİ���
    ngx_int_t rc = ngx_http_discard_request_body(r);
    if (rc != NGX_OK)
    {
        return rc;
    }

    //���÷��ص�Content-Type��ע�⣬ngx_str_t��һ���ܷ���ĳ�ʼ����
//ngx_string�������԰�ngx_str_t��data��len��Ա�����ú�
    ngx_str_t type = ngx_string("text/plain");
    //���صİ�������
    ngx_str_t response = ngx_string("Hello World!");
    //���÷���״̬��
    r->headers_out.status = NGX_HTTP_OK;
    //��Ӧ�����а������ݵģ�������Ҫ����Content-Length����
    r->headers_out.content_length_n = response.len;
    //����Content-Type
    r->headers_out.content_type = type;

    //����httpͷ��
    rc = ngx_http_send_header(r);
    if (rc == NGX_ERROR || rc > NGX_OK || r->header_only)
    {
        return rc;
    }

    //����ngx_buf_t�ṹ׼�����Ͱ���
    ngx_buf_t                 *b;
    b = ngx_create_temp_buf(r->pool, response.len);
    if (b == NULL)
    {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }
    //��Hello World������ngx_buf_tָ����ڴ���
    ngx_memcpy(b->pos, response.data, response.len);
    //ע�⣬һ��Ҫ���ú�lastָ��
    b->last = b->pos + response.len;
    //�����������һ�黺����
    b->last_buf = 1;

    //���췢��ʱ��ngx_chain_t�ṹ��
    ngx_chain_t		out;
    //��ֵngx_buf_t
    out.buf = b;
    //����nextΪNULL
    out.next = NULL;

    //���һ�����Ͱ��壬http��ܻ����ngx_http_finalize_request����
//��������
    return ngx_http_output_filter(r, &out);
}


