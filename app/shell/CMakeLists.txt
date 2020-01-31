# 设置debug版本信息 进程生成名字
if(${CMAKE_BUILD_TYPE} MATCHES "debug")
    if (CMAKE_SYSTEM_NAME MATCHES "Darwin")
        set(EXECUTABLE_NAME "Shell_Debug_MacOS")
    elseif (CMAKE_SYSTEM_NAME MATCHES "Linux")
        set(EXECUTABLE_NAME "Shell_Debug_Linux")
    endif()
else()
    if (CMAKE_SYSTEM_NAME MATCHES "Darwin")
        set(EXECUTABLE_NAME "Shell_Release_MaxOS")
    elseif (CMAKE_SYSTEM_NAME MATCHES "Linux")
        set(EXECUTABLE_NAME "Shell_Release_Linux")
    endif()
endif()

include (${PROJECT_SOURCE_DIR}/../cmake/utils.cmake)
#include (${PROJECT_SOURCE_DIR}/cmake/FindNUMA.cmake)
add_cplusplus_standard_support()

set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -fPIC -O0 -Wall -g -ggdb")

# 设置依赖库
set(COMMLIBS msf_agent msf_event msf_sock msf_base numa pthread dl)

# 设置源代码路径
aux_source_directory (. DIR_SRCS)
add_executable(${EXECUTABLE_NAME} ${DIR_SRCS})
add_definitions(-Wno-builtin-macro-redefined)
msf_redefine_file(${EXECUTABLE_NAME})
target_link_libraries(${EXECUTABLE_NAME} ${COMMLIBS})

# https://blog.csdn.net/qq_38410730/article/details/102837401
set(CMAKE_INSTALL_PREFIX /home/luotang.me)           # 安装路径前缀 $ENV{HOME}/usr: /home/xxx/usr
install(TARGETS ${EXECUTABLE_NAME} RUNTIME DESTINATION bin)    # 二进制安装到 /home/luotang.me/bin

#install (FILES * DESTINATION include/msf/base/)          # 安装到 usr/include/msf/base
#INSTALL(DIRECTORY doc/ DESTINATION share/msf)            # 安装项目文档
