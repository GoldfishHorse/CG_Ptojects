cmake_minimum_required(VERSION 3.20)
project(cpu_path_tracer VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(CPU_PATH_TRACER_ENABLE_OPENMP "Enable OpenMP pixel parallelism" ON)

set(THIRD_PARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party")
foreach(dependency glfw glm stb tinyobjloader tinyxml2)
    if(NOT EXISTS "${THIRD_PARTY_DIR}/${dependency}")
        message(FATAL_ERROR
            "Missing third_party/${dependency}. Run build.ps1 first or clone the dependency manually.")
    endif()
endforeach()

set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory("${THIRD_PARTY_DIR}/glfw" EXCLUDE_FROM_ALL)

add_library(tinyxml2 STATIC
    "${THIRD_PARTY_DIR}/tinyxml2/tinyxml2.cpp")
target_include_directories(tinyxml2 PUBLIC
    "${THIRD_PARTY_DIR}/tinyxml2")

find_package(OpenGL REQUIRED)
if(CPU_PATH_TRACER_ENABLE_OPENMP)
    find_package(OpenMP QUIET)
endif()

file(GLOB PROJECT_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
add_executable(cpu_path_tracer
    ${PROJECT_SOURCES}
    "${THIRD_PARTY_DIR}/glad/glad.c")

target_include_directories(cpu_path_tracer PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${THIRD_PARTY_DIR}"
    "${THIRD_PARTY_DIR}/glad"
    "${THIRD_PARTY_DIR}/glm"
    "${THIRD_PARTY_DIR}/stb"
    "${THIRD_PARTY_DIR}/tinyobjloader")

target_compile_definitions(cpu_path_tracer PRIVATE
    PROJECT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    $<$<CONFIG:Debug>:LOG_LEVEL=0>
    $<$<NOT:$<CONFIG:Debug>>:LOG_LEVEL=1>)

if(MSVC)
    target_compile_options(cpu_path_tracer PRIVATE /W4 /permissive- /utf-8)
    target_compile_definitions(cpu_path_tracer PRIVATE _CRT_SECURE_NO_WARNINGS NOMINMAX)
else()
    target_compile_options(cpu_path_tracer PRIVATE -Wall -Wextra -Wpedantic)
endif()

target_link_libraries(cpu_path_tracer PRIVATE
    glfw tinyxml2 OpenGL::GL)

if(OpenMP_CXX_FOUND)
    target_link_libraries(cpu_path_tracer PRIVATE OpenMP::OpenMP_CXX)
    message(STATUS "OpenMP enabled")
else()
    message(STATUS "OpenMP not found; building a single-threaded renderer")
endif()

set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" PROPERTY VS_STARTUP_PROJECT cpu_path_tracer)
