#ifndef HTTP_H
#define HTTP_H

#include "../headers.h"

#include "../core/core.h"

class Request;

int initListen(Cycle *cycle, int port);
Connection *addListen(Cycle *cycle, int port);
int keepAliveRequest(std::shared_ptr<Request> r);
int finalizeConnection(Connection *c);
int finalizeRequest(std::shared_ptr<Request> r);
int readRequestHeader(std::shared_ptr<Request> r);
int processRequestHeader(std::shared_ptr<Request> r, int needHost);
int processRequest(std::shared_ptr<Request> r);
int readRequestBody(std::shared_ptr<Request> r, std::function<int(std::shared_ptr<Request>)> postHandler);
int processRequestBody(std::shared_ptr<Request> r);
int requestBodyLength(std::shared_ptr<Request> r);
int requestBodyChunked(std::shared_ptr<Request> r);
int processStatusLine(std::shared_ptr<Request> upsr);
int processHeaders(std::shared_ptr<Request> upsr);
int processBody(std::shared_ptr<Request> upsr);
std::string cacheControl(int fd);
bool matchEtag(int fd, std::string browserEtag);

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
int sendStrEvent(Event *ev);

enum class HeaderState
{
    START = 0,
    NAME,
    SPACE0,
    VALUE,
    SPACE1,
    IGNORE,
    LINE_DONE,
    HEADERS_DONE
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
    Header(std::string &&name, std::string &&value);
    std::string name;
    std::string value;
    // unsigned long offset;
};

#define CONNECTION_CLOSE 0
#define CONNECTION_KEEP_ALIVE 1

class InfoRecv
{
  public:
    std::list<Header> headers;
    std::unordered_map<std::string, Header> headerNameValueMap;
    unsigned long contentLength;
    unsigned chunked : 1;
    unsigned connectionType : 1;
};

#define RES_FILE 0
#define RES_STR 1
#define RES_EMPTY 2
#define RES_AUTO_INDEX 3

class InfoSend
{
  public:
    std::list<Header> headers;
    std::unordered_map<std::string, Header> headerNameValueMap;
    unsigned long contentLength;
    unsigned chunked : 1;

    int status;
    std::string statusLine;

    std::string strBody;
    struct
    {
        Fd filefd;
        off_t fileSize;
        off_t offset;
    } fileBody;
    int restype = RES_EMPTY;
};

class ChunkedInfo
{
  public:
    ChunkedInfo();
    ChunkedState state;
    size_t size;
    size_t length;
    size_t dataOffset;
};

class RequestBody
{
  public:
    RequestBody();
    off_t rest;
    ChunkedInfo chunkedInfo;
    // str_t body;
    std::list<str_t> listBody;
    std::function<int(std::shared_ptr<Request>)> postHandler;
};

extern Cycle *cyclePtr;

class Request
{
  public:
    void init();

    int nowProxyPass = 0;
    int quit = 0;
    Connection *c;

    RequestBody requestBody;

    Method method;
    HeaderState headerState = HeaderState::START;
    RequestState requestState = RequestState::sw_start;
    ResponseState responseState = ResponseState::sw_start;

    uintptr_t httpVersion;

    InfoRecv inInfo;
    InfoSend outInfo;

    str_t protocol;
    str_t methodName;
    str_t schema;
    str_t host;

    str_t requestLine;
    str_t args;
    str_t uri;
    str_t exten;
    str_t unparsedUri;

    off_t requestLength;

    int atPhase;

    // URI with "/." and on Win32 with "//"
    unsigned complexUri : 1;
    // URI with "%"
    unsigned quotedUri : 1;
    // URI with "+"
    unsigned plusInUri : 1;
    // URI with empty path
    unsigned emptyPathInUri : 1;
    unsigned invalidHeader : 1;
    unsigned validUnparsedUri : 1;

    // used for parse http headers
    u_char *pos;

    u_char *headerNameStart;
    u_char *headerNameEnd;
    u_char *headerValueStart;
    u_char *headerValueEnd;

    u_char *uriStart;
    u_char *uriEnd;
    u_char *uriExt;
    u_char *argsStart;
    u_char *requestStart;
    u_char *requestEnd;
    // method_end points to the last character of method, not the place after it
    u_char *methodEnd;
    u_char *schemaStart;
    u_char *schemaEnd;
    u_char *hostStart;
    u_char *hostEnd;
    u_char *portStart;
    u_char *portEnd;

    unsigned httpMinor : 16;
    unsigned httpMajor : 16;
};

class Status
{
  public:
    Status();
    void init();
    uintptr_t httpVersion;
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
    off_t internalBodyLength;
    unsigned head : 1;
    unsigned internalChunked : 1;
    unsigned headerSent : 1;
};

class Upstream
{
  public:
    Upstream();
    Connection *upstream;
    Connection *client;
    ProxyCtx ctx;
    std::function<int(std::shared_ptr<Request> r)> processHandler;
};

class HttpCode
{
  public:
    HttpCode() = default;
    HttpCode(int code, std::string &&str);
    int code;
    std::string str;
};

#define HTTP_OK 200
#define HTTP_NOT_MODIFIED 304
#define HTTP_UNAUTHORIZED 401
#define HTTP_FORBIDDEN 403
#define HTTP_NOT_FOUND 404
#define HTTP_INTERNAL_SERVER_ERROR 500

#define SEMICOLON_SPLIT "; "

#define UTF_8 "charset=utf-8"

#endif