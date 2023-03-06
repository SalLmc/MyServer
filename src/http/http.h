#ifndef HTTP_H
#define HTTP_H

#include "../core/core.h"
#include <list>
#include <unordered_map>

class Request;

int initListen(Cycle *cycle, int port);
Connection *addListen(Cycle *cycle, int port);
int keepAliveRequest(Request *r);
int finalizeConnection(Connection *c);
int finalizeRequest(Request *r);
int readRequestHeader(Request *r);
int processRequestHeader(Request *r, int need_host);
int processRequest(Request *r);
int readRequestBody(Request *r, std::function<int(Request *)> post_handler);
int processRequestBody(Request *r);
int requestBodyLength(Request *r);
int requestBodyChunked(Request *r);
int processStatusLine(Request *upsr);
int processHeaders(Request *upsr);
int processBody(Request *upsr);

// event handler
int newConnection(Event *ev);
int waitRequest(Event *ev);
int keepAlive(Event *ev);
int processRequestLine(Event *ev);
int processRequestHeaders(Event *ev);
int blockReading(Event *ev);
int blockWriting(Event *ev);
int runPhases(Event *ev);
int writeResponse(Event *ev);
int readRequestBodyInner(Event *ev);
int sendfileEvent(Event *ev);

#define PARSE_HEADER_DONE 1

enum class HeaderState
{
    sw_start = 0,
    sw_name,
    sw_space_before_value,
    sw_value,
    sw_space_after_value,
    sw_ignore_line,
    sw_almost_done,
    sw_header_almost_done
};

enum class RequestState
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

enum class HttpState
{
    INITING_REQUEST_STATE,
    READING_REQUEST_STATE,
    PROCESS_REQUEST_STATE,

    CONNECT_UPSTREAM_STATE,
    WRITING_UPSTREAM_STATE,
    READING_UPSTREAM_STATE,

    WRITING_REQUEST_STATE,
    LINGERING_CLOSE_STATE,
    KEEPALIVE_STATE
};

enum class ChunkedState
{
    sw_chunk_start = 0,
    sw_chunk_size,
    sw_chunk_extension,
    sw_chunk_extension_almost_done,
    sw_chunk_data,
    sw_after_data,
    sw_after_data_almost_done,
    sw_last_chunk_extension,
    sw_last_chunk_extension_almost_done,
    sw_trailer,
    sw_trailer_almost_done,
    sw_trailer_header,
    sw_trailer_header_almost_done
};

enum class ResponseState
{
    sw_start = 0,
    sw_H,
    sw_HT,
    sw_HTT,
    sw_HTTP,
    sw_first_major_digit,
    sw_major_digit,
    sw_first_minor_digit,
    sw_minor_digit,
    sw_status,
    sw_space_after_status,
    sw_status_text,
    sw_almost_done
};

class Header
{
  public:
    Header() = default;
    Header(std::string &&namee, std::string &&valuee);
    std::string name;
    std::string value;
    // unsigned long offset;
};

#define CONNECTION_CLOSE 0
#define CONNECTION_KEEP_ALIVE 1

class Headers_in
{
  public:
    std::list<Header> headers;
    std::unordered_map<std::string, Header> header_name_value_map;
    unsigned long content_length;
    unsigned chunked : 1;
    unsigned connection_type : 1;
};

#define RES_FILE 0
#define RES_STR 1
#define RES_EMPTY 2
#define RES_AUTO_INDEX 3

class Headers_out
{
  public:
    std::list<Header> headers;
    std::unordered_map<std::string, Header> header_name_value_map;
    unsigned long content_length;
    unsigned chunked : 1;

    int status;
    std::string status_line;

    std::string str_body;
    class
    {
      public:
        Fd filefd;
        off_t file_size;
        off_t offset;
    } file_body;
    int restype = RES_EMPTY;
};

class ChunkedInfo
{
  public:
    ChunkedInfo();
    ChunkedState state;
    size_t size;
    size_t length;
    size_t data_offset;
};

