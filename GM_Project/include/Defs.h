#pragma once

#if defined(__GNUC__)
    #if __cplusplus < 201703L
        #error C++ 17 required!
    #endif
#elif defined (_MSC_VER)
    #if _MSVC_LANG < 201703L
        #error C++ 17 required!
    #endif
#endif 

#include <cstdio>

/* --------------------------------------------- */
/* 基于宏的日志系统 */

#define __COLOR_NONE            "\033[m"
#define __COLOR_RED             "\033[0;32;31m"
#define __COLOR_LIGHT_RED       "\033[1;31m"
#define __COLOR_GREEN           "\033[0;32;32m"
#define __COLOR_LIGHT_GREEN     "\033[1;32m"
#define __COLOR_BLUE            "\033[0;32;34m"
#define __COLOR_LIGHT_BLUE      "\033[1;34m"
#define __COLOR_DARY_GRAY       "\033[1;30m"
#define __COLOR_CYAN            "\033[0;36m"
#define __COLOR_LIGHT_CYAN      "\033[1;36m"
#define __COLOR_PURPLE          "\033[0;35m"
#define __COLOR_LIGHT_PURPLE    "\033[1;35m"
#define __COLOR_BROWN           "\033[0;33m"
#define __COLOR_YELLOW          "\033[1;33m"
#define __COLOR_LIGHT_GRAY      "\033[0;37m"
#define __COLOR_WHITE           "\033[1;37m"

#ifndef __func__
#ifndef __GNUC__
#define __PRETTY_FUNCTION__ __FUNCTION__
#endif
#define __func__ __PRETTY_FUNCTION__
#endif

#define LOG_LEVEL_DEBUG     0 
#define LOG_LEVEL_INFO      1 
#define LOG_LEVEL_WARNING   2 
#define LOG_LEVEL_ERROR     3

#ifndef LOG_LEVEL 
#ifndef NDEBUG
#define LOG_LEVEL LOG_LEVEL_DEBUG 
#else 
#define LOG_LEVEL LOG_LEVEL_INFO
#endif
#endif

#define __PRINT_DETAIL_MSG(identifier) \
    printf("[%s %s][%s : %s][%d] [" identifier "] ", __DATE__, __TIME__, __FILE__, __func__, __LINE__)

#define __PRINT_IDENTIFIER(identifier) \
    printf("[" identifier "] ")

#if LOG_LEVEL <= LOG_LEVEL_DEBUG
#define __PRINT_MSG(identifier) __PRINT_DETAIL_MSG(identifier)
#else
#define __PRINT_MSG(identifier) __PRINT_IDENTIFIER(identifier)
#endif


#define __LOG(level, fmt, ...)                  \
    do                                          \
    {                                           \
        __PRINT_MSG(level);                     \
        printf(fmt "\n", ##__VA_ARGS__);        \
    } while (0);

#define __WRAP_LIGHT_GRAY(msg) __COLOR_LIGHT_GRAY  msg     __COLOR_NONE
#define __WRAP_GREEN(msg)      __COLOR_GREEN       msg     __COLOR_NONE
#define __WRAP_YELLOW(msg)     __COLOR_YELLOW      msg     __COLOR_NONE
#define __WRAP_RED(msg)        __COLOR_RED         msg     __COLOR_NONE


#define __DEBUG_LEVEL       __WRAP_LIGHT_GRAY("DEBUG")
#define __INFO_LEVEL        __WRAP_GREEN("INFO")
#define __WARNING_LEVEL     __WRAP_YELLOW("WARNING")
#define __ERROR_LEVEL       __WRAP_RED("ERROR")
#define __ASSERTION_FAILS   __WRAP_RED("ASSERTION FAILS")


#if LOG_LEVEL <= LOG_LEVEL_DEBUG
#define LOG_DEBUG(fmt, ...)     __LOG(__DEBUG_LEVEL,    fmt,    ##__VA_ARGS__)
#else
#define LOG_DEBUG(fmt, ...)
#endif

#if LOG_LEVEL <= LOG_LEVEL_INFO
#define LOG_INFO(fmt, ...)      __LOG(__INFO_LEVEL,     fmt,    ##__VA_ARGS__)
#else
#define LOG_INFO(fmt, ...)
#endif

#if LOG_LEVEL <= LOG_LEVEL_WARNING
#define LOG_WARNING(fmt, ...)   __LOG(__WARNING_LEVEL,  fmt,    ##__VA_ARGS__)
#else 
#define LOG_WARNING(fmt, ...)
#endif

#if LOG_LEVEL <= LOG_LEVEL_ERROR
#define LOG_ERROR(fmt, ...)     __LOG(__ERROR_LEVEL,    fmt,    ##__VA_ARGS__)
#else
#define LOG_ERROR(fmt, ...)
#endif

// #if LOG_LEVEL <= LOG_LEVEL_DEBUG
#define ASSERT(expr, fmt, ...)                              \
    do                                                      \
    {                                                       \
        if (!(expr))                                        \
        {                                                   \
            __PRINT_MSG(__ASSERTION_FAILS);                 \
            printf(fmt "\n", ##__VA_ARGS__);                \
            exit(-1);                                       \
        }                                                   \
    } while (0)
// #else
// #define ASSERT(expr, fmt, ...)
// #endif

/* --------------------------------------------- */
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>

/* 为 glm 中的类型定义别名
    另外部分关于 array 的别名定义在了 Utils.h 中 */

template <size_t dim, typename T>
using Point = glm::vec<dim, T>;

template <size_t dim, typename T>
using Vec = glm::vec<dim, T>;

template <size_t dim>
using Pointf = Point<dim, float>;
template <size_t dim>
using Vecf = Vec<dim, float>;
using Point2f = Point<2, float>;
using Point3f = Point<3, float>;
using Point4f = Point<4, float>;


using Mat3f = glm::mat3;
using Mat4f = glm::mat4;
using Vec2f = glm::vec2;
using Vec3f = glm::vec3;
using Vec4f = glm::vec4;

/* 使用 nanoflann 库的 KD-Tree 实现
    见 Utils.h 和 Utils.cpp 中的 Checker::surfaceIsSelfIntersection 函数 */
#ifndef USING_NANOFLANN
#define USING_NANOFLANN true
#endif

/*  设置指定初始化器 (designated initializers), 指定初始值设定项
    C++20 标准, gcc >= 8 支持该扩展。
    https://gcc.gnu.org/onlinedocs/gcc/Designated-Inits.html

    编译命令用的是 C++17 标准, MSVC 会报错:
    error C7555: use of designated initializers requires at least '/std:c++20'

    ("SFINIT" == struct field init)
    https://stackoverflow.com/questions/5440611/how-to-rewrite-c-struct-designated-initializers-to-c89-resp-msvc-c-compiler */
#if defined(__cpp_designated_initializers) || ( defined(__GNUC__) && (__GNUC__ >= 8) )
#define SFINIT(f, ...) f = __VA_ARGS__
#else
#define SFINIT(f, ...) __VA_ARGS__
#endif


/* GUI 窗口的宽和高 
    见 GLProgrram.cpp */
#define WINDOW_WIDTH 1600
#define WINDOW_HEIGHT 1200