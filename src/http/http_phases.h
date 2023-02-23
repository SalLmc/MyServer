#ifndef HTTP_PHASES_H
#define HTTP_PHASES_H

#include <functional>

class Request;

class PhaseHandler
{
  public:
    PhaseHandler() = default;
    std::function<int(Request *, PhaseHandler *)> checker;
    std::function<int(Request *)> handler;
    unsigned long next;
};

#define HTTP_POST_READ_PHASE 0
#define HTTP_SERVER_REWRITE_PHASE 1
#define HTTP_FIND_CONFIG_PHASE 2
#define HTTP_REWRITE_PHASE 3
#define HTTP_POST_REWRITE_PHASE 4
#define HTTP_PREACCESS_PHASE 5
#define HTTP_ACCESS_PHASE 6
#define HTTP_POST_ACCESS_PHASE 7
#define HTTP_PRECONTENT_PHASE 8
#define HTTP_CONTENT_PHASE 9
#define HTTP_LOG_PHASE 10

PhaseHandler phases[] = {{NULL, NULL, 0}, {NULL, NULL, 0}, {NULL, NULL, 0}, {NULL, NULL, 0},
                         {NULL, NULL, 0}, {NULL, NULL, 0}, {NULL, NULL, 0}, {NULL, NULL, 0},
                         {NULL, NULL, 0}, {NULL, NULL, 0}, {NULL, NULL, 0}};

#endif