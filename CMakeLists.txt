cmake_minimum_required(VERSION 4.2)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_COMPILER clang)
set(CMAKE_C_STANDARD_REQUIRED ON)

project(vulkan-tutorial-c LANGUAGES C)

set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(PROJECT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/src)

set(APP_SOURCE_DIR ${CMAKE_SOURCE_DIR}/src)

file(GLOB_RECURSE SOURCES_C   CONFIGURE_DEPENDS ${APP_SOURCE_DIR}/*.c)

set(SOURCES ${SOURCES_C})

set_source_files_properties(${SOURCES_C}   PROPERTIES LANGUAGE C)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE src)

target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall -Wextra
    $<$<CONFIG:Release>: -O3 -DNDEBUG -march=native>
    # $<$<CONFIG:Debug>: -O0 -g -fsanitize=address -Wpadded>
    $<$<CONFIG:Debug>: -O0 -g -fsanitize=address>
)
target_link_options(${PROJECT_NAME} PRIVATE
    -fuse-ld=lld
    $<$<CONFIG:Debug>: -fsanitize=address>
)