class RequestBody
{
  public:
    RequestBody();
    off_t rest;
    ChunkedInfo chunkedInfo;
    // str_t body;
    std::list<str_t> lbody;
    std::function<int(Request *)> post_handler;
};

extern Cycle *cyclePtr;

class Request
{
  public:
    void init();

    int now_proxy_pass = 0;
    int quit = 0;
    Connection *c;

    RequestBody request_body;

    Method method;
    HttpState http_state;
    HeaderState headerState = HeaderState::sw_start;
    RequestState requestState = RequestState::sw_start;
    ResponseState responseState = ResponseState::sw_start;

    uintptr_t http_version;

    Headers_in headers_in;
    Headers_out headers_out;

    str_t http_protocol;
    str_t method_name;
    str_t schema;
    str_t host;

    str_t request_line;
    str_t args;
    str_t uri;
    str_t exten;
    str_t unparsed_uri;

    off_t request_length;

    int at_phase;

    /* URI with "/." and on Win32 with "//" */
    unsigned complex_uri : 1;
    /* URI with "%" */
    unsigned quoted_uri : 1;
    /* URI with "+" */
    unsigned plus_in_uri : 1;
    /* URI with empty path */
    unsigned empty_path_in_uri : 1;
    unsigned invalid_header : 1;
    unsigned valid_unparsed_uri : 1;

    // used for parse http headers
    u_char *pos;
    // uintptr_t header_hash;
    uintptr_t lowcase_index;
    u_char lowcase_header[32];

    u_char *header_name_start;
    u_char *header_name_end;
    u_char *header_start;
    u_char *header_end;

    u_char *uri_start;
    u_char *uri_end;
    u_char *uri_ext;
    u_char *args_start;
    u_char *request_start;
    u_char *request_end;
    // method_end points to the last character of method, not the place after it
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

class Status
{
  public:
    Status();
    void init();
    uintptr_t http_version;
    uintptr_t code;
    uintptr_t count;
    u_char *start;
    u_char *end;
};

class ProxyCtx
{
  public:
    ProxyCtx();
    void init();
    Status status;
    off_t internal_body_length;
    unsigned head : 1;
    unsigned internal_chunked : 1;
    unsigned header_sent : 1;
};

class Upstream
{
  public:
    Upstream();
    Connection *c4upstream;
    Connection *c4client;
    ProxyCtx ctx;
    std::function<int(Request *r)> process_handler;
};

#define HTTP_CONTINUE 100
#define HTTP_SWITCHING_PROTOCOLS 101
#define HTTP_PROCESSING 102

#define HTTP_OK 200
#define HTTP_CREATED 201
#define HTTP_ACCEPTED 202
#define HTTP_NO_CONTENT 204
#define HTTP_PARTIAL_CONTENT 206

#define HTTP_SPECIAL_RESPONSE 300
#define HTTP_MOVED_PERMANENTLY 301
#define HTTP_MOVED_TEMPORARILY 302
#define HTTP_SEE_OTHER 303
#define HTTP_NOT_MODIFIED 304
#define HTTP_TEMPORARY_REDIRECT 307
#define HTTP_PERMANENT_REDIRECT 308

#define HTTP_BAD_REQUEST 400
#define HTTP_UNAUTHORIZED 401
#define HTTP_FORBIDDEN 403
#define HTTP_NOT_FOUND 404
#define HTTP_NOT_ALLOWED 405
#define HTTP_REQUEST_TIME_OUT 408
#define HTTP_CONFLICT 409
#define HTTP_LENGTH_REQUIRED 411
#define HTTP_PRECONDITION_FAILED 412
#define HTTP_REQUEST_ENTITY_TOO_LARGE 413
#define HTTP_REQUEST_URI_TOO_LARGE 414
#define HTTP_UNSUPPORTED_MEDIA_TYPE 415
#define HTTP_RANGE_NOT_SATISFIABLE 416
#define HTTP_MISDIRECTED_REQUEST 421
#define HTTP_TOO_MANY_REQUESTS 429

#endif