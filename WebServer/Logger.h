//
// Created by weilai on 3/20/24.
//

#ifndef WEBSERVER_LOGGER_H
#define WEBSERVER_LOGGER_H

#include <asio/awaitable.hpp>
#include <asio/use_awaitable.hpp>
#include <thread>
#include <mutex>
#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/posix/stream_descriptor.hpp>
#include <sys/eventfd.h>
#include <asio/buffer.hpp>
#include <vector>

#define LOG Logger::getInstance() << __FILE__ << __LINE__

class Logger {
public:
    using ConstBuffer = std::shared_ptr<std::vector<char>>;

    static Logger &getInstance();

    asio::awaitable<void> run();

    void append(std::string data);

    friend Logger &operator<<(Logger &logger, int value);

    friend Logger &operator<<(Logger &logger, const char *value);

    friend Logger &operator<<(Logger &logger, const std::string &value);

    static std::shared_ptr<Logger> init_and_run(std::shared_ptr<asio::io_context> ioContext, size_t buffer_size = 2048);

    ~Logger();

    void close();

private:
    static std::mutex init_mutex_;
    static std::shared_ptr<Logger> loggerInstancePtr;
    bool closed = false;
    std::string logFileName = "server.log";
    std::shared_ptr<asio::io_context> ioContext_;
    size_t buffer_size_;
    ConstBuffer buffer1_;
    ConstBuffer buffer2_;
    std::mutex mutex_;
    std::vector<ConstBuffer> buffer_vec_;
    // 将 eventfd 包装为 ASIO 的 stream_descriptor
    std::shared_ptr<asio::posix::stream_descriptor> eventfd_stream_;
    std::shared_ptr<asio::posix::stream_descriptor> log_stream_;
    int eventfd_;
    int logfd_;


    Logger(std::shared_ptr<asio::io_context> ioContext, size_t buffer_size);
};

#endif //WEBSERVER_LOGGER_H
