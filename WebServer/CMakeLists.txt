set(SRCS
    Main.cpp
    TcpServer.cpp
    IOContextPool.cpp
        HttpParser.cpp
        HttpParser.h
        HttpServer.cpp
        HttpServer.h
        Timer.cpp
        Timer.h
        RestfulParser.cpp
        RestfulParser.h
        Entity.cpp
        Entity.h
        Logger.cpp
        Logger.h
)

include_directories(${PROJECT_SOURCE_DIR}/base)
include_directories(/usr/local/include)
link_directories(/usr/local/lib)

add_executable(WebServer ${SRCS})
target_link_libraries(WebServer http_parser jsoncpp ssl crypto)

#add_subdirectory(base)
#add_subdirectory(tests)