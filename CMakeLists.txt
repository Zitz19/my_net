cmake_minimum_required(VERSION 3.0.0)
project(StudNet VERSION 0.1.0 LANGUAGES C CXX)

include(CTest)
enable_testing()

add_executable(StudNet
    src/main.cpp
    src/core/client.cpp
    src/core/peer.cpp
    src/config/config.cpp
    src/packet/packet.cpp
)
target_include_directories(StudNet PRIVATE "include" "src" "/home/zitz/my_net/include/boost_1_84_0")

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
