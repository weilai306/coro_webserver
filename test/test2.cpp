#include <iostream>
#include <coroutine>
#include "task.h"

Task<> on_conn() {
    std::cout << "new Conn!" << std::endl;
    co_return ;
}

void run() {
    Task coro = on_conn();
    coro.mCoroutine.resume();
}

int main(){
    run();
}