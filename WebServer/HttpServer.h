//
// Created by weilai on 3/19/24.
//

#ifndef WEBSERVER_HTTPSERVER_H
#define WEBSERVER_HTTPSERVER_H


#include <asio/awaitable.hpp>
#include <asio/use_awaitable.hpp>
#include <asio/ip/tcp.hpp>
#include <string>

using asio::ip::tcp;
using namespace std::chrono_literals;
class Entity;
class HttpServer {
public:
    HttpServer(std::shared_ptr<asio::io_context> ioContextPtr, std::shared_ptr<tcp::socket> socket, std::string url,
               std::string req_body);

    HttpServer(std::shared_ptr<asio::io_context> ioContextPtr, std::shared_ptr<tcp::socket> socket);

    void setUrl(std::string &url);

    void setBody(std::string &body);

    void setUrl(std::string &&url);


    void setBody(std::string &&body);

    asio::awaitable<void> heartbeat(std::chrono::seconds duration);

    asio::awaitable<void> async_send(std::string resp_body, int code);

    std::pair<std::shared_ptr<Entity>, std::string> parse_to_restful();

    asio::awaitable<bool> on_error(std::string err_msg = "");

    asio::awaitable<bool> on_get();


    asio::awaitable<bool> on_post();

    asio::awaitable<bool> on_put();

    asio::awaitable<bool> on_delete();

    asio::awaitable<bool> on_head();

private:
    std::shared_ptr<asio::io_context> ioContextPtr_;
    std::shared_ptr<tcp::socket> socket_;
    std::string url_;
    std::string req_body_;
};


#endif //WEBSERVER_HTTPSERVER_H
