//
// Created by weilai on 3/19/24.
//

#ifndef WEBSERVER_TIMER_H
#define WEBSERVER_TIMER_H


#include <asio.hpp>
#include <asio/experimental/awaitable_operators.hpp>
#include <iostream>
#include <chrono>

using asio::ip::tcp;
using asio::steady_timer;
using asio::co_spawn;
using asio::detached;
using asio::use_awaitable;
using namespace std::chrono_literals;

class Timer{
public:
    static asio::awaitable<void> async_wait_and_return(steady_timer& timer) {
        co_await timer.async_wait(use_awaitable);
    }
};
#endif //WEBSERVER_TIMER_H
