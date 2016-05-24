cmake_minimum_required (VERSION 2.6)
project(logfanoutd C)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
include(CheckSymbolExists)

find_package(PkgConfig REQUIRED)
pkg_check_modules(MICROHTTPD REQUIRED libmicrohttpd)
pkg_check_modules(CHECK REQUIRED check)
pkg_check_modules(SYSTEMD libsystemd)
find_package(CURL REQUIRED)

set(CMAKE_C_FLAGS "-std=c99 -D_FILE_OFFSET_BITS=64 -D_XOPEN_SOURCE -D_XOPEN_SOURCE_EXTENDED ${MICROHTTPD_CFLAGS} ${CHECK_CFLAGS} ${SYSTEMD_CFLAGS}")

include_directories ("${PROJECT_SOURCE_DIR}/include" ${MICROHTTPD_INCLUDE_DIRS} ${CHECK_INCLUDE_DIRS} ${SYSTEMD_INCLUDE_DIRS})

set(CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES} ${MICROHTTPD_INCLUDE_DIRS})
set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} ${MICROHTTPD_LIBRARIES})
check_symbol_exists(MHD_create_response_from_fd_at_offset64 microhttpd.h MHD_HAS_RFFO64)

if(MHD_HAS_RFFO64)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DMHD_HAS_RFFO64")
endif(MHD_HAS_RFFO64)
if(SYSTEMD_FOUND)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DHAS_SYSTEMD")
endif(SYSTEMD_FOUND)

file(GLOB_RECURSE SOURCES src/*.c)
add_executable(logfanoutd ${SOURCES})
target_link_libraries(logfanoutd ${MICROHTTPD_LIBRARIES} ${SYSTEMD_LIBRARIES})
install(TARGETS logfanoutd DESTINATION bin)

enable_testing()
add_executable(test_vpath test/vpath.c src/vpath.c)
target_link_libraries(test_vpath ${CHECK_LIBRARIES})
add_test(test_vpath test_vpath)

add_executable(test_list test/list.c src/list.c)
target_link_libraries(test_list ${CHECK_LIBRARIES})
add_test(test_list test_list)

add_executable(test_range test/range.c src/range.c)
target_link_libraries(test_range ${CHECK_LIBRARIES})
add_test(test_range test_range)

add_executable(test_logfanoutd test/logfanoutd.c src/range.c src/logfanoutd.c src/vpath.c)
target_link_libraries(test_logfanoutd ${CHECK_LIBRARIES} ${CURL_LIBRARIES} ${MICROHTTPD_LIBRARIES})
set_target_properties(test_logfanoutd PROPERTIES COMPILE_FLAGS -DPROJECT_ROOT='"${PROJECT_SOURCE_DIR}"')
add_test(test_logfanoutd test_logfanoutd)
