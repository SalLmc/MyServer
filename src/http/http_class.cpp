#include "../headers.h"

#include "http.h"

#include "../memory/memory_manage.hpp"

extern HeapMemory heap;

Header::Header(std::string &&name, std::string &&value) : name(name), value(value)
{
}

ChunkedInfo::ChunkedInfo()
{
    state = ChunkedState::sw_chunk_start;
    size = 0;
    length = 0;
    dataOffset = 0;
}

RequestBody::RequestBody()
{
    rest = -1;
    postHandler = NULL;
}

void Request::init()
{
    nowProxyPass = 0;
    quit = 0;
    c = NULL;
    httpVersion = 0;

    requestBody.rest = -1;
    requestBody.postHandler = NULL;
    requestBody.listBody.clear();
    requestBody.chunkedInfo.state = ChunkedState::sw_chunk_start;
    requestBody.chunkedInfo.size = 0;
    requestBody.chunkedInfo.length = 0;
    requestBody.chunkedInfo.dataOffset = 0;

    headerState = HeaderState::START;
    requestState = RequestState::START;
    responseState = ResponseState::sw_start;

    inInfo.chunked = 0;
    inInfo.connectionType = CONNECTION_CLOSE;
    inInfo.contentLength = 0;
    inInfo.headerNameValueMap.clear();
    inInfo.headers.clear();

    outInfo.headers.clear();
    outInfo.headerNameValueMap.clear();
    outInfo.contentLength = 0;
    outInfo.chunked = 0;
    outInfo.status = 0;
    outInfo.statusLine.clear();
    outInfo.strBody.clear();
    outInfo.fileBody.filefd.closeFd();
    outInfo.fileBody.fileSize = 0;
    outInfo.fileBody.offset = 0;
    outInfo.restype = RES_EMPTY;

    protocol.data = NULL;
    methodName.data = NULL;
    schema.data = NULL;
    host.data = NULL;
    requestLine.data = NULL;
    args.data = NULL;
    if (complexUri || quotedUri || emptyPathInUri)
    {
        heap.hFree(uri.data);
    }
    uri.data = NULL;
    exten.data = NULL;
    unparsedUri.data = NULL;
    protocol.len = 0;
    methodName.len = 0;
    schema.len = 0;
    host.len = 0;
    requestLine.len = 0;
    args.len = 0;
    uri.len = 0;
    exten.len = 0;
    unparsedUri.len = 0;

    requestLength = 0;

    atPhase = 0;

    complexUri = 0;
    quotedUri = 0;
    plusInUri = 0;
    emptyPathInUri = 0;
    invalidHeader = 0;
    validUnparsedUri = 0;

    pos = NULL;

    headerNameStart = NULL;
    headerNameEnd = NULL;
    headerValueStart = NULL;
    headerValueEnd = NULL;

    uriStart = NULL;
    uriEnd = NULL;
    uriExt = NULL;
    argsStart = NULL;
    requestStart = NULL;
    requestEnd = NULL;
    methodEnd = NULL;
    schemaStart = NULL;
    schemaEnd = NULL;
    hostStart = NULL;
    hostEnd = NULL;
    portStart = NULL;
    portEnd = NULL;

    httpMinor = 0;
    httpMajor = 0;
}

Status::Status() : httpVersion(0), code(0), count(0), start(NULL), end(NULL)
{
}

void Status::init()
{
    httpVersion = 0;
    code = 0;
    count = 0;
    start = NULL;
    end = NULL;
}

ProxyCtx::ProxyCtx()
{
    internalBodyLength = 0;
    head = 0;
    internalChunked = 0;
    headerSent = 0;
}

void ProxyCtx::init()
{
    status.init();
    internalBodyLength = 0;
    head = 0;
    internalChunked = 0;
    headerSent = 0;
}

Upstream::Upstream()
{
    upstream = NULL;
    client = NULL;
    processHandler = NULL;
}

HttpCode::HttpCode(int code, std::string &&str) : code(code), str(str)
{
}