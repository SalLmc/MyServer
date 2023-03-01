#include "http.h"

Header::Header(std::string &&namee, std::string &&valuee)
{
    name = namee;
    value = valuee;
}

void Request::init()
{
    quit = 0;
    c = NULL;
    
    headers_in.chunked = 0;
    headers_in.connection_type = 0;
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
    headers_out.file_body.filefd = -1;
    headers_out.file_body.file_size = 0;
    headers_out.restype = RES_EMPTY;

    at_phase=0;
}