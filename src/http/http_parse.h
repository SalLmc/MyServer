#ifndef HTTP_PARSE_H
#define HTTP_PARSE_H

#include "../headers.h"

class Request;
class Status;

#define ngx_str3_cmp(m, c0, c1, c2, c3) *(uint32_t *)m == ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0)

#define ngx_str3Ocmp(m, c0, c1, c2, c3) *(uint32_t *)m == ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0)

#define ngx_str4cmp(m, c0, c1, c2, c3) *(uint32_t *)m == ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0)

#define ngx_str5cmp(m, c0, c1, c2, c3, c4) *(uint32_t *)m == ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0) && m[4] == c4

#define ngx_str6cmp(m, c0, c1, c2, c3, c4, c5)                                                                         \
    *(uint32_t *)m == ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0) && (((uint32_t *)m)[1] & 0xffff) == ((c5 << 8) | c4)

#define ngx_str7_cmp(m, c0, c1, c2, c3, c4, c5, c6, c7)                                                                \
    *(uint32_t *)m == ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0) &&                                                    \
        ((uint32_t *)m)[1] == ((c7 << 24) | (c6 << 16) | (c5 << 8) | c4)

#define ngx_str8cmp(m, c0, c1, c2, c3, c4, c5, c6, c7)                                                                 \
    *(uint32_t *)m == ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0) &&                                                    \
        ((uint32_t *)m)[1] == ((c7 << 24) | (c6 << 16) | (c5 << 8) | c4)

#define ngx_str9cmp(m, c0, c1, c2, c3, c4, c5, c6, c7, c8)                                                             \
    *(uint32_t *)m == ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0) &&                                                    \
        ((uint32_t *)m)[1] == ((c7 << 24) | (c6 << 16) | (c5 << 8) | c4) && m[8] == c8

int parseRequestLine(std::shared_ptr<Request> r);
int processRequestUri(std::shared_ptr<Request> r);
int parseComplexUri(std::shared_ptr<Request> r, int mergeSlashes);
int parseHeaderLine(std::shared_ptr<Request> r, bool allowUnderscores);
int parseChunked(std::shared_ptr<Request> r);
int parseStatusLine(std::shared_ptr<Request> r, Status *status);
#endif