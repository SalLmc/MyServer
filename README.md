# README

## Config

| Config              | Description                                                                         | Default             |
| ------------------- | ----------------------------------------------------------------------------------- | ------------------- |
| logger.threshold    | The threshold to wake up the logger                                                 | 1                   |
| logger.enable       | True to enable logger                                                               | true                |
| logger.interval     | The time gap between two logging actions, even if the threshold hasn't been reached | 3                   |
| process.daemon      | True to daemonize                                                                   | false               |
| process.only_worker | True to have only one worker process, usually used in debug                         | false               |
| process.processes   | The number of worker processes                                                      | The number of cores |
| event.use_epoll     | True to use epoll, else use poll                                                    | true                |
| event.connections   | Max connections to connect simultaneously                                           | 1024                |
| event.delay         | The interval to re-enable accept event when meet event.connections                  | 1                   |
| server.port         | Port to listen to                                                                   | 80                  |
| server.root         | Root resource location                                                              | "static"            |
| server.index        | Index page                                                                          | "index.html"        |
| server.from         | Proxy pass source uri                                                               | ""                  |
| server.to           | Proxy pass target uri                                                               | ""                  |
| server.auto_index   | True to enable directory index                                                      | false               |
| server.try_files    | Files to show                                                                       | {}                  |
| server.auth_path    | uri need to be authenticated                                                        | {}                  |

### Proxy pass

Use Nginx style

Upstream uri = to + (Request - from)

| from     | to                         | Request             | Received by upstream |
| -------- | -------------------------- | ------------------- | -------------------- |
| /webapp/ | http://localhost:5000/api/ | /webapp/foo?bar=baz | /api/foo?bar=baz     |
| /webapp/ | http://localhost:5000/api  | /webapp/foo?bar=baz | /apifoo?bar=baz      |
| /webapp  | http://localhost:5000/api/ | /webapp/foo?bar=baz | /api//foo?bar=baz    |
| /webapp  | http://localhost:5000/api  | /webapp/foo?bar=baz | /api/foo?bar=baz     |
| /webapp  | http://localhost:5000/api  | /webappfoo?bar=baz  | /apifoo?bar=baz      |
| /webapp/ | http://localhost:5000      | /webapp/foo?bar=baz | /foo?bar=baz         |
| /webapp/ | http://localhost:5000/     | /webapp/foo?bar=baz | /foo?bar=baz         |

### Example

```json
{
    "logger": {
        "enable": 1,
        "threshold": 1000,
        "interval": 5
    },
    "process": {
        "daemon": 0,
        "only_worker": 0,
        "processes": 8
    },
    "event": {
        "use_epoll": 0,
        "connections": 1024,
        "delay": 0
    },
    "servers": [
        {
            "port": 8000,
            "root": "static",
            "index": "index.html",
            "from": "/bing/",
            "to": "http://cn.bing.com/"
        },
        {
            "port": 8001,
            "root": "./",
            "index": "xxxx",
            "auto_index": 1,
            "auth_path": [
                "*README*"
            ]
        },
        {
            "port": 8082,
            "root": "/mnt/d/A_File",
            "index": "sdfsdf",
            "auto_index": 1
        }
    ]
}
```

Put your password in file "authcode_$(port)" and add it to header like "code: $(YOUR_CODE)" to gain access to authenticated paths

It's better if your auth-file doesn't contains CR,LF. Since the header parsing process ignores useless CRLF

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