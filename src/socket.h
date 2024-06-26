
#ifndef SRC_SOCKET_H
#define SRC_SOCKET_H
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

void setNonblock(int fd) {
    int attr = 1;
    ioctl(fd, FIONBIO, &attr);
}

int socket_bind_listen(int port) {
    // 检查port值，取正确区间范围
    if (port < 0 || port > 65535) return -1;

    // 创建socket(IPv4 + TCP)，返回监听描述符
    int listen_fd = 0;
    if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) return -1;

    // 消除bind时"Address already in use"错误
    int optval = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &optval,
                   sizeof(optval)) == -1) {
        close(listen_fd);
        return -1;
    }

    // 设置服务器IP和Port，和监听描述副绑定
    struct sockaddr_in server_addr;
    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons((unsigned short)port);
    if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) ==-1) {
        close(listen_fd);
        return -1;
    }

    // 开始监听，最大等待队列长为LISTENQ
    if (listen(listen_fd, 2048) == -1) {
        close(listen_fd);
        return -1;
    }

    // 无效监听描述符
    if (listen_fd == -1) {
        close(listen_fd);
        return -1;
    }
    return listen_fd;
}

inline Task<EpollEventMask, EpollFilePromise>
wait_file_event(EpollLoop &loop, int file, EpollEventMask events) {
    co_return co_await EpollFileAwaiter(loop, file, events);
}

inline Task<int> socket_accept(EpollLoop &loop, int fd) {
    co_await wait_file_event(loop, fd, EPOLLIN);
    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(struct sockaddr_in));
    socklen_t client_addr_len = sizeof(client_addr);
    int accept_fd = 0;
    accept_fd = accept4(fd, (sockaddr *)&client_addr, &client_addr_len, SOCK_NONBLOCK);
    co_return accept_fd;
}

#endif //SRC_SOCKET_H
