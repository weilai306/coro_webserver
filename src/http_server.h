#ifndef WEBSERVER_HTTPSERVER_H
#define WEBSERVER_HTTPSERVER_H

#include "task.h"
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include "restful_parser.h"
#include <sys/types.h>
#include <fcntl.h>

using namespace std::chrono_literals;

class Entity;
class HttpServer {
public:
    HttpServer(AsyncLoop& loop, AsyncFile& file);

    inline void setUrl(std::string &url);

    inline void setBody(std::string &body);

    inline void setUrl(std::string &&url);

    inline void setBody(std::string &&body);

    Task<void> heartbeat(AsyncLoop& loop, std::chrono::seconds duration);

    Task<size_t>async_send(std::string resp_body, int code);

    std::pair<std::shared_ptr<Entity>, std::string> parse_to_restful();

    Task<bool> on_error(std::string err_msg = "");

    Task<bool> on_get();

    Task<bool> on_post();

    Task<bool> on_put();

    Task<bool> on_delete();

    Task<bool> on_head();

private:
    AsyncLoop & loop_;
    AsyncFile& client_;
    std::string url_;
    std::string req_body_;
};


HttpServer::HttpServer(AsyncLoop& loop, AsyncFile& file): loop_(loop), client_(file){

}

inline void HttpServer::setUrl(std::string &url) {
    url_ = url; // copy
}

inline void HttpServer::setBody(std::string &body) {
    req_body_ = body; //copy
}

inline void HttpServer::setUrl(std::string &&url) {
    url_ = std::move(url); // only move
}

inline void HttpServer::setBody(std::string &&body) {
    req_body_ = std::move(body); //only move
}

Task<void> HttpServer::heartbeat(AsyncLoop& loop, std::chrono::seconds duration) {
    co_await TimerAwaiter(loop, 5s); //test Timer (web bench should delete this)
    std::string msg = "HeartbeatMessage";
    co_return ;
}

Task<size_t> HttpServer::async_send(std::string resp_body, int code) {
    size_t content_length = resp_body.size();
    std::string resp_header = "HTTP/1.1 " + std::to_string(code) + " OK\r\nServer: Weilai's AsyCoroWebServer\r\n";
    resp_header += "Content-Length: " + std::to_string(content_length) + "\r\n";
    resp_header += "\r\n";

    std::string resp = resp_header + resp_body;
    std::span<char const> span(resp.c_str(), resp.size());
    co_return co_await write_file(loop_,client_,span);
}

std::pair<std::shared_ptr<Entity>, std::string> HttpServer::parse_to_restful() {
    RestfulParser restfulParser;
    return restfulParser.parse(url_);
}

Task<bool>  HttpServer::on_error(std::string err_msg) {
    co_await async_send(err_msg,400);
    co_return false;
}

Task<bool>  HttpServer::on_get() {
    auto p = parse_to_restful();
    auto entity_ptr = p.first;
    std::string resource = p.second;
    if (entity_ptr) {
        int id = stoi(resource);
        Entity entity(id, "weilai", "hello world!");

        std::string resp_body(entity.serialize().toStyledString());
        co_await async_send(resp_body, 200);
    } else {
        do {
            if(resource == "hello"){
                co_await on_head();
                co_return true;
            }

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

            co_await async_send({mapped, static_cast<size_t>(sb.st_size)}, 200);

            if (::munmap(mapped, sb.st_size) == -1) {
                close(fd);
                break;
            }
            close(fd);
        } while (false);

        co_await on_error();
        co_return false;
    }
    co_return true;
}

Task<bool>  HttpServer::on_post() {
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
            co_await async_send({entity.serialize().toStyledString()}, 200);
        } else {
            co_await on_error();
        }
    } else {
        co_await on_error();
        co_return false;
    }
    co_return true;
}

Task<bool> HttpServer::on_put() {
    auto p = parse_to_restful();
    auto entity_ptr = p.first;
    std::string resource = p.second;
    //
    // parse json -> entity
    //
    if (entity_ptr) {
        // update entity and get response
    } else {
        co_await on_error();
        co_return false;
    }
    co_return true;
}

Task<bool>  HttpServer::on_delete() {
    auto p = parse_to_restful();
    auto entity_ptr = p.first;
    std::string resource = p.second;
    //
    // parse json -> entity
    //
    if (entity_ptr) {
        // delete a existed entity and get response
    } else {
        co_await on_error();
        co_return false;
    }
    co_return true;
}

Task<bool> HttpServer::on_head() {
    co_await async_send({}, 200);
    co_return true;
}


#endif //WEBSERVER_HTTPSERVER_H
