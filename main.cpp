#include "src/headers.h"

#include "src/core/core.h"
#include "src/core/process.h"
#include "src/event/epoller.h"
#include "src/event/poller.h"
#include "src/global.h"
#include "src/log/logger.h"
#include "src/utils/utils.h"

#include "src/utils/json.hpp"

extern ServerAttribute defaultAttr;

// need to define these global variables
Server *serverPtr;
bool enable_logger = 0;
Level logger_level = Level::INFO;

std::vector<ServerAttribute> readServerConfig();
std::unordered_map<std::string, std::string> readTypesConfig();
void daemonize();
int preWork(int argc, char *argv[]);

int main(int argc, char *argv[])
{
    umask(0);

    std::unique_ptr<Server> server(new Server(new Logger("log/", "startup")));
    serverPtr = server.get();

    switch (preWork(argc, argv))
    {
    case DONE:
        return 0;
        break;
    case OK:
        break;
    default:
        printf("Server starts failed\n");
        return 1;
        break;
    }

    // server init
    server->setServers(readServerConfig());
    server->setTypes(readTypesConfig());

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

    master(serverPtr);
}

int preWork(int argc, char *argv[])
{
    std::unordered_map<Arg, std::string> mp;

    if (getOption(argc, argv, &mp) == ERROR)
    {
        LOG_CRIT << "get option failed";
        return ERROR;
    }

    if (initSignals() == -1)
    {
        LOG_CRIT << "init signals failed";
        return ERROR;
    }

    if (mp.count(Arg::SIGNAL))
    {
        std::string signal = mp[Arg::SIGNAL];
        pid_t pid = readNumberFromFile<pid_t>("pid_file");
        if (pid != -1)
        {
            int ret = sendSignal(pid, signal);
            if (ret == -1)
            {
                LOG_WARN << "Process has been stopped or send signal failed";
            }
            else if (ret == -2)
            {
                LOG_WARN << "invalid signal command";
            }
            return DONE;
        }
        else
        {
            LOG_CRIT << "open pid file failed";
            return ERROR;
        }
    }

    {
        pid_t pid = readNumberFromFile<pid_t>("pid_file");
        if (pid != ERROR && kill(pid, 0) == 0)
        {
            LOG_CRIT << "server is running!";
            printf("Server is running!\n");
            return ERROR;
        }
    }

    return OK;
}

ServerAttribute getServer(JsonResult config)
{
    ServerAttribute server = defaultAttr;

    setIfValid(config, "port", &server.port);
    setIfValid(config, "root", &server.root);
    setIfValid(config, "index", &server.index);

    setIfValid(config, "from", &server.from);
    setIfValid(config, "to", &server.to);
    setIfValid(config, "auto_index", &server.autoIndex);

    setIfValid(config, "try_files", &server.tryFiles);
    setIfValid(config, "auth_path", &server.authPaths);

    return server;
}

std::vector<ServerAttribute> readServerConfig()
{
    MemFile configFile(open("config.json", O_RDONLY));

    JsonParser configParser(configFile.addr_, configFile.len_);

    JsonResult config = configParser.parse();

    // get values
    setIfValid(config["logger"], "threshold", &serverConfig.loggerThreshold);
    setIfValid(config["logger"], "enable", &serverConfig.loggerEnable);
    enable_logger = serverConfig.loggerEnable;
    setIfValid(config["logger"], "interval", &serverConfig.loggerInterval);
    setIfValid(config["logger"], "level", &serverConfig.loggerLevel);
    logger_level = Level(serverConfig.loggerLevel);

    setIfValid(config["process"], "processes", &serverConfig.processes);
    setIfValid(config["process"], "daemon", &serverConfig.daemon);
    setIfValid(config["process"], "only_worker", &serverConfig.onlyWorker);

    setIfValid(config["event"], "use_epoll", &serverConfig.useEpoll);
    setIfValid(config["event"], "delay", &serverConfig.eventDelay);
    setIfValid(config["event"], "connections", &serverConfig.connections);

    JsonResult servers = config["servers"];

    std::vector<ServerAttribute> ans;
    for (int i = 0; i < servers.size(); i++)
    {
        ans.push_back(getServer(servers[i]));
    }

    return ans;
}

std::unordered_map<std::string, std::string> readTypesConfig()
{
    MemFile types(open("types.json", O_RDONLY));

    JsonParser parser(types.addr_, types.len_);
    JsonResult res = parser.parse();

    return res.value(std::unordered_map<std::string, std::string>());
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