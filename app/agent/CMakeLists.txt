# 设置链接库搜索目录
link_directories(
    /home/share/tomato/mod/libmsf/build/lib/msf_base/
    /home/share/tomato/mod/libmsf/build/lib/msf_sock/
    /home/share/tomato/mod/libmsf/build/lib/msf_event/
    /home/share/tomato/mod/libmsf/build/app/msf_agent/
    /home/share/tomato/mod/libmsf/build/lib/
)

# c++17
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++17 -fPIC -O3 -Wall -g -ggdb")

add_subdirectory (client msf_agent)
add_subdirectory (server AgentServer)