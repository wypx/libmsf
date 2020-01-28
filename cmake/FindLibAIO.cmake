find_path(LIBAIO_INCLUDE_DIR NAMES libaio.h)
mark_as_advanced(LIBAIO_INCLUDE_DIR)

find_library(LIBAIO_LIBRARY NAMES aio)
mark_as_advanced(LIBAIO_LIBRARY)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(
  LIBAIO
  REQUIRED_VARS LIBAIO_LIBRARY LIBAIO_INCLUDE_DIR)

if(LIBAIO_FOUND)
    set(LIBAIO_LIBRARIES ${LIBAIO_LIBRARY})
    set(LIBAIO_INCLUDE_DIRS ${LIBAIO_INCLUDE_DIR})
    MESSAGE(STATUS "LibAIO library is found.")
else ()
    SET(NUMA_FOUND FALSE)
    MESSAGE(STATUS "WARNING: Numa library not found.")
    MESSAGE(STATUS "Try: 'sudo yum install libaio' (or sudo apt-get install libaio)")
endif()

