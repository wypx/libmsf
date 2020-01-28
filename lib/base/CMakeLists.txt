aux_source_directory(. DIR_SRCS)
add_library(msf_base STATIC ${DIR_SRCS})

msf_redefine_file(msf_base)
add_definitions(-Wno-builtin-macro-redefined)

#install(TARGETS lib LIBRARY DESTINATION lib)            # 安装到 usr/local/lib
#install(FILES * DESTINATION include/msf/base/)          # 安装到 usr/include/msf/base