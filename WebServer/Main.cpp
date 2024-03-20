#include <thread>
#include <iostream>
#include "TcpServer.h"
#include "Logger.h"
int main() {
//    std::cout << std::hash<std::thread::id>{}(std::this_thread::get_id()) << " main" << std::endl;

    TcpServer tcpServer;
    tcpServer.run();
//    tcpServer.stop();
}