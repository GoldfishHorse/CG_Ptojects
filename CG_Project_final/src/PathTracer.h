#pragma once

#include "Defs.h"
#include "Ray.h"

#include "Scene.h"

class PathTracer {
public:
    struct BounceSample {
        Ray ray;
        Vec3f throughput = Vec3f{0.0f};
        bool valid = false;
    };

    PathTracer(
        Scene* scene = nullptr,
        const size_t lightSamples = PathTracer::defaultLightSamples,
        const size_t maxDepth =  PathTracer::defaultMaxDepth,
        const size_t filterFrequency = PathTracer::defaultFilterFrequency,
        const size_t saveImgFrequency = PathTracer::defaultSaveImgFrequency,
        const std::string& saveImgPath = std::string(PathTracer::defaultSaveImgPath)
    );

    ~PathTracer() = default;

    /* 调用 traceRay 对窗口中每一个像素进行光线追踪, 更新 framebuffer */
    void renderScene();
    /* 对单个光线进行追踪 */
    Vec3f traceRay(Ray &ray, size_t depth = 0);
    /* 在光线与模型的相交点处采样下一个光线及其吞吐量权重。 */
    BounceSample nextRaySampling(const Ray &ray, const Intersection &intersection);
    static Vec3f BRDFImportanceSampling(const Vec3f &direction, bool isSpecular = false, float Ns = 0);
    
    /* 对直接光照进行随机采样 */
    Vec3f directLight(const Intersection &intersection);

    const std::vector<Vec3f>& getFrameBufferData() const { 
        return this->scene->frameBuffer.data(); 
    }

    void init(Scene* targetScene)
    {
        this->scene = targetScene;
        this->iterCnt = 0;
        this->bilateralFilter.init(targetScene->width, targetScene->height);
        this->timer.reset();
    }

    DISALLOW_COPY(PathTracer);

public:
    Scene* scene;
    size_t iterCnt;
    BilateralFilter bilateralFilter;
    Timer timer;

    /* 可选参数 */

    size_t lightSamples, maxDepth, filterFrequency, saveImgFrequency;
    std::string saveImgPath;

    /* 可选参数的默认值 */

    /*  directLight 中直接光照的采样数量
        该数值对程序运行速度产生较大影响 */
    static constexpr size_t defaultLightSamples = 3;
    /* traceRay 递归的最大层数
        该数值会对程序运行速度产生较大影响 */
    static constexpr size_t defaultMaxDepth = 6;
    /* 每隔多少次迭代进行一次双边滤波 */
    static constexpr size_t defaultFilterFrequency = 0; /* 0 means not filter */
    /* 每隔多少次迭代保存一次图片 */
    static constexpr size_t defaultSaveImgFrequency = 0; /* 0 means not save */
    /* 保存图片的目标路径 */
    static constexpr const char* defaultSaveImgPath = "./result";
};
