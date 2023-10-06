# README

## Config

| Config            | Description                                         | Default                    |
| ----------------- | --------------------------------------------------- | -------------------------- |
| logger_wake       | The threshold to wake up the logger                 | 1                          |
| enable_logger     | True to enable logger                               | true                       |
| daemon            | True to daemonize                                   | false                      |
| only_worker       | True to have only one worker process, used in debug | false                      |
| use_epoll         | True to use epoll, else use poll                    | true                       |
| processes         | The number of worker processes                      | The number of cores (AUTO) |
| server.port       | Port to listen to                                   | 80                         |
| server.root       | Root resource location                              | "./static"                 |
| server.index      | Index page                                          | "index"                    |
| server.from       | Proxy pass source uri                               | ""                         |
| server.to         | Proxy pass target uri                               | ""                         |
| server.auto_index | True to enable directory index                      | false                      |
| server.try_files  | Files to show                                       | {}                         |
| server.auth_path  | uri need to be authenticated                        | {}                         |

### Example

```json
{
    "logger_wake": 1,
    "enable_logger": 0,
    "daemon": 1,
    "only_worker": 0,
    "use_epoll": 1,
    "servers": [
        {
            "port": 80,
            "root": "static",
            "index": "index.html"
        },
        {
            "port": 1479,
            "root": "/home/sallmc/Notes",
            "index": "sdfsdf",
            "auto_index": 1,
            "auth_path": [
                "*"
            ]
        },
        {
            "port": 1480,
            "root": "/home/sallmc/blog/public",
            "index": "index.html",
            "auth_path": [
                "/Notes/*"
            ]
        },
        {
            "port": 8082,
            "root": "/mnt/d",
            "index": "sdfxcv",
            "auto_index": 1
        }
    ]
}
```

## Usage

### Start

```shell
make && sudo ./main
```

### Stop

```shell
sudo ./main -s stop
```

### Restart

```shell
sudo ./restart.sh
```