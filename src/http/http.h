#ifndef HTTP_H
#define HTTP

#include "../core/core.h"

class Request;

int initListen(Cycle *cycle, int port);
Connection *addListen(Cycle *cycle, int port);
int finalizeConnection(Connection *c);
int readRequestHeader(Request *r);

// event handler
int newConnection(Event *ev);
int waitRequest(Event *ev);
int processRequestLine(Event *ev);

enum class State
{
    sw_start = 0,
    sw_method,
    sw_spaces_before_uri,
    sw_schema,
    sw_schema_slash,
    sw_schema_slash_slash,
    sw_host_start,
    sw_host,
    sw_host_end,
    sw_host_ip_literal,
    sw_port,
    sw_after_slash_in_uri,
    sw_check_uri,
    sw_uri,
    sw_http_09,
    sw_http_H,
    sw_http_HT,
    sw_http_HTT,
    sw_http_HTTP,
    sw_first_major_digit,
    sw_major_digit,
    sw_first_minor_digit,
    sw_minor_digit,
    sw_spaces_after_digit,
    sw_almost_done
};

enum class Method
{
    GET,
    POST,
    PUT,
    DELETE,
    COPY,
    MOVE,
    LOCK,
    HEAD,
    MKCOL,
    PATCH,
    TRACE,
    UNLOCK,
    OPTIONS,
    CONNECT,
    PROPFIND,
    PROPPATCH
};

class Request
{
  public:
    Connection *c;
    int idx_ = -1;

    Method method;
    uintptr_t http_version;

    str_t http_protocol;
    str_t request_line;
    str_t method_name;
    str_t schema;
    str_t host;

    off_t request_length;

    /* URI with "/." and on Win32 with "//" */
    unsigned complex_uri : 1;
    /* URI with "%" */
    unsigned quoted_uri : 1;
    /* URI with "+" */
    unsigned plus_in_uri : 1;
    /* URI with empty path */
    unsigned empty_path_in_uri : 1;

    // http headers
    u_char *pos;
    State state = State::sw_start;
    uintptr_t header_hash;
    uintptr_t lowcase_index;
    u_char lowcase_header[32];

    u_char *header_name_start;
    u_char *header_name_end;
    u_char *header_start;
    u_char *header_end;

    /*
     * a memory that can be reused after parsing a request line
     * via ngx_http_ephemeral_t
     */

    u_char *uri_start;
    u_char *uri_end;
    u_char *uri_ext;
    u_char *args_start;
    u_char *request_start;
    u_char *request_end;
    u_char *method_end;
    u_char *schema_start;
    u_char *schema_end;
    u_char *host_start;
    u_char *host_end;
    u_char *port_start;
    u_char *port_end;

    unsigned http_minor : 16;
    unsigned http_major : 16;
};

class RequestPool
{
  private:
    Request **rPool_;
    uint32_t flags;

  public:
    const static int POOLSIZE = 32;
    RequestPool();
    ~RequestPool();
    Request *getNewRequest();
    void recoverRequest(Request *c);
};

#endif