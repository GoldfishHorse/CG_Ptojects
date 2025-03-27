
#pragma once

#include <iostream>
#include <string>
#include <sstream>


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

#define __LOG(level, fmt, args...)                                                                     \
    do                                                                                                 \
    {                                                                                                  \
        printf("[%s %s][%s : %s][%d] [" level "] ", __DATE__, __TIME__, __FILE__, __func__, __LINE__); \
        printf(fmt "\n", ##args);                                                                      \
    } while (0);

#define LIGHT_GRAY(msg) __COLOR_LIGHT_GRAY  msg     __COLOR_NONE
#define GREEN(msg)      __COLOR_GREEN       msg     __COLOR_NONE
#define YELLOW(msg)     __COLOR_YELLOW      msg     __COLOR_NONE
#define RED(msg)        __COLOR_RED         msg     __COLOR_NONE

#define __DEBUG_LEVEL   LIGHT_GRAY("DEBUG")
#define __INFO_LEVEL    GREEN("INFO")
#define __WARNING_LEVEL YELLOW("WARNING")
#define __ERROR_LEVEL   RED("ERROR")

#define LOG_DEBUG(fmt, args...)     __LOG(__DEBUG_LEVEL,    fmt,    ##args)
#define LOG_INFO(fmt, args...)      __LOG(__INFO_LEVEL,     fmt,    ##args)
#define LOG_WARNING(fmt, args...)   __LOG(__WARNING_LEVEL,  fmt,    ##args)
#define LOG_ERROR(fmt, args...)     __LOG(__ERROR_LEVEL,    fmt,    ##args)

#define ASSERT(expr, fmt, args...)                                                                                                              \
    do                                                                                                                                          \
    {                                                                                                                                           \
        if (!(expr))                                                                                                                            \
        {                                                                                                                                       \
            printf("[%s %s][%s : %s][%d] [" __COLOR_RED "ASSERTION FAILS" __COLOR_NONE "] ", __DATE__, __TIME__, __FILE__, __func__, __LINE__); \
            printf(fmt "\n", ##args);                                                                                                           \
        }                                                                                                                                       \
    } while (0)

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>
#include <sal.h>

#define ALERT_MESSAGEBOX(msg)   MessageBox(NULL, msg, "Warning", MB_YESNO | MB_ICONEXCLAMATION);
#define ERROR_MESSAGEBOX(msg)   MessageBox(NULL, msg, "Error", MB_YESNO | MB_ICONSTOP);
#define INFO_MESSAGEBOX(msg)    MessageBox(NULL, msg, "Information", MB_YESNOCANCEL | MB_ICONINFORMATION);

#include <tchar.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <d3dx9math.h>

/* DirectX 9 中常用类型 */
typedef D3DXVECTOR3     Vec3f   ;
typedef Vec3f           Point3f ;
typedef D3DXVECTOR2     Vec2f;
typedef D3DXVECTOR4     Vec4f;
typedef D3DXQUATERNION  Quaternion;
typedef D3DXCOLOR       Color;
typedef D3DLIGHT9       Light;
typedef D3DXMATRIX      Matrix4x4f;
typedef D3DXPLANE       Plane;

/* D3DCOLOR (DWORD) 类型的颜色, 格式为 RGB(A) */
#define D3DCOLOR_WHITE  D3DCOLOR_XRGB(255, 255, 255)
#define D3DCOLOR_BLACK  D3DCOLOR_XRGB(0, 0, 0)
#define D3DCOLOR_RED    D3DCOLOR_XRGB(255, 0, 0)
#define D3DCOLOR_GREEN  D3DCOLOR_XRGB(0, 255, 0)
#define D3DCOLOR_BLUE   D3DCOLOR_XRGB(0, 0, 255)
