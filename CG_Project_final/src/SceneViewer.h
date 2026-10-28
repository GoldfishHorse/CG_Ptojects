#pragma once

#include "Defs.h"
#include "PathTracer.h"

#include <mutex>
#include <atomic>

/* 在路径追踪的过程中进行窗口显示 */
class SceneViewer
{
public:
    SceneViewer(PathTracer *tracer = nullptr)
        : pathTracer(tracer), windowOpened(false)
    {
        if (tracer != nullptr)
            this->init(tracer);
    };

    void init(PathTracer *tracer)
    {
        this->pathTracer = tracer;
        this->w = static_cast<int>(tracer->scene->width);
        this->h = static_cast<int>(tracer->scene->height);
        this->frame.resize(tracer->scene->frameBuffer.size(), Vec3f{0.f});
        this->windowOpened = false;
        return;
    }
    /* 将 this->frame 中的值发送到窗口 */
    void renderFuncCallBack();
    /* 管理整个窗口显示 */
    void viewScene();
    /* 对 frame 中的像素值进行 gamma 矫正 */
    void gammaCorrection(const float gamma = 1.f / 2.2f);
    /* 根据 pathTracer 中的结果更新 frame 的值 */
    void updateFrame();

    DISALLOW_COPY(SceneViewer);

public:
    PathTracer *pathTracer;
    /*  PathTracer::renderScene 和 SceneViewer::renderFuncCallBack 是并发进行的,
        因此设置锁。 */
    std::mutex writeLock;
    /* 写入窗口的图像像素值 */
    std::vector<Vec3f> frame;
    int w, h;
    /* 窗口是否打开: 在窗口关闭后同时终止路径追踪 */
    std::atomic_bool windowOpened;
};
