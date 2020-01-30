aux_source_directory(. DIR_SRCS)
add_library(msf_encrypt STATIC ${DIR_SRCS})
msf_redefine_file(msf_encrypt)
add_definitions(-Wno-builtin-macro-redefined)

install(TARGETS msf_encrypt LIBRARY DESTINATION lib)
#install(FILES * DESTINATION include/msf/base/)