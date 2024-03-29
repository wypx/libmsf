set(WATCHDOG_SERVER_VERSION 0.1)

execute_process(COMMAND git log --pretty=format:%h -1 WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} OUTPUT_VARIABLE GITVER)
add_definitions(-DVERSION_TAG=${AGENT_SERVER_VERSION}-${GITVER})

# 设置debug版本信息 进程生成名字
if(${CMAKE_BUILD_TYPE} MATCHES "debug")
    file(MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX}/Debug) 
    if (CMAKE_SYSTEM_NAME MATCHES "Darwin")
        file(MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX}/Darwin) 
    elseif (CMAKE_SYSTEM_NAME MATCHES "Linux")
        file(MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX}/Linux) 
    endif()
else()
    file(MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX}/Release) 
    if (CMAKE_SYSTEM_NAME MATCHES "Darwin")
        file(MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX}/Darwin) 
    elseif (CMAKE_SYSTEM_NAME MATCHES "Linux")
        file(MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX}/Linux) 
    endif()
endif()
set(EXECUTABLE_NAME "Watchdog")

include (${PROJECT_SOURCE_DIR}/../cmake/utils.cmake)
#include (${PROJECT_SOURCE_DIR}/cmake/FindNUMA.cmake)
add_cplusplus_standard_support()

set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -g -ggdb -wall -DGFLAGS_NS=${GFLAGS_NS}")
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} ${DEFINE_CLOCK_GETTIME} -DGFLAGS_NS=${GFLAGS_NS}")

set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

# 设置依赖库
# brpc.a
# gflags.a
# leveldb.a
# ssl.a
# crypto.a
# snappy.a
# z.a
set(COMMLIBS MSF
    numa.a
    uring.a
    snappy.a
    protobuf.a
    protoc.a
    uuid
    pthread
    dl)

# 设置源代码路径
aux_source_directory (. DIR_SRCS)
# aux_source_directory (../proto DIR_SRCS)
add_executable(${EXECUTABLE_NAME} ${DIR_SRCS})
add_definitions(-Wno-builtin-macro-redefined)
msf_redefine_file(${EXECUTABLE_NAME})
#set_target_properties(${EXECUTABLE_NAME} PROPERTIES VERSION ${WATCHDOG_SERVER_VERSION})
target_link_libraries(${EXECUTABLE_NAME} ${COMMLIBS})

# https://blog.csdn.net/qq_38410730/article/details/102837401
install(TARGETS ${EXECUTABLE_NAME} RUNTIME DESTINATION bin)    # 二进制安装到 /home/luotang.me/bin

#install (FILES * DESTINATION include/msf/base/)          # 安装到 usr/include/msf/base
#INSTALL(DIRECTORY doc/ DESTINATION share/msf)            # 安装项目文档