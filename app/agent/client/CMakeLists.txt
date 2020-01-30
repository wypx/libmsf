aux_source_directory(. DIR_SRCS)
add_library(msf_agent STATIC ${DIR_SRCS})

msf_redefine_file(msf_agent)
add_definitions(-Wno-builtin-macro-redefined)

# 设置链接库搜索目录
link_directories(
    ${PROJECT_SOURCE_DIR}/../lib/msf_base/
    ${PROJECT_SOURCE_DIR}/../lib/msf_sock/
    ${PROJECT_SOURCE_DIR}/../lib/msf_event/
    ${PROJECT_SOURCE_DIR}/agent/client/
    ${PROJECT_SOURCE_DIR}/agent/
)

install(TARGETS msf_agent LIBRARY DESTINATION lib)       # 安装到 /home/luotang.me/lib
#install(FILES * DESTINATION include/msf/base/)          # 安装到 usr/include/msf/base

