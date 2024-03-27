set(SRCS
#        test.cpp
        task.h
        test2.cpp
        previous_awaiter.h
)

include_directories(${PROJECT_SOURCE_DIR}/base)
include_directories(/usr/local/include)
#include_directories(./header)
link_directories(/usr/local/lib)

add_executable(WebServer ${SRCS})
target_link_libraries(WebServer http_parser jsoncpp ssl crypto)
