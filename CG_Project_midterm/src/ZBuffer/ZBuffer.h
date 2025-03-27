#pragma once

#include "defs.h"
#include "Model.h"

/* Z-Buffer 算法的基类 */
class ZBuffer
{
public:
    ZBuffer() = delete;
    ZBuffer(const int windowsWidth, const int windowHeight)
        : width(windowsWidth), height(windowHeight) {};
    virtual ~ZBuffer() = default;

    /** @brief 渲染模型, 填写帧缓冲区。
     * 继承类需覆盖实现此函数。
     * 直接调用 ZBuffer::renderModel 代表无 Z-Buffer 渲染。 */
    virtual void renderModel(const Model &model);
    void init();
    /** @brief 获取帧缓冲区数据 */
    const Color *getFrameBuffer() const { return this->frameBuffer.data(); }

protected:
    int width, height;              /* 窗口宽度和高度 */
    std::vector<FLOAT> zBuffer;     /* 深度缓冲区 */
    std::vector<Color> frameBuffer; /* 帧缓冲区 */
};