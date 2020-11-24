cmake_minimum_required(VERSION 3.10.2 FATAL_ERROR)      # 指定 cmake 的最小版本
cmake_policy(VERSION 3.10.2)
project(MSF_LIB_PROJECT)                                    # 设置项目名称

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Agent
    ${CMAKE_CURRENT_SOURCE_DIR}/Agent/Proto
    ${CMAKE_CURRENT_SOURCE_DIR}/Agent/Client
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib/
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib/base
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib/conf
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib/net
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib/net/sock
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib/net/event
    ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/baidu/incubator-brpc/src/
)

# 设置链接库搜索目录
link_directories(
    ${PROJECT_SOURCE_DIR}/../build/lib/
    ${PROJECT_SOURCE_DIR}/../build/app/AgentProto/
    ${PROJECT_SOURCE_DIR}/../build/app/AgentClient
    ${PROJECT_SOURCE_DIR}/../thirdparty/baidu/incubator-brpc
)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/../cmake)
include (${CMAKE_MODULE_PATH}/FindNUMA.cmake)
include (${CMAKE_MODULE_PATH}/utils.cmake)
#include (${CMAKE_MODULE_PATH}/FindProtobuf.cmake)

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++17" COMPILER_SUPPORTS_CXX17)
CHECK_CXX_COMPILER_FLAG("-std=c++1a" COMPILER_SUPPORTS_CXX1a)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)

# c++17
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++17 -fPIC -O3 -Wall -g -ggdb")

# 设置安装前缀
# https://blog.csdn.net/qq_38410730/article/details/102837401
set(CMAKE_INSTALL_PREFIX /home/luotang.me)

add_subdirectory (Agent AgentServer)
add_subdirectory (WatchDog WatchDog)
add_subdirectory (Shell Shell)
add_subdirectory (Mobile Mobile)