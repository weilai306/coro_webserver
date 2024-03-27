#ifndef WEBSERVER_TASK_H
#define WEBSERVER_TASK_H

#include <exception>
#include <coroutine>
#include <utility>
#include "previous_awaiter.h"


template<class T>
struct Promise {
    auto initial_suspend() noexcept {
        return std::suspend_always();
    }

    auto final_suspend() noexcept {
        return PreviousAwaiter(mPrevious);
    }

    void unhandled_exception() noexcept {
        mException = std::current_exception();
    }

    void return_value(T &&ret) {
        mResult = std::move(ret);
    }

    void return_value(T const &ret) {
        mResult = ret;
    }

    T result() {
        if (mException) [[unlikely]] {
            std::rethrow_exception(mException);
        }
        return mResult;
    }

    auto get_return_object() {
        return std::coroutine_handle<Promise>::from_promise(*this);
    }

    std::coroutine_handle<> mPrevious;
    std::exception_ptr mException{};
    T mResult; // destructed??

    Promise &operator=(Promise &&) = delete;
};

template<>
struct Promise<void> {
    auto initial_suspend() noexcept {
        return std::suspend_always();
    }

    auto final_suspend() noexcept {
        return PreviousAwaiter(mPrevious);
    }

    void unhandled_exception() noexcept {
        mException = std::current_exception();
    }

    void return_void() noexcept {}

    void result() {
        if (mException) [[unlikely]] {
            std::rethrow_exception(mException);
        }
    }

    auto get_return_object() {
        return std::coroutine_handle<Promise>::from_promise(*this);
    }

    std::coroutine_handle<> mPrevious;
    std::exception_ptr mException{};
};

template<class T = void, class P = Promise<T>>
struct [[nodiscard]] Task {
    using promise_type = P;

    Task(std::coroutine_handle<promise_type> coroutine = nullptr) noexcept
            : mCoroutine(coroutine) {}

    Task(Task &&that) noexcept: mCoroutine(that.mCoroutine) {
        that.mCoroutine = nullptr;
    }

    Task &operator=(Task &&that) noexcept {
        std::swap(mCoroutine, that.mCoroutine);
    }

    ~Task() {
        if (mCoroutine)
            mCoroutine.destroy(); // TODOTODO
    }

    struct Awaiter {
        bool await_ready() const noexcept {
            return false;
        }

        std::coroutine_handle<promise_type>
        await_suspend(std::coroutine_handle<> coroutine) const noexcept {
            promise_type &promise = mCoroutine.promise();
            promise.mPrevious = coroutine;
            return mCoroutine;
        }

        T await_resume() const {
            return mCoroutine.promise().result();
        }

        std::coroutine_handle<promise_type> mCoroutine;
    };

    auto operator
    co_await() const noexcept {
        return Awaiter(mCoroutine);
    }

    operator std::coroutine_handle<promise_type>() const noexcept {
        return mCoroutine;
    }

    std::coroutine_handle<promise_type> mCoroutine;
};

#endif //WEBSERVER_TASK_H
