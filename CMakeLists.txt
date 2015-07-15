cmake_minimum_required (VERSION 2.6)
project(logfanoutd C)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

find_package(PkgConfig REQUIRED)
pkg_check_modules(MICROHTTPD REQUIRED libmicrohttpd)
pkg_check_modules(CHECK REQUIRED check)
find_package(CURL REQUIRED)

set(CMAKE_C_FLAGS "-std=c99 -D_FILE_OFFSET_BITS=64 -D_XOPEN_SOURCE -D_XOPEN_SOURCE_EXTENDED ${MICROHTTPD_CFLAGS} ${CHECK_CFLAGS}")

include_directories ("${PROJECT_SOURCE_DIR}/include" ${MICROHTTPD_INCLUDE_DIRS} ${CHECK_INCLUDE_DIRS})

file(GLOB_RECURSE SOURCES src/*.c)
add_executable(logfanoutd ${SOURCES})
target_link_libraries(logfanoutd ${MICROHTTPD_LIBRARIES})
install(TARGETS logfanoutd DESTINATION bin)

enable_testing()
add_executable(test_vpath test/vpath.c src/vpath.c)
target_link_libraries(test_vpath ${CHECK_LIBRARIES})
add_test(test_vpath test_vpath)

add_executable(test_range test/range.c src/range.c)
target_link_libraries(test_range ${CHECK_LIBRARIES})
add_test(test_range test_range)

add_executable(test_logfanoutd test/logfanoutd.c src/range.c src/logfanoutd.c src/vpath.c)
target_link_libraries(test_logfanoutd ${CHECK_LIBRARIES} ${CURL_LIBRARIES} ${MICROHTTPD_LIBRARIES})
set_target_properties(test_logfanoutd PROPERTIES COMPILE_FLAGS -DPROJECT_ROOT='"${PROJECT_SOURCE_DIR}"')
add_test(test_logfanoutd test_logfanoutd)
