#include "../headers.h"

#include "http.h"

void c_str::init()
{
    data_ = NULL;
    len_ = 0;
}

Header::Header(std::string &&name, std::string &&value) : name_(name), value_(value)
{
}

ChunkedInfo::ChunkedInfo()
{
    state_ = ChunkedState::START;
    size_ = 0;
    dataOffset_ = 0;
}

void ChunkedInfo::init()
{
    state_ = ChunkedState::START;
    size_ = 0;
    dataOffset_ = 0;
}

RequestBody::RequestBody()
{
    left_ = -1;
    postHandler_ = NULL;
}

void RequestBody::init()
{
    left_ = -1;
    postHandler_ = NULL;
    chunkedInfo_.init();
    listBody_.clear();
}

void CtxIn::init()
{
    isChunked_ = 0;
    connectionType_ = ConnectionType::CLOSED;
    contentLength_ = 0;
    headerNameValueMap_.clear();
    headers_.clear();
}

void CtxOut::init()
{
    headers_.clear();
    headerNameValueMap_.clear();
    contentLength_ = 0;
    isChunked_ = 0;
    resCode_ = HTTP_OK;
    statusLine_.clear();
    strBody_.clear();
    fileBody_.filefd_.close();
    fileBody_.fileSize_ = 0;
    fileBody_.offset_ = 0;
    resType_ = ResponseType::EMPTY;
}

Request::~Request()
{
    if (complexUri_ || quotedUri_ || emptyPathInUri_)
    {
        free(uri_.data_);
    }
}

void Request::init()
{
    nowProxyPass_ = 0;
    quit_ = 0;
    c_ = NULL;
    httpVersion_ = 0;

    requestBody_.init();

    headerState_ = HeaderState::START;
    requestState_ = RequestState::START;
    responseState_ = ResponseState::START;

    contextIn_.init();
    contextOut_.init();

    if (complexUri_ || quotedUri_ || emptyPathInUri_)
    {
        free(uri_.data_);
    }

    protocol_.init();
    methodName_.init();
    schema_.init();
    host_.init();
    requestLine_.init();
    args_.init();
    uri_.init();
    exten_.init();

    atPhase_ = 0;

    complexUri_ = 0;
    quotedUri_ = 0;
    plusInUri_ = 0;
    emptyPathInUri_ = 0;
    invalidHeader_ = 0;

    headerNameStart_ = NULL;
    headerNameEnd_ = NULL;

    headerValueStart_ = NULL;
    headerValueEnd_ = NULL;

    uriStart_ = NULL;
    uriEnd_ = NULL;

    uriExt_ = NULL;

    argsStart_ = NULL;
    requestStart_ = NULL;

    requestEnd_ = NULL;

    methodEnd_ = NULL;

    schemaStart_ = NULL;
    schemaEnd_ = NULL;
    
    hostStart_ = NULL;
    hostEnd_ = NULL;

    httpMinor_ = 0;
    httpMajor_ = 0;
}

ResponseStatus::ResponseStatus() : httpVersion_(0), code_(0), count_(0), start_(NULL), end_(NULL)
{
}

void ResponseStatus::init()
{
    httpVersion_ = 0;
    code_ = 0;
    count_ = 0;
    start_ = NULL;
    end_ = NULL;
}

void UpstreamContext::init()
{
    status_.init();
}

Upstream::Upstream()
{
    upstream_ = NULL;
    client_ = NULL;
    processHandler_ = NULL;
}

HttpCode::HttpCode(int code, std::string &&str) : code_(code), str_(str)
{
}