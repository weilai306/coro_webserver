//
// Created by weilai on 3/19/24.
//

#include "HttpServer.h"

#include <asio/awaitable.hpp>
#include <asio/use_awaitable.hpp>
#include <asio/ip/tcp.hpp>
#include <iostream>
#include <thread>
#include <string>
#include "Entity.h"
#include "RestfulParser.h"
#include "Timer.h"
#include <sys/mman.h>
#include "Logger.h"
using namespace std::chrono_literals;


HttpServer::HttpServer(std::shared_ptr<asio::io_context> ioContextPtr, std::shared_ptr<tcp::socket> socket,
                       std::string url,
                       std::string req_body)
        : ioContextPtr_(ioContextPtr), socket_(socket), url_(std::move(url)), req_body_(std::move(req_body)) {

}

HttpServer::HttpServer(std::shared_ptr<asio::io_context> ioContextPtr, std::shared_ptr<tcp::socket> socket)
        : ioContextPtr_(ioContextPtr), socket_(socket) {

}

void HttpServer::setUrl(std::string &url) {
    url_ = url; // copy
}

void HttpServer::setBody(std::string &body) {
    req_body_ = body; //copy
}

void HttpServer::setUrl(std::string &&url) {
    url_ = std::move(url); // only move
}


void HttpServer::setBody(std::string &&body) {
    req_body_ = std::move(body); //only move
}

asio::awaitable<void> HttpServer::heartbeat(std::chrono::seconds duration) {
    std::string msg = "HeartbeatMessage";
    while (socket_->is_open()) {
        asio::steady_timer steadyTimer(*ioContextPtr_, duration);
        co_await steadyTimer.async_wait(asio::use_awaitable);
        LOG << "SendHeartBeat";
        co_await socket_->async_send(asio::buffer(msg), asio::use_awaitable);
    }
}

asio::awaitable<void> HttpServer::async_send(std::string resp_body, int code) {
    size_t content_length = resp_body.size();
    std::string resp_header = "HTTP/1.1 " + std::to_string(code) + " OK\r\nServer: Weilai's AsyCoroWebServer\r\n";
    resp_header += "Content-Length: " + std::to_string(content_length) + "\r\n";
    resp_header += "\r\n";

    std::string resp = resp_header + resp_body;
    co_await socket_->async_send(asio::buffer(resp), asio::use_awaitable);
    co_return;
}

std::pair<std::shared_ptr<Entity>, std::string> HttpServer::parse_to_restful() {
    RestfulParser restfulParser;
    return restfulParser.parse(url_);
}

asio::awaitable<bool> HttpServer::on_error(std::string err_msg) {
    co_await co_spawn(*ioContextPtr_, async_send(std::move(err_msg), 400), asio::use_awaitable);
    co_return false;
}

asio::awaitable<bool> HttpServer::on_get() {
    auto p = parse_to_restful();
    auto entity_ptr = p.first;
    std::string resource = p.second;
    if (entity_ptr) {
        int id = stoi(resource);
        Entity entity(id, "weilai", "hello world!");

        std::string resp_body(entity.serialize().toStyledString());
        co_await co_spawn(*ioContextPtr_, async_send(resp_body, 200), asio::use_awaitable);
    } else {
        do {
            int fd = open(resource.c_str(), O_RDONLY);
            if (fd == -1) {
                perror("Error opening file");
                break;
            }

            struct stat sb;
            if (fstat(fd, &sb) == -1) {
                perror("Error getting file size");
                close(fd);
                break;
            }

            char *mapped = static_cast<char *>(::mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0));
            if (mapped == MAP_FAILED) {
                perror("Error mapping file");
                close(fd);
                break;
            }

            co_await co_spawn(*ioContextPtr_, async_send({mapped, static_cast<size_t>(sb.st_size)}, 200),
                              asio::use_awaitable);

            if (::munmap(mapped, sb.st_size) == -1) {
                close(fd);
                break;
            }
            close(fd);
        } while (false);

        co_await co_spawn(*ioContextPtr_, on_error(), asio::use_awaitable);
        co_return false;
    }
}

asio::awaitable<bool> HttpServer::on_post() {
    auto p = parse_to_restful();
    auto entity_ptr = p.first;
    std::string resource = p.second;
    //
    // parse json -> entity
    //
    if (entity_ptr) {
        Entity entity;
        bool deserialize_res = entity.deserialize(req_body_);
        if (deserialize_res) {
            co_await co_spawn(*ioContextPtr_, async_send({entity.serialize().toStyledString()}, 200),
                              asio::use_awaitable);
        } else {
            co_await co_spawn(*ioContextPtr_, on_error(), asio::use_awaitable);
        }
    } else {
        co_await co_spawn(*ioContextPtr_, on_error(), asio::use_awaitable);
        co_return false;
    }
    co_return true;
}

asio::awaitable<bool> HttpServer::on_put() {
    auto p = parse_to_restful();
    auto entity_ptr = p.first;
    std::string resource = p.second;
    //
    // parse json -> entity
    //
    if (entity_ptr) {
        // update entity and get response
    } else {
        co_await co_spawn(*ioContextPtr_, on_error(), asio::use_awaitable);
        co_return false;
    }
    co_return true;
}

asio::awaitable<bool> HttpServer::on_delete() {
    auto p = parse_to_restful();
    auto entity_ptr = p.first;
    std::string resource = p.second;
    //
    // parse json -> entity
    //
    if (entity_ptr) {
        // delete a existed entity and get response
    } else {
        co_await co_spawn(*ioContextPtr_, on_error(), asio::use_awaitable);
        co_return false;
    }
    co_return true;
}

asio::awaitable<bool> HttpServer::on_head() {
    co_await co_spawn(*ioContextPtr_, async_send({}, 200), asio::use_awaitable);
    co_return true;
}



