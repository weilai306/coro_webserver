//
// Created by weilai on 2024/3/18.
//
#include "Logger.h"
#include "TcpServer.h"
#include "HttpParser.h"
#include "HttpServer.h"
#include "Timer.h"


TcpServer::TcpServer() {
    main_context_ = std::make_shared<IOContext>();
    ioContextPool_ = std::make_shared<IOContextPool>();
}

void TcpServer::run() {
    if (runned)
        return;
    try {
        runned = true;
        asio::co_spawn(*main_context_, listener(), asio::detached);
        Logger::init_and_run(ioContextPool_->getNextIOContext());
        main_context_->run();
    } catch (std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}

void TcpServer::stop() {
    if (!runned)
        return;
    runned = false;
}


asio::awaitable<void> TcpServer::on_close(std::shared_ptr<tcp::socket> socket) {
    if (socket->is_open()) {
        socket->close();
    }
    co_return;
}

asio::awaitable<void> TcpServer::on_conn(std::shared_ptr<tcp::socket> socket, IOContextPtr ioContext) {
    HttpServer httpServer(ioContext, socket);
    co_spawn(*ioContext, httpServer.heartbeat(60s), asio::detached);
    while (socket->is_open()) {
        char data[1024];
        size_t bytes = co_await socket->async_receive(asio::buffer(data), asio::use_awaitable);
        LOG << bytes << " " << "bytes received from" << socket->remote_endpoint().address().to_string();
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
                res = co_await asio::co_spawn(*ioContext, httpServer.on_get(),
                                              asio::use_awaitable);
            } else if (method == http_method::HTTP_DELETE) {
                res = co_await asio::co_spawn(*ioContext, httpServer.on_delete(),
                                              asio::use_awaitable);
            } else if (method == http_method::HTTP_PUT) {
                res = co_await asio::co_spawn(*ioContext, httpServer.on_put(),
                                              asio::use_awaitable);
            } else if (method == http_method::HTTP_POST) {
                res = co_await asio::co_spawn(*ioContext, httpServer.on_post(),
                                              asio::use_awaitable);
            } else if (method == http_method::HTTP_HEAD) {
                res = co_await asio::co_spawn(*ioContext, httpServer.on_head(),
                                              asio::use_awaitable);
            } else {
                res = false;
            }
        } else if (!res) {
            co_await asio::co_spawn(*ioContext, httpServer.on_error(), asio::use_awaitable);
            break;
        }

        co_await asio::co_spawn(*ioContext, on_close(socket), asio::use_awaitable);
    }
}

asio::awaitable<void> TcpServer::listener() {
    auto executor = co_await this_coro::executor;
    asio::ip::address_v4 address = asio::ip::address_v4::from_string("0.0.0.0");
    tcp::acceptor acceptor(executor, {address, 8080});
    while (runned) {
        auto socket = std::make_shared<tcp::socket>(co_await acceptor.async_accept(asio::use_awaitable));
        auto next_io_context = ioContextPool_->getNextIOContext();
        LOG << socket->remote_endpoint().address().to_string() << "  " << socket->remote_endpoint().port();
        asio::co_spawn(*next_io_context, on_conn(socket, next_io_context), asio::detached);
    }
    co_return;
}
