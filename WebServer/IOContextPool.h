//
// Created by weilai on 2024/3/18.
//

#ifndef WEBSERVER_IOCONTEXTPOOL_H
#define WEBSERVER_IOCONTEXTPOOL_H

#include <asio.hpp>
#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <iostream>
#include <memory>
class IOContextPool {
public:
    using IOContext = asio::io_context;
    using IOContextPtr = std::shared_ptr<IOContext>;

    IOContextPtr getNextIOContext(){
        auto ret = io_contexts_vec_[next_io_contexts_index++];
        if(next_io_contexts_index>=io_contexts_num){
            next_io_contexts_index = 0;
        }
        return ret;
    }

    IOContextPool(){
        io_contexts_num = std::thread::hardware_concurrency();
        for (int i = 0; i < io_contexts_num; i++) {
            IOContextPtr ioContext_in_thread = std::make_shared<IOContext>();
            work_guards_vec_.emplace_back(asio::make_work_guard(*ioContext_in_thread));
            io_contexts_vec_.emplace_back(ioContext_in_thread);
            threads_.emplace_back([ioContext_in_thread]{
                ioContext_in_thread->run();
            });
        }
    }

    ~IOContextPool(){
        stop();
    }

    void stop(){
        for(int i=0; i<io_contexts_num;i++){
            work_guards_vec_[i].reset();
            threads_[i].join();
        }
        work_guards_vec_.clear();
    }

private:
    int io_contexts_num;
    std::vector<std::thread> threads_;
    std::vector<IOContextPtr> io_contexts_vec_;
    std::vector<asio::executor_work_guard<asio::io_context::executor_type>> work_guards_vec_;
    int next_io_contexts_index = 0;
};


#endif //WEBSERVER_IOCONTEXTPOOL_H
