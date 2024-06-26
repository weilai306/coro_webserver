
#ifndef WEBSERVER_TCPSERVER_H
#define WEBSERVER_TCPSERVER_H

#include "task.h"
#include "loop.h"
#include "socket.h"
#include "event_loop.h"
#include <memory>
#include <algorithm>
#include "http_server.h"
#include "http_parser.h"
class TcpServer {
public:
    Loop &getNextLoop() {
        auto res = loops[next_loop++];
        if (next_loop >= n) {
            next_loop = 0;
        }
        return *res;
    }

    void run() {
        for (int i = 0; i < n; i++) {
            loops.emplace_back(new Loop);
            threads.emplace_back(std::thread([this, i]() {
                auto &loop = this->loops[i];
                loop->run();
            }));
        }

        auto l = listen();
        l.mCoroutine.resume();
        baseLoop.run();
    }

    static Task<> on_conn(Loop &loop, int client_fd) {
        AsyncFile clientFile(client_fd);
        HttpServer httpServer(loop, clientFile);
        while (true) {
//            char data[1024];
//            size_t bytes = co_await read_file(loop, clientFile, data); //epoll version

            char data[1024];
            AsyncReadTask readTask{};
            readTask.fd = client_fd;
            readTask.data = data;
            readTask.len = 1024;
            co_await AsyncAwaiter(loop, &readTask);
            int bytes = readTask.len;

            if (bytes == 0) {
                break;
            }
            HttpParser httpParser;
            bool parse_res = co_await httpParser.parse((char *) data, bytes);

            auto method = httpParser.method_;
            bool res = false;
            httpServer.setUrl(httpParser.getUrl());
            httpServer.setBody(httpParser.getBody());
            if (parse_res) {
                if (method == http_method::HTTP_GET) {
                    res = co_await httpServer.on_get();
                } else if (method == http_method::HTTP_DELETE) {
                    res = co_await httpServer.on_delete();
                } else if (method == http_method::HTTP_PUT) {
                    res = co_await httpServer.on_put();
                } else if (method == http_method::HTTP_POST) {
                    res = co_await httpServer.on_post();
                } else if (method == http_method::HTTP_HEAD) {
                    res = co_await httpServer.on_head();
                } else {
                    res = false;
                }
            }
            if (!res) {
                break;
            }
        }
        co_return;
    }

    Task<> listen() {
        int listen_fd = socket_bind_listen(8080);

        while (listen_fd) {
            AsyncAcceptTask acceptTask;
            acceptTask.fd = listen_fd;
            co_await AsyncAwaiter(baseLoop, &acceptTask);
            int client_fd = acceptTask.new_conn_fd; // io_uring version
//            std::cout << client_fd << std::endl;
            auto &Loop = getNextLoop();
            Loop.addNewClientPromise(std::make_shared<Task<>>(on_conn(Loop, client_fd)));
        }
        co_return;
    }

private:
    std::vector<std::thread> threads;
    std::vector<Loop *> loops;
    Loop baseLoop;
    int n = std::thread::hardware_concurrency() - 1 <= 0 ? 1 : std::thread::hardware_concurrency() - 1;
    int next_loop = 0;
};


#endif //WEBSERVER_TCPSERVER_H
