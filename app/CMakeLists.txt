cmake_minimum_required(VERSION 3.15 FATAL_ERROR)      # 指定 cmake 的最小版本
cmake_policy(VERSION 3.15)
project(MSF_LIB_PROJECT)                                    # 设置项目名称

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib/
)

# c++17
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++17 -fPIC -O3 -Wall -g -ggdb")

add_subdirectory (agent msf_agent_server)
add_subdirectory (guard msf_guard)
add_subdirectory (shell msf_shell)
add_subdirectory (Mobile mobile)