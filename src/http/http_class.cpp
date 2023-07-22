#include "../headers.h"

#include "http.h"

#include "../memory/memory_manage.hpp"

extern HeapMemory heap;

Header::Header(std::string &&namee, std::string &&valuee)
{
    name = namee;
    value = valuee;
}

ChunkedInfo::ChunkedInfo()
{
    state = ChunkedState::sw_chunk_start;
    size = 0;
    length = 0;
    data_offset = 0;
}

RequestBody::RequestBody()
{
    rest = -1;
    post_handler = NULL;
}

void Request::init()
{
    now_proxy_pass = 0;
    quit = 0;
    c = NULL;
    http_version = 0;

    request_body.rest = -1;
    request_body.post_handler = NULL;
    request_body.lbody.clear();
    request_body.chunkedInfo.state = ChunkedState::sw_chunk_start;
    request_body.chunkedInfo.size = 0;
    request_body.chunkedInfo.length = 0;
    request_body.chunkedInfo.data_offset = 0;

    headerState = HeaderState::sw_start;
    requestState = RequestState::sw_start;
    responseState = ResponseState::sw_start;

    headers_in.chunked = 0;
    headers_in.connection_type = CONNECTION_CLOSE;
    headers_in.content_length = 0;
    headers_in.header_name_value_map.clear();
    headers_in.headers.clear();

    headers_out.headers.clear();
    headers_out.header_name_value_map.clear();
    headers_out.content_length = 0;
    headers_out.chunked = 0;
    headers_out.status = 0;
    headers_out.status_line.clear();
    headers_out.str_body.clear();
    headers_out.file_body.filefd.closeFd();
    headers_out.file_body.file_size = 0;
    headers_out.file_body.offset = 0;
    headers_out.restype = RES_EMPTY;

    http_protocol.data = NULL;
    method_name.data = NULL;
    schema.data = NULL;
    host.data = NULL;
    request_line.data = NULL;
    args.data = NULL;
    if (complex_uri || quoted_uri || empty_path_in_uri)
    {
        heap.hFree(uri.data);
    }
    uri.data = NULL;
    exten.data = NULL;
    unparsed_uri.data = NULL;
    http_protocol.len = 0;
    method_name.len = 0;
    schema.len = 0;
    host.len = 0;
    request_line.len = 0;
    args.len = 0;
    uri.len = 0;
    exten.len = 0;
    unparsed_uri.len = 0;

    request_length = 0;

    at_phase = 0;

    complex_uri = 0;
    quoted_uri = 0;
    plus_in_uri = 0;
    empty_path_in_uri = 0;
    invalid_header = 0;
    valid_unparsed_uri = 0;

    pos = NULL;

    lowcase_index = 0;

    header_name_start = NULL;
    header_name_end = NULL;
    header_start = NULL;
    header_end = NULL;

    uri_start = NULL;
    uri_end = NULL;
    uri_ext = NULL;
    args_start = NULL;
    request_start = NULL;
    request_end = NULL;
    method_end = NULL;
    schema_start = NULL;
    schema_end = NULL;
    host_start = NULL;
    host_end = NULL;
    port_start = NULL;
    port_end = NULL;

    http_minor = 0;
    http_major = 0;
}

Status::Status() : http_version(0), code(0), count(0), start(NULL), end(NULL)
{
}

void Status::init()
{
    http_version = 0;
    code = 0;
    count = 0;
    start = NULL;
    end = NULL;
}

ProxyCtx::ProxyCtx()
{
    internal_body_length = 0;
    head = 0;
    internal_chunked = 0;
    header_sent = 0;
}

void ProxyCtx::init()
{
    status.init();
    internal_body_length = 0;
    head = 0;
    internal_chunked = 0;
    header_sent = 0;
}

Upstream::Upstream()
{
    c4upstream = NULL;
    c4client = NULL;
    process_handler = NULL;
}