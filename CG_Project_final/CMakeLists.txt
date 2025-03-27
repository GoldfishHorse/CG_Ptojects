cmake_minimum_required(VERSION 3.16)
project("project")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /DNDEBUG /DLOG_LEVEL=1 /D_CRT_SECURE_NO_WARNINGS")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od /Zi /DLOG_LEVEL=0")
    add_compile_options(/utf-8)
else()
    message(FATAL_ERROR "Unsupported C++ compiler.")
endif()

if(NOT CMAKE_BUILD_TYPE)
    # Note that in MSVC by default is Debug mode;
    # One shall must add --config Release to switch to Release mode.  
    set(CMAKE_BUILD_TYPE Release)
endif()

add_definitions(-DPROJECT_SOURCE_DIR="${PROJECT_SOURCE_DIR}")

set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_DOCS OFF)
IF (CMAKE_SYSTEM_NAME MATCHES "Linux") 
    set(GLFW_BUILD_WAYLAND OFF)
    set(GLFW_BUILD_X11 ON)
endif()
add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/glfw)
set_target_properties(glfw PROPERTIES FOLDER "lib/glfw")
set_target_properties(uninstall PROPERTIES FOLDER "lib/glfw")
set_target_properties(update_mappings PROPERTIES FOLDER "lib/glfw")


set(tinyxml2_BUILD_TESTING OFF)
add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/tinyxml2)
set_target_properties(tinyxml2 PROPERTIES FOLDER "lib")

add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/glm)
set_target_properties(glm PROPERTIES FOLDER "lib/glm")


file(GLOB SRCS "${CMAKE_SOURCE_DIR}/src/*.cpp" "${CMAKE_SOURCE_DIR}/third_party/glad/glad.c")

add_executable(project ${SRCS})

target_include_directories(project PRIVATE 
    "${CMAKE_SOURCE_DIR}/src" 
    "${CMAKE_SOURCE_DIR}/third_party"
    "${CMAKE_SOURCE_DIR}/third_party/glm"
    "${CMAKE_SOURCE_DIR}/third_party/glfw/include"
    "${CMAKE_SOURCE_DIR}/third_party/stb"
    "${CMAKE_SOURCE_DIR}/third_party/glad"
    "${CMAKE_SOURCE_DIR}/third_party/tinyobjloader"
    "${CMAKE_SOURCE_DIR}/third_party/tinyxml2"
)

find_package(OpenMP REQUIRED)


# 链接库
IF (CMAKE_SYSTEM_NAME MATCHES "Linux") 
    target_link_libraries(project tinyxml2 glfw glm dl GL m)
else()
    target_link_libraries(project tinyxml2 glfw glm)
endif()
target_link_libraries(project OpenMP::OpenMP_CXX ) 