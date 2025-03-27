# 定义编译器和编译选项
CC = g++
CFLAGS = -std=c++17 -Wall -I./src -I./src/ZBuffer -O2
LDFLAGS = -ld3d9 -ld3dx9 -lcomdlg32 #-lgdi32

# 目标文件
TARGET_UI = ZBufferUI.exe
TARGET_CMD = ZBufferCMD.exe

# 查找所有的源文件
SOURCES = $(wildcard src/*.cpp src/ZBuffer/*.cpp)
OBJECTS = $(SOURCES:.cpp=.o)

# 单独编译 mainUI.cpp 和 mainCMD.cpp 
UI_OBJECTS = $(filter-out src/mainCMD.o src/mainUI.o, $(SOURCES:.cpp=.o)) src/mainUI.o 
CMD_OBJECTS = $(filter-out src/mainCMD.o src/mainUI.o, $(SOURCES:.cpp=.o)) src/mainCMD.o

# 默认目标
all: $(TARGET_UI) $(TARGET_CMD)

# 链接 UI 目标文件 
$(TARGET_UI): $(UI_OBJECTS) 
	$(CC) -o $@ $^ $(LDFLAGS)

# 链接 CMD 目标文件 
$(TARGET_CMD): $(CMD_OBJECTS) 
	$(CC) -o $@ $^ $(LDFLAGS)

# 编译源文件
%.o: %.cpp
	$(CC) -c -o $@ $< $(CFLAGS)

# 清理生成文件
clean:
	rm -rf $(UI_OBJECTS) $(CMD_OBJECTS) $(TARGET_UI) $(TARGET_CMD)

.PHONY: all clean
