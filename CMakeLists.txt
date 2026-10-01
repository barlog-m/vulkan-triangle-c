cmake_minimum_required(VERSION 4.2)

if(WIN32 AND NOT DEFINED CMAKE_MSVC_RUNTIME_LIBRARY)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
endif()

if(NOT DEFINED CMAKE_C_COMPILER)
    set(CMAKE_C_COMPILER clang CACHE FILEPATH "" FORCE)
endif()
if(NOT DEFINED CMAKE_CXX_COMPILER)
    set(CMAKE_CXX_COMPILER clang++ CACHE FILEPATH "" FORCE)
endif()

project(vulkan-triangle-c LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -march=native")

find_package(Vulkan REQUIRED)

add_subdirectory(vendor/sdl EXCLUDE_FROM_ALL)

add_library(vulkan_memory_allocator INTERFACE)
target_include_directories(vulkan_memory_allocator SYSTEM INTERFACE vendor/vulkan_memory_allocator)

file(GLOB_RECURSE CGLM_SOURCES CONFIGURE_DEPENDS vendor/cglm/src/*.c)
add_library(cglm STATIC ${CGLM_SOURCES})
target_include_directories(cglm SYSTEM PUBLIC vendor/cglm/include)

set(APP_SOURCE_DIR ${CMAKE_SOURCE_DIR}/src)

file(GLOB_RECURSE SOURCES_C CONFIGURE_DEPENDS FOLLOW_SYMLINKS ${APP_SOURCE_DIR}/*.c)
file(GLOB_RECURSE SOURCES_CPP CONFIGURE_DEPENDS FOLLOW_SYMLINKS ${APP_SOURCE_DIR}/*.cpp)

set(SOURCES ${SOURCES_C} ${SOURCES_CPP})

set_source_files_properties(${SOURCES_C}   PROPERTIES LANGUAGE C)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/vulkan_memory_allocator
)

target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall -Wextra
    $<$<CONFIG:Release>: -O3 -DNDEBUG -march=native>
    # $<$<CONFIG:Debug>: -O0 -g -fsanitize=address -Wpadded>
    $<$<CONFIG:Debug>: -O0 -g -fsanitize=address>
)
target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<AND:$<PLATFORM_ID:Windows>,$<CXX_COMPILER_ID:Clang>,$<CONFIG:Debug>>:_ITERATOR_DEBUG_LEVEL=0>
)
target_link_options(${PROJECT_NAME} PRIVATE
    -fuse-ld=lld
    $<$<CONFIG:Debug>: -fsanitize=address>
)

target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3)
target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan)
target_link_libraries(${PROJECT_NAME} PRIVATE vulkan_memory_allocator)
target_link_libraries(${PROJECT_NAME} PRIVATE cglm)

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
