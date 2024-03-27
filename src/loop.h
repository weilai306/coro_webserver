#ifndef SRC_LOOP_H
#define SRC_LOOP_H

#include <deque>
#include <queue>
#include "task.h"
#include <coroutine>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <optional>
#include <memory>
#include <unordered_set>
#include <chrono>
#include <functional>
#include <sys/epoll.h>
#include <list>
#include <iostream>
#include "event_loop.h"
#include "http_parser.h"
#include "http_server.h"
#include "tcp_server.h"
#include "timer_loop.h"
using namespace std::chrono_literals;


struct AsyncLoop {
    void run() {
        while (true) {
            auto timeout = 60s;
            if (!new_clients.empty()) {
                auto task = new_clients.front();
                new_clients.pop_front();
                task->mCoroutine.resume();
                tasks_guards.emplace_back(task);
            }

            auto duration = mTimerLoop.run();
            mEpollLoop.run(duration);
        }
    }

    void addNewClientPromise(std::shared_ptr<Task<>> client){
        new_clients.emplace_back(client);
    }

    operator EpollLoop &() {
        return mEpollLoop;
    }

    operator TimerLoop &() {
        return mTimerLoop;
    }
private:
    TimerLoop mTimerLoop;
    EpollLoop mEpollLoop;
    std::deque<std::shared_ptr<Task<>> > new_clients;
    std::deque<std::shared_ptr<Task<>>> tasks_guards;
};


#endif //SRC_LOOP_H
