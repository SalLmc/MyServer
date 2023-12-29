#include "../headers.h"

#include "core.h"

#include "../event/epoller.h"
#include "../event/poller.h"
#include "../log/logger.h"
#include "../utils/utils_declaration.h"

extern Server *serverPtr;

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

void Server::setServers(const std::vector<ServerAttribute> &servers)
{
    servers_ = servers;
}

int Server::initListen(std::function<int(Event *)> handler)
{
    for (auto &x : servers_)
    {
        Connection *c = setupListen(this, x.port_);
        if (c == NULL)
        {
            return ERROR;
        }
        c->serverIdx_ = listening_.size();

        listening_.push_back(c);

        c->read_.type_ = EventType::ACCEPT;
        c->read_.handler_ = handler;
    }
    return OK;
}

void Server::initEvent(bool useEpoll)
{
    if (useEpoll)
    {
        multiplexer_ = new Epoller();
        Epoller *epoller = dynamic_cast<Epoller *>(multiplexer_);
        epoller->setEpollFd(epoll_create1(0));
    }
    else
    {
        multiplexer_ = new Poller();
    }
}

int Server::regisListen()
{
    bool ok = 1;
    for (auto &listen : listening_)
    {
        // use LT on listenfd
        if (multiplexer_->addFd(listen->fd_.getFd(), EVENTS(IN), listen) == 0)
        {
            ok = 0;
        }
    }
    return ok ? OK : ERROR;
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

Connection *setupListen(Server *server, int port)
{
    Connection *listenConn = server->pool_.getNewConnection();
    int val = 1;

    if (listenConn == NULL)
    {
        LOG_CRIT << "get listen failed";
        return NULL;
    }

    listenConn->addr_.sin_addr.s_addr = INADDR_ANY;
    listenConn->addr_.sin_family = AF_INET;
    listenConn->addr_.sin_port = htons(port);

    listenConn->fd_ = socket(AF_INET, SOCK_STREAM, 0);

    if (listenConn->fd_.getFd() < 0)
    {
        LOG_CRIT << "open listenfd failed";
        goto bad;
    }

    if (setsockopt(listenConn->fd_.getFd(), SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val)) < 0)
    {
        LOG_WARN << "set keepalived failed";
        goto bad;
    }

    if (setsockopt(listenConn->fd_.getFd(), SOL_SOCKET, SO_REUSEPORT, &val, sizeof(val)) < 0)
    {
        LOG_CRIT << "set reuseport failed";
        goto bad;
    }

    if (setnonblocking(listenConn->fd_.getFd()) < 0)
    {
        LOG_CRIT << "set nonblocking failed";
        goto bad;
    }

    if (bind(listenConn->fd_.getFd(), (sockaddr *)&listenConn->addr_, sizeof(listenConn->addr_)) != 0)
    {
        LOG_CRIT << "bind failed";
        goto bad;
    }

    if (listen(listenConn->fd_.getFd(), 4096) != 0)
    {
        LOG_CRIT << "listen failed";
        goto bad;
    }

    LOG_INFO << "listen to " << port;

    return listenConn;

bad:
    server->pool_.recoverConnection(listenConn);
    return NULL;
}
