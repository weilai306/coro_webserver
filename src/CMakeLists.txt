set(SRCS
        main.cpp
        task.h
        loop.h
        socket.h
        event_loop.h
        http_server.h
        http_parser.h
        timer_loop.h
        async_loop.h
)

include_directories(${PROJECT_SOURCE_DIR}/base)
include_directories(/usr/local/include)
link_directories(/usr/local/lib)

add_executable(WebServer ${SRCS})
target_link_libraries(WebServer http_parser jsoncpp uring)
