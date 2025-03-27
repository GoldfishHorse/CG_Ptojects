#pragma once

#include "Defs.h"
#include "Ray.h"
#include "Geometry.h"
#include "Camera.h"
#include "Model.h"

#include <string>
#include <vector>

/*  场景类, 包含单个模型以及
    对该场景进行渲染所必须的相机、帧缓存、窗口大小等信息 */
class Scene {
public:
    Scene() = default;
    Scene(const size_t width, const size_t height, const std::string& name = "", const Vec3f& ambient = defaultAmbient)
        : width(width), height(height), name(name), ambient(ambient), frameBuffer(width, height)
        {}

    float getAspect () const 
        { return static_cast<float>(width) / static_cast<float>(height); }
    Ray getRay(const size_t x, const size_t y, const float noiseMap = 0.5f ) const;
    bool hit(Ray &ray, Intersection &intersection) const 
        { return this->model.hit(ray, intersection); }
    void initFrameBuffer() {
        this->frameBuffer.init(width, height);
        return;
    }
    void init(const size_t width, const size_t height, const Vec3f& ambient = defaultAmbient)
    {
        this->width = width;
        this->height = height;
        this->ambient = ambient;
        this->initFrameBuffer();
        this->camera.aspect = this->getAspect();
    }

public:
    /* 场景模型 */
    Model model;
    /* 相机 */
    PerspectiveCamera camera;
    /* 窗口宽和高 */
    size_t width, height;
    /* 场景名称 */
    std::string name;
    /* 场景的环境光 */
    Vec3f ambient;
    /* 帧缓存, 在路径追踪的规程中更新 */
    Image frameBuffer;
    
    static constexpr size_t defaultWidth = 1280;
    static constexpr size_t defaultHeight = 720;
    static constexpr Vec3f defaultAmbient = Vec3f {0};
};