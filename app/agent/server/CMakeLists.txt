set(AGENT_SERVER_VERSION 0.1)

execute_process(COMMAND git log --pretty=format:%h -1 WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} OUTPUT_VARIABLE GITVER)
add_definitions(-DVERSION_TAG=${AGENT_SERVER_VERSION}-${GITVER})

# https://blog.csdn.net/qq_38410730/article/details/102837401
set(CMAKE_INSTALL_PREFIX /home/luotang.me)           # 安装路径前缀 $ENV{HOME}/usr: /home/xxx/usr

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
set(EXECUTABLE_NAME "AgentServer")

include (${PROJECT_SOURCE_DIR}/../cmake/utils.cmake)
#include (${PROJECT_SOURCE_DIR}/cmake/FindNUMA.cmake)
add_cplusplus_standard_support()

set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -g -ggdb -wall")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

# 设置依赖库
set(COMMLIBS msf_agent msf_proto msf_event msf_sock msf_base protobuf protoc numa pthread dl)

# 设置源代码路径
aux_source_directory (. DIR_SRCS)
add_executable(${EXECUTABLE_NAME} ${DIR_SRCS})
add_definitions(-Wno-builtin-macro-redefined)
msf_redefine_file(${EXECUTABLE_NAME})
#https://www.cnblogs.com/wangjian8888/p/10710690.html
set_target_properties(${EXECUTABLE_NAME} PROPERTIES VERSION ${AGENT_SERVER_VERSION})
#set_target_properties(${EXECUTABLE_NAME} PROPERTIES VERSION ${AGENT_SERVER_VERSION} SOVERSION 1) 
target_link_libraries(${EXECUTABLE_NAME} ${COMMLIBS})

install(TARGETS ${EXECUTABLE_NAME} RUNTIME DESTINATION bin)    # 二进制安装到 /home/luotang.me/bin

set(AGENT_SERVER_CONF ${PROJECT_SOURCE_DIR}/agent/conf/AgentServer.conf)
install(FILES ${AGENT_SERVER_CONF} DESTINATION conf)
#install (FILES * DESTINATION include/msf/base/)          # 安装到 usr/include/msf/base
#INSTALL(DIRECTORY doc/ DESTINATION share/msf)            # 安装项目文档