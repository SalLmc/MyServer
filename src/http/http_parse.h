#ifndef HTTP_PARSE_H
#define HTTP_PARSE_H

#include "../headers.h"

class Request;
class ResponseStatus;

#define equal(a, b, len) (memcmp(a, b, len) == 0)
int parseRequestLine(std::shared_ptr<Request> r);
int parseComplexUri(std::shared_ptr<Request> r, int mergeSlashes);
int parseHeaderLine(std::shared_ptr<Request> r);
int parseChunked(std::shared_ptr<Request> r);
int parseStatusLine(std::shared_ptr<Request> r, ResponseStatus *status);

#endif