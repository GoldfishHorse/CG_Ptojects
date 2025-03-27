/* 各种杂项功能的实现 */

#pragma once
#include "Defs.h"

#include <chrono>
#include <initializer_list>


inline Point3f cwiseMax(const Point3f &v1, const Point3f &v2)
{
    /*
    return Point3f{
        std::fmax(v1.x, v2.x),
        std::fmax(v1.y, v2.y),
        std::fmax(v1.z, v2.z)};
    */
   /* https://shaderific.com/glsl/common_functions.html
      glm::max/min is component-wise. */
   return glm::max(v1, v2);
}

inline Point3f cwiseMin(const Point3f &v1, const Point3f &v2)
{
    /*
    return Point3f{
        std::fmin(v1.x, v2.x),
        std::fmin(v1.y, v2.y),
        std::fmin(v1.z, v2.z)}; */
    return glm::min(v1, v2);
}


Point3f cwiseMax(const std::initializer_list<Point3f>& vecs);

Point3f cwiseMin(const std::initializer_list<Point3f>& vecs);

/* 比较两个浮点数 */
inline bool equalf(float a, float b, float eps = FLOAT_EPS)
{
    return (std::abs(a - b) < eps) ? true : false;
}

/* 是否是一个零向量 */
inline bool isZeroVec(const Vec3f &v)
{
    return equalf(glm::length(v), 0.f);
}

/* 归一化向量 */
inline Vec3f normalize(const Vec3f &v)
{
    const float len = glm::length(v);
    return equalf(len, 0.f) ? v : (v / len);
}


/* 计算三角形的法向量 */
inline Vec3f calculateNormal(const Point3f &a, const Point3f &b, const Point3f &c)
{
    return normalize(glm::cross(b - a, c - b));
}

/* 计算三角形面积 */
inline float getArea(const Point3f &a, const Point3f &b, const Point3f &c)
{
    return glm::length(glm::cross(b - a, c - b)) / 2.f;
}

/* 字符串中字母全部小写 */
std::string stringToLower(const std::string &s);
/* 将字符串 (如 "0.2, 0.2, 0.2") 转换成对应的 Vec3f (如 Vec3f{0.2f, 0.2f, 0.2f})。 */
Vec3f cvtString2Vec3f(const std::string& s);
/*  将以浮点形式存储的图像 std::vector<Vec3f>  转换为以 std::vector<unsigned char> 存储的图像
    用于 stbi_image 将图像写入文件。 */
std::vector<unsigned char> cvtVectorVec3f2UnsignedChar(const std::vector<Vec3f>& arr, bool gammaCorretion = false);

/*  将图像的 y 轴 (上下) 进行反转;
    stbi_image 中的原点位于左上角, 因此在写入图像之前需要反转 y 轴。 */
std::vector<Vec3f> yAxisFilp(const std::vector<Vec3f>& arr, const size_t width, const size_t height);

/* 判断两个向量是否足够接近 */
template<glm::length_t L, typename T>
inline bool vecClose(const glm::vec<L, T>& p, const glm::vec<L, T>& q) {
    return glm::length(p - q) < DISTANCE_EPS;
} 

/* 用于 PathTracer::nextRaySampling 计算 Fresnel factor */
inline float calculateFresnel(float cosIn, float n1, float n2)
{
    /* https://en.wikipedia.org/wiki/Schlick%27s_approximation */
    const float f0 = glm::pow2((n1 - n2) / (n1 + n2));
    return static_cast<float>(f0 + (1.0f - f0) * std::pow(1.0f - std::abs(cosIn), 5));
}

/* 随机生成 [0,1] 之间的浮点数 */
float random01 ();

/* 随机生成 [low, high) 之间的整数 */
size_t randomInteger (const size_t low , const size_t high );
/* 随机生成 [0, high) 之间的整数 */
size_t randomInteger (const size_t high );

/*  对像素进行 gamma 矫正
    https://en.wikipedia.org/wiki/Gamma_correction */
inline Point3f gammaCorrection(const Point3f& color, const float gamma = 1.f / 2.2f)
    { return glm::pow(color, Point3f{gamma}); }


/* 计时器类 */
class Timer {
public:
    Timer() : totalTime(0.f), elapsedTime(0.f), isRunning(false) {}

    void start() ;
    void stop() ;

    void reset() {
        totalTime = 0.0f;
        isRunning = false;
    }

    float getTotalTime() const { return totalTime; }
    float getElapsedTime() const { return elapsedTime; }

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> startTime;
    float totalTime; /* 累计时间 */
    float elapsedTime;
    bool isRunning;  /* 是否正在计时 */
};


#include "Material.h"

/*  双边滤波类
    对单张 frame 进行滤波
    可能会损失高频细节 */
class BilateralFilter {
public:
    BilateralFilter() = default;
    BilateralFilter(
        const size_t width, const size_t height,
        const int radius = defaultRadius,
        const float sigmaSpace = defaultSigmaSpace, 
        const float sigmaColor = defaultSigmaColor
    ) : width(width), height(height), radius(radius),
        sigmaSpace(sigmaSpace), sigmaColor(sigmaColor) 
    {}

    ~BilateralFilter() = default;
    Vec3f bilateralFilterPixel(
        const std::vector<Vec3f>& frameBuffer, 
        const size_t x, const size_t y) const ;
    void applyBilateralFilter(
        std::vector<Vec3f>& frameBuffer) const;
    void applyBilateralFilter(
        Image& image ) const
    {
        this->applyBilateralFilter(image.data());
        return;
    }
    void init(
        const size_t width, const size_t height,
        const int radius = defaultRadius,
        const float sigmaSpace = defaultSigmaSpace, 
        const float sigmaColor = defaultSigmaColor)
    {
        this->width = width;  this->height = height;
        this->radius = radius; 
        this->sigmaSpace = sigmaSpace; this->sigmaColor = sigmaColor;
    }

public:
    size_t width, height;
    int radius;
    float sigmaSpace, sigmaColor;

    static constexpr int defaultRadius = 10;
    static constexpr float defaultSigmaSpace = 1.f;
    static constexpr float defaultSigmaColor = 1.f;
};