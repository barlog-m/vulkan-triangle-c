cmake_minimum_required(VERSION 4.2)

if(NOT DEFINED CMAKE_C_COMPILER)
    set(CMAKE_C_COMPILER clang CACHE FILEPATH "" FORCE)
endif()
if(NOT DEFINED CMAKE_CXX_COMPILER)
    set(CMAKE_CXX_COMPILER clang++ CACHE FILEPATH "" FORCE)
endif()

project(vulkan-tutorial-c LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

#find_package(glfw3 3.4 REQUIRED)

set(APP_SOURCE_DIR ${CMAKE_SOURCE_DIR}/src)

file(GLOB_RECURSE SOURCES_C CONFIGURE_DEPENDS FOLLOW_SYMLINKS ${APP_SOURCE_DIR}/*.c)

set(SOURCES ${SOURCES_C})

set_source_files_properties(${SOURCES_C}   PROPERTIES LANGUAGE C)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

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

#target_link_libraries(${PROJECT_NAME} PRIVATE glfw)

# Windows + Clang + Debug: app.exe needs clang_rt.asan_dynamic-<arch>.dll
if(WIN32 AND CMAKE_C_COMPILER_ID MATCHES "Clang")
    get_filename_component(_clang_bin_dir "${CMAKE_C_COMPILER}" DIRECTORY)
    file(GLOB _asan_candidates
            "${_clang_bin_dir}/../lib/clang/*/lib/windows/clang_rt.asan_dynamic-*.dll")
    if(_asan_candidates)
        list(GET _asan_candidates 0 _asan_dll)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E "$<IF:$<CONFIG:Debug>,copy_if_different,echo>"
                "${_asan_dll}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
                COMMAND_EXPAND_LISTS
                VERBATIM)
    endif()
endif()
