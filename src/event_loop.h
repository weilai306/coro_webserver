#pragma once

#include <coroutine>
#include <chrono>
#include <cstdint>
#include <utility>
#include <optional>
#include <string>
#include <string_view>
#include <span>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <cstring>
#include "task.h"


using EpollEventMask = std::uint32_t;

struct EpollFilePromise : Promise<EpollEventMask> {
    auto get_return_object() {
        return std::coroutine_handle<EpollFilePromise>::from_promise(*this);
    }

    EpollFilePromise &operator=(EpollFilePromise &&) = delete;

    inline ~EpollFilePromise();

    struct EpollFileAwaiter *mAwaiter{};
};

struct EpollLoop {
    inline bool addListener(EpollFilePromise &promise, int ctl);

    inline void removeListener(int fileNo);

    inline bool run(std::optional<std::chrono::system_clock::duration> timeout =
    std::nullopt);

    bool hasEvent() const noexcept {
        return fd_events.size()>0;
    }

//    EpollLoop &operator=(EpollLoop &&) = delete;

    EpollLoop(){
        mEventBuf.resize(64);
    }

    ~EpollLoop() {
        close(mEpoll);
    }

    std::unordered_map<int,uint32_t> fd_events;
    int mEpoll = epoll_create1(0);
    std::vector<epoll_event> mEventBuf;
};

struct EpollFileAwaiter {
    bool await_ready() const noexcept {
        return false;
    }

    void await_suspend(std::coroutine_handle<EpollFilePromise> coroutine) {
        auto &promise = coroutine.promise();
        promise.mAwaiter = this;
        if (!mLoop.addListener(promise, mCtlCode)) {
            promise.mAwaiter = nullptr;
            coroutine.resume();
        }
    }

    EpollEventMask await_resume() const noexcept {
        return mResumeEvents;
    }

    EpollLoop &mLoop;
    int mFileNo;
    EpollEventMask mEvents;
    EpollEventMask mResumeEvents;
    int mCtlCode = EPOLL_CTL_ADD;
};

EpollFilePromise::~EpollFilePromise() {
    if (mAwaiter) {
        mAwaiter->mLoop.removeListener(mAwaiter->mFileNo);
    }
}

bool EpollLoop::addListener(EpollFilePromise &promise, int ctl) {
    struct epoll_event event{};
    bzero(&event,sizeof event);
    event.events = promise.mAwaiter->mEvents;
    event.data.ptr = &promise;

    if(fd_events.count(promise.mAwaiter->mFileNo)){
        if(fd_events[promise.mAwaiter->mFileNo] == event.events){
            return true;
        }
        fd_events[promise.mAwaiter->mFileNo]= event.events;
        ctl = EPOLL_CTL_MOD;
//        std::cout << "epoll mod " << promise.mAwaiter->mFileNo << " events:" << event.events << " on epoll "<< mEpoll << std::endl;
    }else{
        fd_events[promise.mAwaiter->mFileNo]= event.events;
        ctl = EPOLL_CTL_ADD;
//        std::cout << "epoll add " << promise.mAwaiter->mFileNo << " events:" << event.events << " on epoll "<< mEpoll <<std::endl;
    }

    int res = epoll_ctl(mEpoll, ctl, promise.mAwaiter->mFileNo, &event);
    if (res == -1)
        return false;

    return true;
}

void EpollLoop::removeListener(int fileNo) {
    epoll_ctl(mEpoll, EPOLL_CTL_DEL, fileNo, NULL);
    fd_events.erase(fileNo);
//    std::cout << "remove " << fileNo << " on thread: " << std::this_thread::get_id() << std::endl;
}

bool EpollLoop::run(std::optional<std::chrono::steady_clock::duration> timeout) {
    if (fd_events.size() == 0) {
        return false;
    }
//    std::cout << std::this_thread::get_id() << std::endl;
    int timeoutInMs = -1;
    if (timeout) {
        timeoutInMs = std::chrono::duration_cast<std::chrono::milliseconds>(*timeout).count();
    }
    int res = epoll_wait(mEpoll, &*mEventBuf.begin(), mEventBuf.size(), timeoutInMs);
    if(res<0){
//        std::cout << errno << std::endl;
        return false;
    }
    for (int i = 0; i < res; i++) {
//        std::cout << "epoll: trigger on : "  <<  mEventBuf[i].events << " on epoll "<< mEpoll << std::endl;
        auto &event = mEventBuf[i];
        auto &promise = *(EpollFilePromise *) event.data.ptr;
        promise.mAwaiter->mResumeEvents = event.events;
    }
    for (int i = 0; i < res; i++) {
        auto &event = mEventBuf[i];
        auto &promise = *(EpollFilePromise *) event.data.ptr;
        std::coroutine_handle<EpollFilePromise>::from_promise(promise).resume();
    }
    return true;
}

struct [[nodiscard]] AsyncFile {
    AsyncFile() : mFileNo(-1) {}

    explicit AsyncFile(int fileNo) noexcept: mFileNo(fileNo) {}

    AsyncFile(AsyncFile &&that) noexcept: mFileNo(that.mFileNo) {
        that.mFileNo = -1;
    }

    AsyncFile &operator=(AsyncFile &&that) noexcept {
        std::swap(mFileNo, that.mFileNo);
        return *this;
    }

    ~AsyncFile() {
        if (mFileNo != -1)
            close(mFileNo);
    }

    int fileNo() const noexcept {
        return mFileNo;
    }

    int releaseOwnership() noexcept {
        int ret = mFileNo;
        mFileNo = -1;
        return ret;
    }

    void setNonblock() const {
        int attr = 1;
        ioctl(fileNo(), FIONBIO, &attr);
    }

private:
    int mFileNo;
};

inline Task <EpollEventMask, EpollFilePromise>
wait_file_event(EpollLoop &loop, AsyncFile &file, EpollEventMask events) {
    co_return co_await EpollFileAwaiter(loop, file.fileNo(), events);
}

inline std::size_t readFileSync(AsyncFile &file, std::span<char> buffer) {
    return read(file.fileNo(), buffer.data(), buffer.size());
}

inline std::size_t writeFileSync(AsyncFile &file, std::span<char const> buffer) {
    return write(file.fileNo(), buffer.data(), buffer.size());
}

inline Task <std::size_t> read_file(EpollLoop &loop, AsyncFile &file, std::span<char> buffer) {
    co_await wait_file_event(loop, file, EPOLLIN | EPOLLRDHUP);
    auto len = readFileSync(file, buffer);
    co_return len;
}

inline Task <std::size_t> write_file(EpollLoop &loop, AsyncFile &file,
                                     std::span<char const> buffer) {
    co_await wait_file_event(loop, file, EPOLLOUT | EPOLLHUP);
    auto len = writeFileSync(file, buffer);
    co_return len;
}


