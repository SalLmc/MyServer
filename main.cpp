#include "src/headers.h"

#include "src/core/core.h"
#include "src/core/process.h"
#include "src/event/epoller.h"
#include "src/event/poller.h"
#include "src/global.h"
#include "src/log/logger.h"
#include "src/utils/utils_declaration.h"

#include "src/utils/json.hpp"

extern int cores;
Server *serverPtr;

std::unordered_map<std::string, std::string> extenContentTypeMap;

std::vector<ServerAttribute> init();
void daemonize();

int main(int argc, char *argv[])
{
    umask(0);

    cores = std::max(1U, std::thread::hardware_concurrency());

    std::unique_ptr<Server> server(new Server(new Logger("log/", "startup")));
    serverPtr = server.get();

    std::unordered_map<std::string, std::string> mp;

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
    server->setServers(init());

    if (serverConfig.daemon)
    {
        if (server->logger_ != NULL)
        {
            delete server->logger_;
            server->logger_ = NULL;
        }

        daemonize();

        if (server->logger_ == NULL)
        {
            server->logger_ = new Logger("log/", "startup");
        }
    }

    if (writePid2File() == ERROR)
    {
        LOG_CRIT << "write pid failed";
        return 1;
    }

    masterProcessCycle(serverPtr);
}

ServerAttribute getServer(JsonResult config)
{
    ServerAttribute server;
    server.port_ = getValue(config, "port", 80);
    server.root_ = getValue(config, "root", std::string("static"));
    server.index_ = getValue(config, "index", std::string("index.html"));

    server.from_ = getValue(config, "from", std::string());
    server.to_ = getValue(config, "to", std::string());

    server.auto_index_ = getValue(config, "auto_index", 0);
    server.tryFiles_ = getValue(config, "try_files", std::vector<std::string>());

    server.authPaths_ = getValue(config, "auth_path", std::vector<std::string>());

    return server;
}

std::vector<ServerAttribute> init()
{
    MemFile typesFile(open("types.json", O_RDONLY));
    MemFile configFile(open("config.json", O_RDONLY));

    JsonParser typesParser(typesFile.addr_, typesFile.len_);
    JsonParser configParser(configFile.addr_, configFile.len_);

    JsonResult types = typesParser.parse();
    JsonResult config = configParser.parse();

    // get values
    extenContentTypeMap = types.value<std::unordered_map<std::string, std::string>>();

    serverConfig.loggerThreshold = getValue(config["logger"], "threshold", 1);
    enable_logger = serverConfig.loggerEnable = getValue(config["logger"], "enable", 1);
    serverConfig.loggerInterval = getValue(config["logger"], "interval", 3);

    serverConfig.processes = getValue(config["process"], "processes", cores);
    serverConfig.daemon = getValue(config["process"], "daemon", 0);
    serverConfig.onlyWorker = getValue(config["process"], "only_worker", 0);

    serverConfig.useEpoll = getValue(config["event"], "use_epoll", 1);
    serverConfig.eventDelay = getValue(config["event"], "delay", 1);
    serverConfig.connections = getValue(config["event"], "connections", 1024);

    JsonResult servers = config["servers"];

    std::vector<ServerAttribute> ans;
    for (int i = 0; i < servers.size(); i++)
    {
        ans.push_back(getServer(servers[i]));
    }

    return ans;
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