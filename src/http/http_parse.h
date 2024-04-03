#ifndef HTTP_PARSE_H
#define HTTP_PARSE_H

#include "../headers.h"

class Request;
class UpsResInfo;

#define equal(a, b, len) (memcmp(a, b, len) == 0)
int parseRequestLine(std::shared_ptr<Request> r);
int parseComplexUri(std::shared_ptr<Request> r, int mergeSlashes);
int parseHeaderLine(std::shared_ptr<Request> r);
int parseChunked(std::shared_ptr<Request> r);
int parseResponseLine(std::shared_ptr<Request> r, UpsResInfo *status);

#endif