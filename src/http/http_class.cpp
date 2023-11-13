#include "../headers.h"

#include "http.h"

#include "../memory/memory_manage.hpp"

extern HeapMemory heap;

Header::Header(std::string &&name, std::string &&value) : name_(name), value_(value)
{
}

ChunkedInfo::ChunkedInfo()
{
    state_ = ChunkedState::START;
    size_ = 0;
    length_ = 0;
    dataOffset_ = 0;
}

RequestBody::RequestBody()
{
    left_ = -1;
    postHandler_ = NULL;
}

Request::~Request()
{
    if (complexUri_ || quotedUri_ || emptyPathInUri_)
    {
        heap.hFree(uri_.data_);
    }
}

void Request::init()
{
    nowProxyPass_ = 0;
    quit_ = 0;
    c_ = NULL;
    httpVersion_ = 0;

    requestBody_.left_ = -1;
    requestBody_.postHandler_ = NULL;
    requestBody_.listBody_.clear();
    requestBody_.chunkedInfo_.state_ = ChunkedState::START;
    requestBody_.chunkedInfo_.size_ = 0;
    requestBody_.chunkedInfo_.length_ = 0;
    requestBody_.chunkedInfo_.dataOffset_ = 0;

    headerState_ = HeaderState::START;
    requestState_ = RequestState::START;
    responseState_ = ResponseState::START;

    inInfo_.isChunked_ = 0;
    inInfo_.connectionType_ = ConnectionType::CLOSED;
    inInfo_.contentLength_ = 0;
    inInfo_.headerNameValueMap_.clear();
    inInfo_.headers_.clear();

    outInfo_.headers_.clear();
    outInfo_.headerNameValueMap_.clear();
    outInfo_.contentLength_ = 0;
    outInfo_.isChunked_ = 0;
    outInfo_.resCode_ = ResponseCode::HTTP_OK;
    outInfo_.statusLine_.clear();
    outInfo_.strBody_.clear();
    outInfo_.fileBody_.filefd_.closeFd();
    outInfo_.fileBody_.fileSize_ = 0;
    outInfo_.fileBody_.offset_ = 0;
    outInfo_.resType_ = ResponseType::EMPTY;

    protocol_.data_ = NULL;
    methodName_.data_ = NULL;
    schema_.data_ = NULL;
    host_.data_ = NULL;
    requestLine_.data_ = NULL;
    args_.data_ = NULL;
    if (complexUri_ || quotedUri_ || emptyPathInUri_)
    {
        heap.hFree(uri_.data_);
    }
    uri_.data_ = NULL;
    exten_.data_ = NULL;
    unparsedUri_.data_ = NULL;
    protocol_.len_ = 0;
    methodName_.len_ = 0;
    schema_.len_ = 0;
    host_.len_ = 0;
    requestLine_.len_ = 0;
    args_.len_ = 0;
    uri_.len_ = 0;
    exten_.len_ = 0;
    unparsedUri_.len_ = 0;

    requestLength_ = 0;

    atPhase_ = 0;

    complexUri_ = 0;
    quotedUri_ = 0;
    plusInUri_ = 0;
    emptyPathInUri_ = 0;
    invalidHeader_ = 0;
    validUnparsedUri_ = 0;

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
    portStart_ = NULL;
    portEnd_ = NULL;

    httpMinor_ = 0;
    httpMajor_ = 0;
}

Status::Status() : httpVersion_(0), code_(0), count_(0), start_(NULL), end_(NULL)
{
}

void Status::init()
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