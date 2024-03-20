//
// Created by weilai on 2024/3/18.
//

#ifndef WEBSERVER_TCPSERVER_H
#define WEBSERVER_TCPSERVER_H


#include "IOContextPool.h"


using asio::ip::tcp;
using namespace std::chrono_literals;
namespace this_coro = asio::this_coro;


class TcpServer {
public:
    using IOContext = asio::io_context;
    using IOContextPtr = std::shared_ptr<IOContext>;
    using IOContextPoolPtr = std::shared_ptr<IOContextPool>;

    TcpServer();

    void run() ;

    void stop() ;

private:
    asio::awaitable<void> on_close(std::shared_ptr<tcp::socket> socket);

    asio::awaitable<void> on_conn(std::shared_ptr<tcp::socket> socket, IOContextPtr ioContext);

    asio::awaitable<void> listener() ;

    IOContextPoolPtr ioContextPool_;
    IOContextPtr main_context_;
    bool runned = false;
};


#endif //WEBSERVER_TCPSERVER_H
