cmake_minimum_required(VERSION 3.16)
project("project")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(MSVC)
    add_compile_options(/utf-8)
endif()

if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    if (MSVC)
        add_compile_options(/O2)
    else()
        add_compile_options(-O2)
    endif()
    add_definitions(-DLOG_LEVEL=1)
endif()

add_definitions(-DPROJECT_SOURCE_DIR="${PROJECT_SOURCE_DIR}")

set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_DOCS OFF)
add_subdirectory(${CMAKE_SOURCE_DIR}/external/glfw)
set_target_properties(glfw PROPERTIES FOLDER "lib/glfw")
set_target_properties(uninstall PROPERTIES FOLDER "lib/glfw")
set_target_properties(update_mappings PROPERTIES FOLDER "lib/glfw")


add_subdirectory(${CMAKE_SOURCE_DIR}/external/imgui)
set_target_properties(imgui PROPERTIES FOLDER "lib")
add_subdirectory(${CMAKE_SOURCE_DIR}/external/glm)
set_target_properties(glm PROPERTIES FOLDER "lib/glm")


file(GLOB SRCS "${CMAKE_SOURCE_DIR}/src/*.cpp" "${CMAKE_SOURCE_DIR}/src/*.c")

add_executable(project ${SRCS})

target_include_directories(project PRIVATE 
    "${CMAKE_SOURCE_DIR}/include" 
    "${CMAKE_SOURCE_DIR}/external"
    "${CMAKE_SOURCE_DIR}/external/glm"
    "${CMAKE_SOURCE_DIR}/external/eigen"
    "${CMAKE_SOURCE_DIR}/external/nanoflann/include"
)

# 链接库
if(LINUX)
    target_link_libraries(project imgui glfw glm dl GL m)
else()
    # target_link_directories(project PRIVATE "./lib")
    target_link_libraries(project imgui glfw glm)
endif()

file (COPY imgui.ini DESTINATION ${CMAKE_CURRENT_BINARY_DIR})