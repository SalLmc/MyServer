#include "src/headers.h"

#include "src/core/core.h"
#include "src/core/process.h"
#include "src/event/epoller.h"
#include "src/event/event.h"
#include "src/global.h"
#include "src/http/http.h"
#include "src/utils/utils_declaration.h"

#include "src/utils/json.hpp"

extern ConnectionPool cPool;
extern Cycle *cyclePtr;
extern sharedMemory shmForAMtx;
extern ProcessMutex acceptMutex;
extern long cores;

std::unordered_map<std::string, std::string> mp;
std::unordered_map<std::string, std::string> extenContentTypeMap;

void init();
void daemonize();

int main(int argc, char *argv[])
{
    umask(0);

    system("cat /proc/cpuinfo | grep cores | uniq | awk '{print $NF'} > cores");
    {
        int num = readNumberFromFile<int>("cores");
        if (num != -1)
        {
            cores = num;
        }
        else
        {
            cores = 1;
        }
    }

    std::unique_ptr<Cycle> cycle(new Cycle(&cPool, new Logger("log/", "startup")));
    cyclePtr = cycle.get();

    if (getOption(argc, argv, &mp) == ERROR)
    {
        LOG_CRIT << "get option failed";
        return 1;
    }

    if (initSignals() == -1)
    {
        LOG_CRIT << "init signals failed";
        return 1;
    }

    if (mp.count("signal"))
    {
        std::string signal = mp["signal"];
        pid_t pid = readNumberFromFile<pid_t>("pid_file");
        if (pid != -1)
        {
            int ret = sendSignal(pid, signal);
            if (ret == 0)
            {
                return 0;
            }
            else if (ret == -1)
            {
                LOG_WARN << "Process has been stopped or send signal failed";
            }
            else if (ret == -2)
            {
                LOG_WARN << "invalid signal command";
            }
            return 0;
        }
        else
        {
            LOG_CRIT << "open pid file failed";
            return 1;
        }
    }

    {
        pid_t pid = readNumberFromFile<pid_t>("pid_file");
        if (pid != ERROR && kill(pid, 0) == 0)
        {
            LOG_CRIT << "server is running!";
            printf("Server is running!\n");
            return 1;
        }
    }

    // server init
    if (system("rm -rf log") == 0)
    {
        LOG_WARN << "rm log failed";
    }

    init();

    if (is_daemon)
    {
        if (cycle->logger_ != NULL)
        {
            delete cycle->logger_;
            cycle->logger_ = NULL;
        }

        daemonize();

        if (cycle->logger_ == NULL)
        {
            cycle->logger_ = new Logger("log/", "startup");
        }
    }

    if (writePid2File() == ERROR)
    {
        LOG_CRIT << "write pid failed";
        return 1;
    }

    // set event
    if (use_epoll)
    {
        LOG_INFO << "Use epoll";
        cycle->multiplexer = new Epoller();
    }
    else
    {
        LOG_INFO << "Use poll";
        cycle->multiplexer = new Poller();
    }

    masterProcessCycle(cyclePtr);
}

template <class T> T getValue(const nlohmann::json &json, const std::string &key, T defaultValue)
{
    if (json.contains(key))
    {
        return json[key];
    }
    else
    {
        return defaultValue;
    }
}

ServerAttribute getServer(nlohmann::json config)
{
    ServerAttribute server;
    server.port = getValue(config, "port", 80);
    server.root = getValue(config, "root", std::string("static"));
    server.index = getValue(config, "index", std::string("index.html"));

    server.from = getValue(config, "from", std::string());
    server.to = getValue(config, "to", std::string());

    server.auto_index = getValue(config, "auto_index", 0);
    server.try_files = getValue(config, "try_files", std::vector<std::string>());

    server.auth_path = getValue(config, "auth_path", std::vector<std::string>());

    return server;
}

void init()
{
    std::ifstream typesStream("types.json");
    nlohmann::json types = nlohmann::json::parse(typesStream);
    extenContentTypeMap = types.get<std::unordered_map<std::string, std::string>>();

    std::ifstream configStream("config.json");
    nlohmann::json config = nlohmann::json::parse(configStream);

    processes = getValue(config, "processes", cores);
    logger_threshold = getValue(config, "logger_threshold", 1);
    only_worker = getValue(config, "only_worker", 0);
    enable_logger = getValue(config, "enable_logger", 1);
    use_epoll = getValue(config, "use_epoll", 1);

    nlohmann::json servers = config["servers"];

    for (long unsigned i = 0; i < servers.size(); i++)
    {
        cyclePtr->servers_.push_back(getServer(servers[i]));
    }

    is_daemon = getValue(config, "daemon", 0);
}

void daemonize()
{
    pid_t pid = fork();
    if (pid < 0)
    {
        exit(1);
    }
    if (pid > 0)
    {
        exit(0);
    }

    if (setsid() < 0)
    {
        exit(1);
    }

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    open("/dev/null", O_RDONLY);
    open("/dev/null", O_WRONLY);
    open("/dev/null", O_WRONLY);

    umask(0);

    signal(SIGCHLD, SIG_IGN);
}