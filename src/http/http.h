#ifndef HTTP_H_
#define HTTP_H_

#include "../headers.h"

#include "../core/basic.h"

class Server;
class Connection;
class Event;
class Request;

enum class HeaderState
{
    START,
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
    START,
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
    START,
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
    START,
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

// enum class ResponseCode
// {
//     HTTP_OK,
//     HTTP_NOT_MODIFIED,
//     HTTP_UNAUTHORIZED,
//     HTTP_FORBIDDEN,
//     HTTP_NOT_FOUND,
//     HTTP_INTERNAL_SERVER_ERROR,
//     HTTP_MOVED_PERMANENTLY
// };

enum ResponseCode
{
    HTTP_OK = 200,
    HTTP_MOVED_PERMANENTLY = 301,
    HTTP_NOT_MODIFIED = 304,
    HTTP_UNAUTHORIZED = 401,
    HTTP_FORBIDDEN = 403,
    HTTP_NOT_FOUND = 404,
    HTTP_INTERNAL_SERVER_ERROR = 500,
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
    std::string name_;
    std::string value_;
};

class CtxIn
{
  public:
    void init();

    std::list<Header> headers_;
    std::unordered_map<std::string, Header> headerNameValueMap_;
    unsigned long contentLength_;
    bool isChunked_;
    ConnectionType connectionType_;
};

class CtxOut
{
  public:
    void init();

    std::list<Header> headers_;
    std::unordered_map<std::string, Header> headerNameValueMap_;
    unsigned long contentLength_;
    bool isChunked_;

    ResponseCode resCode_;
    std::string statusLine_;

    std::string strBody_;
    struct
    {
        Fd filefd_;
        off_t fileSize_;
        off_t offset_;
    } fileBody_;

    ResponseType resType_ = ResponseType::EMPTY;
};

class ChunkedInfo
{
  public:
    ChunkedInfo();
    void init();
    ChunkedState state_;
    size_t size_;
    size_t dataOffset_;
};

class RequestBody
{
  public:
    RequestBody();
    void init();
    off_t left_;
    ChunkedInfo chunkedInfo_;
    std::list<c_str> listBody_;
    std::function<int(std::shared_ptr<Request>)> postHandler_;
};

class Request
{
  public:
    ~Request();
    void init();

    bool nowProxyPass_ = 0;
    bool quit_ = 0;
    Connection *c_;

    RequestBody requestBody_;

    // enums
    Method method_;
    HeaderState headerState_ = HeaderState::START;
    RequestState requestState_ = RequestState::START;
    ResponseState responseState_ = ResponseState::START;

    unsigned int httpVersion_;

    CtxIn contextIn_;
    CtxOut contextOut_;

    // HTTP/1.1
    c_str protocol_;
    // GET / POST
    c_str methodName_;
    // http / https / ftp
    c_str schema_;
    // www.bing.com / 121.12.12.12
    c_str host_;

    c_str requestLine_;
    c_str args_;
    c_str uri_;
    c_str exten_;

    int atPhase_;

    // URI with "/."
    bool complexUri_;
    // URI with "%"
    bool quotedUri_;
    // URI with "+"
    bool plusInUri_;
    // URI with empty path
    bool emptyPathInUri_;

    bool invalidHeader_;

    u_char *headerNameStart_;
    u_char *headerNameEnd_;

    u_char *headerValueStart_;
    u_char *headerValueEnd_;

    u_char *uriStart_;
    u_char *uriEnd_;

    u_char *uriExt_;

    u_char *argsStart_;

    u_char *requestStart_;
    u_char *requestEnd_;

    u_char *methodEnd_;

    u_char *schemaStart_;
    u_char *schemaEnd_;

    u_char *hostStart_;
    u_char *hostEnd_;

    unsigned int httpMinor_;
    unsigned int httpMajor_;
};

class ResponseStatus
{
  public:
    ResponseStatus();
    void init();
    int httpVersion_;
    int code_;
    int count_;
    u_char *start_;
    u_char *end_;
};

class UpstreamContext
{
  public:
    void init();
    ResponseStatus status_;
    int upsIdx_;
};

class Upstream
{
  public:
    Upstream();
    Connection *upstream_;
    Connection *client_;
    UpstreamContext ctx_;
    std::function<int(std::shared_ptr<Request> r)> processHandler_;
};

class HttpCode
{
  public:
    HttpCode() = default;
    HttpCode(int code, std::string &&str);
    int code_;
    std::string str_;
};

int newConnection(Event *ev);
int waitRequest(Event *ev);
int waitRequestAgain(Event *ev);

int processRequestLine(Event *ev);
int readRequest(std::shared_ptr<Request> r);
int handleRequestUri(std::shared_ptr<Request> r);
int processRequestHeaders(Event *ev);
int tryMoveHeader(std::shared_ptr<Request> r, bool isName);
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
int finalizeRequestNow(std::shared_ptr<Request> r);
int finalizeConnectionNow(Connection *c);

// others
std::string getEtag(int fd);
bool etagMatched(int fd, std::string browserEtag);
int clientAliveCheck(Event *ev);
int blockReading(Event *ev);
int blockWriting(Event *ev);
std::string getContentType(std::string exten, Charset charset);
HttpCode getByCode(ResponseCode code);
std::string getStatusLineByCode(ResponseCode code);
int selectServer(std::shared_ptr<Request> r);

#endif