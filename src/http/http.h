#ifndef HTTP_H_
#define HTTP_H_

#include "../headers.h"

#include "../core/core.h"

class Request;

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
    START = 0,
    METHOD,
    SPACE_BEFORE_URI,
    SCHEMA,
    SCHEMA_SLASH0,
    SCHEMA_SLASH1,
    HOST_START,
    HOST,
    HOST_END,
    HOST_IP,
    PORT,
    AFTER_SLASH_URI,
    CHECK_URI,
    URI,
    HTTP_09,
    HTTP_H,
    HTTP_HT,
    HTTP_HTT,
    HTTP_HTTP,
    FIRST_MAJOR_DIGIT,
    MAJOR_DIGIT,
    FIRST_MINOR_DIGIT,
    MINOR_DIGIT,
    SPACES_AFTER_DIGIT,
    REQUEST_DONE
};

enum class ChunkedState
{
    START = 0,
    SIZE,
    EXTENSION,
    EXTENSION_DONE,
    DATA,
    DATA_AFTER,
    DATA_AFTER_DONE,
    LAST_EXTENSION,
    LAST_EXTENSION_DONE,
    TRAILER,
    TRAILER_DONE,
    TRAILER_HEADER,
    TRAILER_HEADER_DONE
};

enum class ResponseState
{
    START = 0,
    H,
    HT,
    HTT,
    HTTP,
    MAJOR_DIGIT0,
    MAJOR_DIGIT1,
    MINOR_DIGIT0,
    MINOR_DIGIT1,
    STATUS,
    STATUS_SPACE,
    STATUS_CONTENT,
    RESPONSE_DONE
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

enum class ResponseCode
{
    HTTP_OK,
    HTTP_NOT_MODIFIED,
    HTTP_UNAUTHORIZED,
    HTTP_FORBIDDEN,
    HTTP_NOT_FOUND,
    HTTP_INTERNAL_SERVER_ERROR
};

enum class Charset
{
    DEFAULT,
    UTF_8
};

enum class ConnectionType
{
    CLOSED,
    KEEP_ALIVE
};

enum class ResponseType
{
    FILE,
    STRING,
    EMPTY,
    AUTO_INDEX
};

class Header
{
  public:
    Header() = default;
    Header(std::string &&name, std::string &&value);
    std::string name;
    std::string value;
};

class InfoRecv
{
  public:
    std::list<Header> headers;
    std::unordered_map<std::string, Header> headerNameValueMap;
    unsigned long contentLength;
    bool isChunked;
    ConnectionType connectionType;
};

class InfoSend
{
  public:
    std::list<Header> headers;
    std::unordered_map<std::string, Header> headerNameValueMap;
    unsigned long contentLength;
    bool isChunked;

    ResponseCode resCode;
    std::string statusLine;

    std::string strBody;
    struct
    {
        Fd filefd;
        off_t fileSize;
        off_t offset;
    } fileBody;

    ResponseType restype = ResponseType::EMPTY;
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
    ~Request();
    void init();

    int nowProxyPass = 0;
    int quit = 0;
    Connection *c;

    RequestBody requestBody;

    Method method;
    HeaderState headerState = HeaderState::START;
    RequestState requestState = RequestState::START;
    ResponseState responseState = ResponseState::START;

    unsigned long httpVersion;

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

    // all end pointers point to the place after the content, except methodEnd
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
    // methodEnd points to the last character of method, not the place after it
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
    int httpVersion;
    int code;
    int count;
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

int initListen(Cycle *cycle, int port);
Connection *addListen(Cycle *cycle, int port);

int newConnection(Event *ev);
int waitRequest(Event *ev);
int waitRequestAgain(Event *ev);

int processRequestLine(Event *ev);
int readRequest(std::shared_ptr<Request> r);
int handleRequestUri(std::shared_ptr<Request> r);
int processRequestHeaders(Event *ev);
int handleRequestHeader(std::shared_ptr<Request> r, int needHost);
int processRequest(std::shared_ptr<Request> r);

int runPhases(Event *ev);

int readRequestBody(std::shared_ptr<Request> r, std::function<int(std::shared_ptr<Request>)> postHandler);
int readRequestBodyInner(Event *ev);
int processRequestBody(std::shared_ptr<Request> r);
int processBodyLength(std::shared_ptr<Request> r);
int processBodyChunked(std::shared_ptr<Request> r);

int processUpsStatusLine(std::shared_ptr<Request> upsr);
int processUpsHeaders(std::shared_ptr<Request> upsr);
int processUpsBody(std::shared_ptr<Request> upsr);

int writeResponse(Event *ev);
int sendfileEvent(Event *ev);
int sendStrEvent(Event *ev);

int keepAliveRequest(std::shared_ptr<Request> r);
int finalizeRequest(std::shared_ptr<Request> r);
int finalizeConnection(Connection *c);

// others
std::string cacheControl(int fd);
bool matchEtag(int fd, std::string browserEtag);
int blockReading(Event *ev);
int blockWriting(Event *ev);
std::string getContentType(std::string exten, Charset charset);
HttpCode getByCode(ResponseCode code);
std::string getStatusLineByCode(ResponseCode code);

#endif