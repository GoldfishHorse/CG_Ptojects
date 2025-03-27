#include "Utils.h"


#include <omp.h>

#include <string>
#include <cstdio>
#include <random>
#include <numeric>

Point3f cwiseMax(const std::initializer_list<Point3f>& vecs) {    
    return std::accumulate(vecs.begin(), vecs.end(), POINT_LOWEST, 
        [] (const Point3f& p1, const Point3f& p2) -> Point3f { return cwiseMax(p1, p2); } );
}

Point3f cwiseMin(const std::initializer_list<Point3f>& vecs) {
    return std::accumulate(vecs.begin(), vecs.end(), POINT_MAX, 
        [] (const Point3f& p1, const Point3f& p2) -> Point3f { return cwiseMin(p1, p2); } );
}




std::string stringToLower(const std::string &s)
{
    std::string ret;
    std::transform(s.cbegin(), s.cend(), std::back_inserter(ret), ::tolower);
    return ret;
}

Vec3f cvtString2Vec3f(const std::string& s)
{
    float x,y,z;
#   if defined(_MSC_VER)
    int r = sscanf_s(s.c_str(), "%f,%f,%f", &x, &y, &z);
#   else
    int r = std::sscanf(s.c_str(), "%f,%f,%f", &x, &y, &z);
#   endif
    ASSERT(r == 3, "Convert String %s to Vec3f fails!", s.c_str());
    return Vec3f{x,y,z};
}

std::vector<unsigned char> cvtVectorVec3f2UnsignedChar(const std::vector<Vec3f>& arr, bool gammaCorretion)
{
    std::vector<unsigned char> ret (arr.size() * 3);
    for (size_t i = 0; i < arr.size(); ++ i)
    {
        const Point3f p = gammaCorretion ? gammaCorrection(arr[i]) : arr[i];
        const Point3f c = glm::clamp(p * 255.f, Vec3f{0.f}, Vec3f{255.f});
        ret[i * 3]      = static_cast<unsigned char>(c.r);
        ret[i * 3 + 1]  = static_cast<unsigned char>(c.g);
        ret[i * 3 + 2]  = static_cast<unsigned char>(c.b);
    }
    return ret;
}

std::vector<Vec3f> yAxisFilp(const std::vector<Vec3f>& arr, const size_t width, const size_t height)
{
    /* stb_image 的原点在左上角, 需要对 y 轴进行一次反转 */
    std::vector<Vec3f> ret (width * height);
    for (size_t r = 0; r < height; ++r)
    {
        std::copy(
            std::next(arr.cbegin(), r * width),
            std::next(arr.cbegin(), (1 + r) * width),
            std::next(ret.begin(), (height - 1 - r) * width)
        );
    }
    return ret;
}


float random01 () {
    static std::default_random_engine e;
    static std::uniform_real_distribution<float> d(0.f, 1.f);
    return d(e);
}

size_t randomInteger (const size_t low , const size_t high )
{
    static std::default_random_engine e;
    std::uniform_int_distribution<size_t> d(low, high - 1);
    return d(e);
}

size_t randomInteger (const size_t high ) 
{
    return randomInteger(0, high);
}

void Timer::start() {
    if (!this->isRunning) {
        this->startTime = std::chrono::high_resolution_clock::now();
        this->isRunning = true;
    } else {
        LOG_WARNING("Timer is already running!");
    }
    return;
}

void Timer::stop() 
{
    if (this->isRunning) {
        const auto endTime = std::chrono::high_resolution_clock::now();
        const auto duration = 
            std::chrono::duration_cast<std::chrono::duration<float> >(endTime - startTime);
        const float t = duration.count();

        this->elapsedTime = t;
        this->totalTime += t;
        this->isRunning = false;
    } else {
        LOG_WARNING("Timer is not running!");
    }
}

Vec3f BilateralFilter::bilateralFilterPixel(
    const std::vector<Vec3f>& frameBuffer, 
    const size_t x, const size_t y) const 
{
    Vec3f sum = Vec3f{0.f};
    float sumWeight = 0.0f;

    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            const int nx = static_cast<int>(x) + dx, ny = static_cast<int>(y) + dy;
            if (nx >= 0 && nx < static_cast<int>(width) && 
                ny >= 0 && ny < static_cast<int>(height)) {
                const Vec3f& neighborColor = frameBuffer[ny * width + nx];
                const Vec3f& centerColor = frameBuffer[y * width + x];

                /* 空间权重（基于像素距离） */
                const float spaceWeight = std::exp(-(dx * dx + dy * dy) / (2 * sigmaSpace * sigmaSpace));

                /* 颜色权重（基于颜色差异）*/
                const float colorWeight = std::exp(-glm::length(neighborColor - centerColor) / (2 * sigmaColor * sigmaColor));

                const float weight = spaceWeight * colorWeight;
                sum += neighborColor * weight;
                sumWeight += weight;
            }
        }
    }

    return sum / sumWeight;
}
void BilateralFilter::applyBilateralFilter(
    std::vector<Vec3f>& frameBuffer) const
{
    std::vector<Vec3f> tempBuffer = frameBuffer; /* 临时缓冲区 */

    /*  error C3016: “y”: OpenMP“for”语句中的索引变量必须是有符号的整型 */
    #pragma omp parallel for
    for (ssize_t y = 0; y < static_cast<ssize_t>(height); ++y) {
        #pragma omp parallel for
        for (ssize_t x = 0; x < static_cast<ssize_t>(width); ++x) {
            frameBuffer[y * width + x] = bilateralFilterPixel(tempBuffer, x, y);
        }
    }
    return;
}