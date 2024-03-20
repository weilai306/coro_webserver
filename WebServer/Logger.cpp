//
// Created by weilai on 3/20/24.
//

#include "Logger.h"

std::mutex Logger::init_mutex_;
std::shared_ptr<Logger> Logger::loggerInstancePtr = nullptr;

#include <asio/awaitable.hpp>
#include <asio/use_awaitable.hpp>
#include <thread>
#include <mutex>
#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/posix/stream_descriptor.hpp>
#include <condition_variable>
#include <sys/eventfd.h>
#include <asio/buffer.hpp>
#include <vector>
#include <iostream>


Logger &Logger::getInstance() {
    if (!loggerInstancePtr) {
        perror("Logger uninitialized!");
    }
    return *loggerInstancePtr;
}

asio::awaitable<void> Logger::run() {
    ConstBuffer newBuffer1(new std::vector<char>);
    newBuffer1->reserve(buffer_size_);
    ConstBuffer newBuffer2(new std::vector<char>);
    newBuffer2->reserve(buffer_size_);

    std::vector<ConstBuffer> buffer_vec2;
    while (!closed) {
        {
            if (buffer_vec_.empty()) {
                char data[8];
                co_await eventfd_stream_->async_read_some(asio::buffer(data), asio::use_awaitable);
            }
//            std::cout << "begin write" << std::endl;
            {
                std::unique_lock lock(mutex_);
                if (buffer1_->size() > 0)
                    buffer_vec_.push_back(std::move(buffer1_));
                buffer1_.reset();

                buffer1_ = std::move(newBuffer1);
                buffer_vec2.swap(buffer_vec_);
                if (!buffer2_) {
                    buffer2_ = std::move(newBuffer2);
                }
            }

            std::string output;
            for (auto &i: buffer_vec2) {
                output.append(i->data(), i->size());
            }

            if (buffer_vec2.size() > 2) {
                buffer_vec2.resize(2);
            }

            if (!newBuffer1) {
                assert(!buffer_vec2.empty());
                newBuffer1 = buffer_vec2.back();
                buffer_vec2.pop_back();
                newBuffer1->clear();
                newBuffer1->reserve(buffer_size_);
            }

            if (!newBuffer2) {
                assert(!buffer_vec2.empty());
                newBuffer2 = buffer_vec2.back();
                buffer_vec2.pop_back();
                newBuffer2->clear();
                newBuffer2->reserve(buffer_size_);
            }

            buffer_vec2.clear();

            co_await log_stream_->async_write_some(asio::buffer(output), asio::use_awaitable);
        }
    }
    co_return;
}

void Logger::append(std::string data) {
    if (closed)
        return;
    std::string str_data = data + "\r\n";
    std::unique_lock lock(mutex_);
    std::cout << str_data.size() << std::endl;
    if(str_data.size() >= buffer_size_){
        return;
    }
    if (buffer1_->capacity() - buffer1_->size() < str_data.size()) {
        buffer_vec_.emplace_back(std::move(buffer1_));
        if (buffer2_) {
            buffer1_ = std::move(buffer2_);
        } else {
            buffer1_.reset(new std::vector<char>());
            buffer1_->reserve(buffer_size_);
        }

        uint64_t uint64 = 1;
        eventfd_stream_->async_write_some(asio::buffer(&uint64,sizeof uint64), asio::detached);
    } else {
        buffer1_->insert(buffer1_->end(), str_data.begin(), str_data.end());
    };
}


std::shared_ptr<Logger>
Logger::init_and_run(std::shared_ptr<asio::io_context> ioContext, size_t buffer_size) {
    std::unique_lock<std::mutex> lock(Logger::init_mutex_);

    if (!Logger::loggerInstancePtr) {
        Logger::loggerInstancePtr = std::shared_ptr<Logger>(new Logger(ioContext, buffer_size));
    }
    co_spawn(*ioContext, Logger::loggerInstancePtr->run(), asio::detached);
    return Logger::loggerInstancePtr;
}

Logger::~Logger() {
    ::close(eventfd_);
}

void Logger::close() {
    closed = true;
}


Logger::Logger(std::shared_ptr<asio::io_context> ioContext, size_t buffer_size) {
    ioContext_ = ioContext;
    buffer_size_ = buffer_size;

    eventfd_ = eventfd(0, EFD_NONBLOCK);
    if (eventfd_ == -1) {
        perror("eventfd create fail");
    }

    // 打开文件
    int logfd_ = ::open(logFileName.c_str(), O_WRONLY | O_CREAT | O_APPEND);
    if (logfd_ == -1) {
        perror("logfile create fail");
    }
    eventfd_stream_ = std::make_shared<asio::posix::stream_descriptor>(*ioContext, eventfd_);
    log_stream_ = std::make_shared<asio::posix::stream_descriptor>(*ioContext, logfd_);
    buffer1_ = std::make_shared<std::vector<char>>();
    buffer1_->reserve(buffer_size_);
    buffer2_ = std::make_shared<std::vector<char>>(buffer_size_);
    buffer2_->reserve(buffer_size_);
}

Logger &operator<<(Logger &logger, int t) {
    logger.append(std::to_string(t));
    return logger;
}

Logger &operator<<(Logger &logger, const char *t) {
    logger.append(std::string(t));
    return logger;
}

Logger &operator<<(Logger &logger, const std::string &t) {
    logger.append(t);
    return logger;
}