#include <thread>
#include <iostream>
#include "tcp_server.h"

int main() {
    TcpServer tcpServer;
    tcpServer.run();
}