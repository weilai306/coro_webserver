#ifndef SRC_ASYNC_LOOP_H
#define SRC_ASYNC_LOOP_H


#include <liburing.h>
#include <fcntl.h>
#include <iostream>
#include <unistd.h>
#include <string>
#include <linux/io_uring.h>

struct AsyncTask{
    int fd; // 文件描述符
    enum type {
        READ,
        WRITE,
        ACCEPT,
    };
    Promise<void>* promise;
    type operator_type;
};

struct AsyncReadTask: public AsyncTask {
    char* data; // 数据指针，根据需要指向具体数据类型
    int len;
    AsyncReadTask(){
        operator_type = type::READ;
    }
};

struct AsyncWriteTask:public AsyncTask {
    char* data; // 数据指针，根据需要指向具体数据类型
    int len;

    AsyncWriteTask(){
        operator_type = type::WRITE;
    }
};

struct AsyncAcceptTask:public AsyncTask {
    struct sockaddr sockaddr;
    int new_conn_fd;

    AsyncAcceptTask(){
        operator_type = type::ACCEPT;
    }
};


class AsyncLoop {
public:
    ~AsyncLoop() {
        io_uring_queue_exit(&ring);
    }

    AsyncLoop() {
        int res = io_uring_queue_init(64,&ring,0);
        if(res < 0){
            perror("io_uring init error");
        }
    }

    void submit_tasks(AsyncTask* task_ptr) {
        struct io_uring_sqe* sqe = io_uring_get_sqe(&ring);
        if (!sqe) {
            perror("sqe nullptr");
            return;
        }

        if(task_ptr->operator_type == AsyncTask::READ){
            auto& task = *(AsyncReadTask*)task_ptr;
            io_uring_prep_read(sqe, task.fd, task.data, task.len, 0);
        }else if(task_ptr->operator_type == AsyncTask::WRITE){
            auto& task = *(AsyncWriteTask*)task_ptr;
            io_uring_prep_write(sqe, task.fd, task.data, task.len, 0);
        }else if(task_ptr->operator_type == AsyncTask::ACCEPT){
            auto& task = *(AsyncAcceptTask*)task_ptr;
            socklen_t len = sizeof(task.sockaddr);
            io_uring_prep_accept(sqe, task.fd, &task.sockaddr, &len, 0);
        }

        // 关联用户数据，可以用来在完成队列中识别任务
        io_uring_sqe_set_data(sqe, task_ptr);

        io_uring_submit(&ring); // 提交所有准备好的任务
//        std::cout << "submit" << std::endl;
    }

    void loop() {
        struct io_uring_cqe* cqe;
        unsigned head;

        // 以非阻塞方式检查完成队列
        io_uring_for_each_cqe(&ring, head, cqe) {
//            std::cout << "get" << std::endl;
            // 处理完成的任务...
            AsyncTask* task_ptr = (AsyncTask*)io_uring_cqe_get_data(cqe);
            if (task_ptr->operator_type == AsyncTask::READ) {
                auto& task = *(AsyncReadTask*)task_ptr;
                int len = cqe->res;
                task.len = len;
                task.promise->get_return_object().resume();
            }else if(task_ptr->operator_type == AsyncTask::WRITE){
                auto& task = *(AsyncWriteTask*)task_ptr;
                int len = cqe->res;
                task.len = len;
                task.promise->get_return_object().resume();
            }else if(task_ptr->operator_type == AsyncTask::ACCEPT){
                int fd = cqe->res;
                if (fd < 0) {
                    perror("accept error");
                } else {
                    auto& task = *(AsyncAcceptTask*)task_ptr;
                    task.new_conn_fd = fd;
                    task.promise->get_return_object().resume();
                }
            }
            // 标记该CQE已处理
            io_uring_cqe_seen(&ring, cqe);
        }
    }

private:
    struct io_uring ring; // io_uring 实例的文件描述符
};

class AsyncLoop;

struct AsyncAwaiter {
    bool await_ready() const noexcept {
        return false;
    }

    void await_suspend(std::coroutine_handle<Promise<void>> coroutine) {
        task_ptr->promise = &coroutine.promise();
        mLoop.submit_tasks(task_ptr);
    }

    void await_resume() const noexcept {}

    AsyncLoop &mLoop;
    AsyncTask* task_ptr;
};

#endif //SRC_ASYNC_LOOP_H
