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
int processRequestHeader(Request *r);
int processRequest(Request *r);

// event handler
int newConnection(Event *ev);
int waitRequest(Event *ev);
int keepAlive(Event *ev);
int processRequestLine(Event *ev);
int processRequestHeaders(Event *ev);
int blockReading(Event *ev);
int runPhases(Event *ev);
int writeResponse(Event *ev);

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
    } file_body;
    int restype = RES_EMPTY;
};

extern Cycle *cyclePtr;

class Request
{
  public:
    void init();

    int now_proxy_pass = 0;
    int quit = 0;
    Connection *c;

    Method method;
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

    HttpState http_state;

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
    HeaderState headerState = HeaderState::sw_start;
    State state = State::sw_start;
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