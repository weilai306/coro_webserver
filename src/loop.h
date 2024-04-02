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
#include "event_loop.h"
#include "timer_loop.h"
#include "async_loop.h"
using namespace std::chrono_literals;


struct Loop {
    void run() {
        while (!stopped) {
            auto timeout = 60s;
            if (!new_clients.empty()) {
                auto task = new_clients.front();
                new_clients.pop_front();
                task->mCoroutine.resume();
                tasks_guards.emplace_back(task);
            }

            mTimerLoop.loop();
            mEpollLoop.loop();

            mAsyncLoop.loop();
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

    operator AsyncLoop &() {
        return mAsyncLoop;
    }

    void stop(){
        stopped = true;
    }
private:
    TimerLoop mTimerLoop;
    EpollLoop mEpollLoop;
    AsyncLoop mAsyncLoop;
    std::deque<std::shared_ptr<Task<>> > new_clients;
    std::deque<std::shared_ptr<Task<>>> tasks_guards;
    bool stopped = false;
};


#endif //SRC_LOOP_H
