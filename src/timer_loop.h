//
// Created by weilai on 3/27/24.
//

#ifndef SRC_TIMER_LOOP_H
#define SRC_TIMER_LOOP_H

class TimerLoop{
private:
    std::priority_queue< std::pair<std::chrono::time_point<std::chrono::steady_clock>, Promise<void>*> > mHeap;
public:
    void addTimer(std::chrono::seconds seconds, Promise<void>* promise_ptr){
        auto timePoint = std::chrono::steady_clock::now() + seconds;
        auto pair = std::make_pair(timePoint,promise_ptr);
        mHeap.push(pair);
    }

    void addTimer(std::chrono::milliseconds milliseconds, Promise<void>* promise_ptr){
        auto timePoint = std::chrono::steady_clock::now() + milliseconds;
        auto pair = std::make_pair(timePoint,promise_ptr);
        mHeap.push(pair);
    }

    std::optional<std::chrono::steady_clock::duration> loop(){
        if(mHeap.size()){
            auto top = mHeap.top();

            if(std::chrono::steady_clock::now() > top.first){
                top.second->get_return_object().resume();
                mHeap.pop();
            }else{
                return top.first - std::chrono::steady_clock::now();
            }
        }
        return std::nullopt;
    }
};

class TimerAwaiter{
public:
    bool await_ready() const noexcept {
        return false;
    }

    void await_suspend(std::coroutine_handle<Promise<void>> coroutine) {
        auto &promise = coroutine.promise();
        timerLoop.addTimer(duration, &promise);
    }

    void await_resume() const noexcept {
        return;
    }

    TimerLoop & timerLoop;
    std::chrono::seconds duration;
};

#endif //SRC_TIMER_LOOP_H
