cmake_minimum_required (VERSION 2.6)
project(logfanoutd C)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

find_library(MICROHTTPD microhttpd)
find_package(Check REQUIRED)
find_package(CURL REQUIRED)

set(CMAKE_C_FLAGS "-std=c99 -D_FILE_OFFSET_BITS=64 -D_XOPEN_SOURCE -D_XOPEN_SOURCE_EXTENDED")

include_directories ("${PROJECT_SOURCE_DIR}/include")

file(GLOB_RECURSE SOURCES src/*.c)
add_executable(logfanoutd ${SOURCES})
target_link_libraries(logfanoutd ${MICROHTTPD})

enable_testing()
add_executable(test_vpath test/vpath.c src/vpath.c)
target_link_libraries(test_vpath ${CHECK_LIBRARIES})
add_test(test_vpath test_vpath)

add_executable(test_range test/range.c src/range.c)
target_link_libraries(test_range ${CHECK_LIBRARIES})
add_test(test_range test_range)

add_executable(test_logfanoutd test/logfanoutd.c src/range.c src/logfanoutd.c src/vpath.c)
target_link_libraries(test_logfanoutd ${CHECK_LIBRARIES} ${CURL_LIBRARIES} ${MICROHTTPD})
set_target_properties(test_logfanoutd PROPERTIES COMPILE_FLAGS -DPROJECT_ROOT='"${PROJECT_SOURCE_DIR}"')
add_test(test_logfanoutd test_logfanoutd)
