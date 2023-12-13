#include "../headers.h"

#include "core.h"

#include "../event/event.h"
#include "../log/logger.h"
#include "../utils/utils_declaration.h"

Server *serverPtr;

Event::Event(Connection *c) : c_(c), type_(EventType::NORMAL), timeout_(TimeoutStatus::NOT_TIMED_OUT)
{
}

void Event::init(Connection *conn)
{
    c_ = conn;
    type_ = EventType::NORMAL;
    timeout_ = TimeoutStatus::NOT_TIMED_OUT;
    handler_ = std::function<int(Event *)>();
}

Connection::Connection(ResourceType type)
    : read_(this), write_(this), serverIdx_(-1), request_(NULL), upstream_(NULL), quit_(0), type_(type)
{
}

void Connection::init(ResourceType type)
{
    read_.init(this);
    write_.init(this);

    if (fd_ != -1)
    {
        close(fd_.getFd());
        fd_ = -1;
    }

    readBuffer_.init();
    writeBuffer_.init();

    serverIdx_ = -1;

    request_.reset();
    upstream_.reset();

    quit_ = 0;

    type_ = type;
}

ConnectionPool::ConnectionPool(int size) : activeCnt(0)
{
    for (int i = 0; i < size; i++)
    {
        Connection *c = new Connection(ResourceType::POOL);
        connectionList_.push_back(c);
        connectionPtrs_.push_back(c);
    }
}

ConnectionPool::~ConnectionPool()
{
    for (auto c : connectionPtrs_)
    {
        delete c;
    }

    for (auto c : savePtrs_)
    {
        delete (Connection *)c;
    }
}

Connection *ConnectionPool::getNewConnection()
{
    Connection *c;

    if (!connectionList_.empty())
    {
        c = connectionList_.front();
        connectionList_.pop_front();
    }
    else
    {
        c = new Connection(ResourceType::MALLOC);
        savePtrs_.insert((uint64_t)c);
    }

    if (c != NULL)
    {
        activeCnt++;
    }

    return c;
}

void ConnectionPool::recoverConnection(Connection *c)
{
    if (c == NULL)
    {
        return;
    }

    activeCnt--;

    if (c->type_ == ResourceType::POOL)
    {
        c->init(ResourceType::POOL);
        connectionList_.push_back(c);
    }
    else
    {
        savePtrs_.erase((uint64_t)c);
        delete c;
    }

    return;
}

ServerAttribute::ServerAttribute(int port, std::string &&root, std::string &&index, std::string &&from,
                                 std::string &&to, bool auto_index, std::vector<std::string> &&tryfiles,
                                 std::vector<std::string> &&auth_path)
    : port_(port), root_(root), index_(index), from_(from), to_(to), auto_index_(auto_index), tryFiles_(tryfiles),
      authPaths_(auth_path)
{
}

Server::Server(Logger *logger) : logger_(logger), multiplexer_(NULL)
{
}

Server::~Server()
{
    if (logger_ != NULL)
    {
        delete logger_;
        logger_ = NULL;
    }
    if (multiplexer_ != NULL)
    {
        delete multiplexer_;
    }
}

void Server::eventLoop()
{
    int flags = NORMAL;

    unsigned long long nextTick = timer_.getNextTick();
    nextTick = ((nextTick == (unsigned long long)-1) ? -1 : (nextTick - getTickMs()));

    int ret = multiplexer_->processEvents(flags, nextTick);

    if (ret == -1)
    {
        LOG_WARN << "process events errno: " << strerror(errno);
    }

    multiplexer_->processPostedAcceptEvents();

    timer_.tick();

    multiplexer_->processPostedEvents();
}
