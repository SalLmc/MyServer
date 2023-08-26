#include "src/headers.h"

#include "src/core/core.h"
#include "src/core/process.h"
#include "src/event/epoller.h"
#include "src/event/event.h"
#include "src/global.h"
#include "src/http/http.h"
#include "src/utils/utils_declaration.h"

#include "src/utils/json.hpp"

std::unordered_map<std::string, std::string> mp;
extern ConnectionPool cPool;
extern Cycle *cyclePtr;
extern sharedMemory shmForAMtx;
extern ProcessMutex acceptMutex;
extern Epoller epoller;
extern long cores;

void init();

int main(int argc, char *argv[])
{
    assert(system("rm -rf log") == 0);
    assert(system("mkdir log") == 0);
    cores = sysconf(_SC_NPROCESSORS_CONF);

    std::unique_ptr<Cycle> cycle(new Cycle(&cPool, new Logger("log/", "startup", 1)));
    cyclePtr = cycle.get();

    if (getOption(argc, argv, &mp) == ERROR)
    {
        LOG_CRIT << "get option failed";
        return 1;
    }

    if (init_signals() == -1)
    {
        LOG_CRIT << "init signals failed";
        return 1;
    }

    if (mp.count("signal"))
    {
        std::string signal = mp["signal"];
        pid_t pid = readPidFromFile();
        if (pid != -1)
        {
            int ret = send_signal(pid, signal);
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
        pid_t pid = readPidFromFile();
        if (pid != ERROR && kill(pid, 0) == 0)
        {
            LOG_CRIT << "server is running!";
            return 1;
        }
    }

    if (writePid2File() == ERROR)
    {
        LOG_CRIT << "write pid failed";
        return 1;
    }

    // server init
    // port root index from to auto_index try_files
    init();
    // cyclePtr->servers_.emplace_back(80, "static", "index.html", "", "", 0, std::vector<std::string>{"index.html"});
    // cyclePtr->servers_.emplace_back(1479, "/home/sallmc/myfolder", "sdfx", "", "", 1, std::vector<std::string>{});
    // cyclePtr->servers_.emplace_back(8081, "/home/sallmc/dist", "index.html", "/api/", "http://175.178.175.106:8080/",
    // 0,
    //                                 std::vector<std::string>{"index.html"});
    // cyclePtr->servers_.emplace_back(8082, "/home/sallmc/share", "sdfxcv", "", "", 1, std::vector<std::string>{});

    // accept mutex
    if (useAcceptMutex)
    {
        if (shmForAMtx.createShared(sizeof(ProcessMutexShare)) == ERROR)
        {
            LOG_CRIT << "create shm for acceptmutex failed";
            return 1;
        }

        ProcessMutexShare *share = (ProcessMutexShare *)shmForAMtx.getAddr();
        share->lock = 0;
        share->wait = 0;

        if (shmtxCreate(&acceptMutex, (ProcessMutexShare *)shmForAMtx.getAddr()) == ERROR)
        {
            LOG_CRIT << "create acceptmutex failed";
            return 1;
        }
    }

    masterProcessCycle(cyclePtr);
}

ServerAttribute getServer(nlohmann::json config)
{
    ServerAttribute server;
    server.port = config["port"];
    server.root = config["root"];
    server.index = config["index"];

    server.proxy_pass.from = config["proxy_pass"]["from"];
    server.proxy_pass.to = config["proxy_pass"]["to"];

    server.auto_index = config["auto_index"];
    server.try_files = config["try_files"];

    std::cout << server.port << " " << server.root << " " << server.index << " " << server.proxy_pass.from << " "
              << server.proxy_pass.to << " " << server.auto_index;
    for (auto x : server.try_files)
    {
        std::cout << x << " ";
    }
    std::cout << "\n";

    return server;
}

void init()
{
    std::ifstream f("config.json");
    nlohmann::json config = nlohmann::json::parse(f);

    process_n = config["processes"];
    logger_wake = config["logger_wake"];
    only_worker = config["only_worker"];
    enable_logger = config["enable_logger"];

    nlohmann::json servers = config["servers"];
    for (int i = 0; i < servers.size(); i++)
    {
        cyclePtr->servers_.push_back(getServer(servers[i]));
    }
}
