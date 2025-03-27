@echo off
setlocal enabledelayedexpansion

rem 定义编译器和编译选项
set "CC=g++"
set "SRC_DIR=%~dp0src"
set "CFLAGS=-std=c++17 -Wall -I%SRC_DIR% -I%SRC_DIR%\ZBuffer -O2"
set "LDFLAGS=-ld3d9 -ld3dx9 -lcomdlg32" rem -lgdi32

rem 目标文件
set "TARGET_UI=ZBufferUI.exe"
set "TARGET_CMD=ZBufferCMD.exe"

rem 查找所有的源文件
set "SOURCES="
for %%f in (%SRC_DIR%\*.cpp %SRC_DIR%\ZBuffer\*.cpp) do (
    set "SOURCES=!SOURCES! %%f"
)

rem 单独编译 mainUI.cpp 和 mainCMD.cpp 
set "UI_OBJECTS="
set "CMD_OBJECTS="
for %%f in (%SOURCES%) do (
    set "OBJ_FILE=%%~dpnf.o"
    if "%%f" neq "%SRC_DIR%\mainCMD.cpp" if "%%f" neq "%SRC_DIR%\mainUI.cpp" (
        set "UI_OBJECTS=!UI_OBJECTS! !OBJ_FILE!"
        set "CMD_OBJECTS=!CMD_OBJECTS! !OBJ_FILE!"
    )
)
set "UI_OBJECTS=!UI_OBJECTS! %SRC_DIR%\mainUI.o"
set "CMD_OBJECTS=!CMD_OBJECTS! %SRC_DIR%\mainCMD.o"

rem 默认目标
if "%1" == "" (
    set "param=all"
) else (
    set "param=%1"
)
echo Compiling with parameter: %param%

if "%param%" == "all" goto build_all
if "%param%" == "clean" goto clean
goto end

:build_all
call :compile_sources
call :build_ui
call :build_cmd
goto end

rem 编译源文件
:compile_sources
for %%f in (%SOURCES%) do (
    set "OBJ_FILE=%%~dpnf.o"
    echo %CC% -c -o !OBJ_FILE! %%f %CFLAGS%
    %CC% -c -o !OBJ_FILE! %%f %CFLAGS%
)
goto end

rem 链接 UI 目标文件 
:build_ui
echo Building UI executable...
echo %CC% -o %TARGET_UI% %UI_OBJECTS% %LDFLAGS%
%CC% -o %TARGET_UI% %UI_OBJECTS% %LDFLAGS%
goto end

rem 链接 CMD 目标文件 
:build_cmd
echo Building CMD executable...
echo %CC% -o %TARGET_CMD% %CMD_OBJECTS% %LDFLAGS%
%CC% -o %TARGET_CMD% %CMD_OBJECTS% %LDFLAGS%
goto end

rem 清理生成文件
:clean
echo Cleaning up...
for %%f in (%SOURCES%) do (
    set "OBJ_FILE=%%~dpnf.o"
    if exist !OBJ_FILE! del !OBJ_FILE!
)
if exist %TARGET_UI% del %TARGET_UI%
if exist %TARGET_CMD% del %TARGET_CMD%
goto end

:end
echo Done.
endlocal
