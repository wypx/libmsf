cmake_minimum_required(VERSION 3.10.2 FATAL_ERROR)
cmake_policy(VERSION 3.10.2)
project(MSF_LIB_PROJECT)

# 加入一个配置头文件,用于处理 CMake 对源码的设置
configure_file (
    "${PROJECT_SOURCE_DIR}/conf/config.h.in"
    "${PROJECT_SOURCE_DIR}/conf/config.h"
)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/../cmake)
include (${CMAKE_MODULE_PATH}/FindNUMA.cmake)
include (${CMAKE_MODULE_PATH}/utils.cmake)
#include (${CMAKE_MODULE_PATH}/FindProtobuf.cmake)

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++17" COMPILER_SUPPORTS_CXX17)
CHECK_CXX_COMPILER_FLAG("-std=c++20" COMPILER_SUPPORTS_CXX20)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O3 -Wall -Wl,-rpath=/tools/lib64 -Wl,#--dynamic-linker=/tools/lib/ld-2.17.so")

# -std=gnu++11 与 -std=c++11 有什么区别？
# https://blog.csdn.net/juan202/article/details/79968898?utm_source=blogxgwz4
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++17 -fPIC -O3 -Wall -g -ggdb")

# 宏定义
add_definitions(-DMSF_UNIX_IPC)

# 设置包含的头文件目录
# 其他方式: set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I${CMAKE_CURRENT_SOURCE_DIR}")
include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/conf
    ${CMAKE_CURRENT_SOURCE_DIR}/base
    ${CMAKE_CURRENT_SOURCE_DIR}/network
    ${CMAKE_CURRENT_SOURCE_DIR}/network/sock
    ${CMAKE_CURRENT_SOURCE_DIR}/network/event
    ${CMAKE_CURRENT_SOURCE_DIR}/network/frpc
    ${CMAKE_CURRENT_SOURCE_DIR}/network/fraft
    ${CMAKE_CURRENT_SOURCE_DIR}/network/turn
    ${CMAKE_CURRENT_SOURCE_DIR}/network/stun
    ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/baidu/incubator-brpc/src/
)

#set(CMAKE_VERBOSE_MAKEFILE ON)
# 是否使开启debug
# 执行cmake -DCMAKE_BUILD_TYPE=Debug（或Release)
# Debug版会使用参数-g；Release版使用-O3 –DNDEBUG
# 在执行cmake时增加参数即可，例如cmake -DDEBUG_mode=ON

option(DEBUG_mode "ON for debug or OFF for release" ON)
if(DEBUG_mode)
    message(STATUS "Current project compile in debug mode.") 
    add_definitions(-DDEBUG)
    SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -fPIC -O0 -Wall -g -ggdb")
else()
    message(STATUS "Current project compile in release mode.") 
    SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -fPIC -O3 -Wall")
endif()

aux_source_directory(base DIR_SRCS)
aux_source_directory(network/sock DIR_SRCS)
aux_source_directory(network/event DIR_SRCS)
aux_source_directory(network/frpc DIR_SRCS)
aux_source_directory(network/fraft DIR_SRCS)
#aux_source_directory(protocol/turn DIR_SRCS)
aux_source_directory(protocol/stun DIR_SRCS)

add_library(MSF STATIC ${DIR_SRCS})


msf_redefine_file(MSF)
add_definitions(-Wno-builtin-macro-redefined)

#install(TARGETS MSF LIBRARY DESTINATION lib)
#install(FILES * DESTINATION include/msf/)

INSTALL(
  TARGETS MSF
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  COMPONENT library
)

# $ENV{HOME}/usr
set(CMAKE_INSTALL_PREFIX /home/luotang.me)                      # 安装路径前缀 /home/xxx/usr
#INSTALL(DIRECTORY doc/ DESTINATION share/msf)                  # 安装项目文档
#INSTALL(TARGETS ${EXECUTABLE_NAME} RUNTIME DESTINATION lib)    # 二进制安装到 usr/bin