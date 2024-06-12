cmake_minimum_required (VERSION 3.6)
project(logfanoutd LANGUAGES C VERSION 0.2.4)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
set(CMAKE_C_STANDARD 99)

include(CheckSymbolExists)

find_package(PkgConfig REQUIRED)
pkg_check_modules(MICROHTTPD REQUIRED IMPORTED_TARGET libmicrohttpd)
pkg_check_modules(CHECK REQUIRED IMPORTED_TARGET check)
pkg_check_modules(SYSTEMD IMPORTED_TARGET libsystemd)
find_package(CURL REQUIRED)

set(CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES} ${MICROHTTPD_INCLUDE_DIRS})
set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} ${MICROHTTPD_LIBRARIES})
check_symbol_exists(MHD_create_response_from_fd_at_offset64 microhttpd.h MHD_HAS_RFFO64)


aux_source_directory("src" SOURCES)
add_executable(logfanoutd ${SOURCES})
target_compile_definitions(logfanoutd PRIVATE _FILE_OFFSET_BITS=64 _XOPEN_SOURCE _XOPEN_SOURCE_EXTENDED)
target_include_directories(logfanoutd PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(logfanoutd PkgConfig::MICROHTTPD)
install(TARGETS logfanoutd DESTINATION bin)

if(MHD_HAS_RFFO64)
	target_compile_definitions(logfanoutd PRIVATE MHD_HAS_RFFO64)
endif(MHD_HAS_RFFO64)
if(SYSTEMD_FOUND)
	target_compile_definitions(logfanoutd PRIVATE HAS_SYSTEMD)
	target_link_libraries(logfanoutd PkgConfig::SYSTEMD)
endif(SYSTEMD_FOUND)


enable_testing()

add_executable(test_vpath test/vpath.c src/vpath.c)
add_test(test_vpath test_vpath)
target_include_directories(test_vpath PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(test_vpath PkgConfig::CHECK)

add_executable(test_range test/range.c src/range.c)
add_test(test_range test_range)
target_include_directories(test_range PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(test_range PkgConfig::CHECK)

add_executable(test_logfanoutd test/logfanoutd.c src/range.c src/logfanoutd.c src/vpath.c)
add_test(test_logfanoutd test_logfanoutd)
target_link_libraries(test_logfanoutd PkgConfig::CHECK PkgConfig::MICROHTTPD CURL::libcurl)
target_compile_definitions(test_logfanoutd PRIVATE PROJECT_ROOT="${PROJECT_SOURCE_DIR}" _FILE_OFFSET_BITS=64 _XOPEN_SOURCE _XOPEN_SOURCE_EXTENDED)
if(MHD_HAS_RFFO64)
	target_compile_definitions(test_logfanoutd PRIVATE MHD_HAS_RFFO64)
endif(MHD_HAS_RFFO64)
target_include_directories(test_logfanoutd PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
