#pragma once

#include "Defs.h"

#include <string>
#include <filesystem>

class Texture;

struct Material
{
public:
    Material(const std::string &name);
    ~Material() = default;

    Point3f getAmbience(const Point2f &texCoord) const;
    Point3f getDiffusion(const Point2f &texCoord) const;
    Point3f getSpecularity(const Point2f &texCoord) const;
    Point3f getEmission() const;

public:
    std::string name;

    Vec3f Ka = Vec3f{0};   /* ambience 环境光 */
    Vec3f Kd = Vec3f{0};   /* diffuse reflectance 漫射光 */
    Vec3f Ks = Vec3f{0};   /* specular reflectance 镜面光 */
    Vec3f Ke = Vec3f{0};   /* emission 自射光 对应 xml 文件中 radiance */
    Vec3f Tr = Vec3f{0};   /* transmittance 透射光 */
    float Ns = 60.f;       /* shiness,  the exponent of phong lobe 反光强度 */
    float Ni = 1.0;        /* refraction 折射率 */
    bool emissive = false; /* 是否自发光 */

    Texture *map_Ka = nullptr;
    Texture *map_Kd = nullptr;
    Texture *map_Ks = nullptr;
};

/*  图像类, 用于对图像进行文件读/写;
    可对纹理以及帧缓存进行管理。 */
class Image
{
public:
    Image() : width(0), height(0) {};
    Image(const size_t width, const size_t height, const std::string &name = "")
        : width(width), height(height), name(name)
    {
        this->imgData.resize(width * height, Vec3f{0});
    }
    Image(const std::filesystem::path &filePath);
    ~Image() = default;

    /* 加载图像文件 */
    bool loadFromFile(const std::filesystem::path &filePath);
    /* 写入图像文件 */
    bool writeToFile(const std::filesystem::path& filePath);

    bool empty() const { return this->size() == (size_t)0; }
    size_t size() const { return width * height; }

    const std::vector<Vec3f> &data() const { return this->imgData; }
    std::vector<Vec3f>& data() { return this->imgData; }

    /* 包含越界检查的 getter/setter */
    
    Point3f at(const size_t x, const size_t y) const;
    Point3f& at(const size_t x, const size_t y) ;
    Point3f at(const Vec2f& coord) const;
    Point3f& at(const Vec2f& coord);
    Point3f at(const size_t idx) const;
    Point3f& at(const size_t idx);

    /* 无越界检查的 getter/setter, 提高速度 */
    Vec3f operator()(const size_t x, const size_t y) const {
        return this->imgData[this->calculateIndex(x, y)];
    }
    Vec3f &operator()(const size_t x, const size_t y) {
        return this->imgData[this->calculateIndex(x, y)];
    }
    Vec3f operator()(const Vec2f &coord) const {
        const auto [x, y] = normalizeToPixel(coord);
        return this->operator()(x, y);
    }
    Vec3f& operator()(const Vec2f &coord) {
        const auto [x, y] = normalizeToPixel(coord);
        return this->operator()(x, y);
    }
    Vec3f operator[] (const size_t idx) const {
        return this->imgData[idx];
    }
    Vec3f& operator[] (const size_t idx) {
        return this->imgData[idx];
    }

    /* 初始化为全 0 (黑色) 图像 */
    void init(const size_t width, const size_t height);
    
    inline size_t calculateIndex(const size_t x, const size_t y) const 
    {
        return y * this->width + x; 
    }


private:
    /* 检查输入是否有效 */

    bool chkPixelCoord(const size_t x, const size_t y) const
    {
        return x < this->width && y < this->height;
    }
    bool chkPixelCoord(const Vec2f &coordinate) const
    {
        return (coordinate.x >= 0 && coordinate.x <= 1) &&
               (coordinate.y >= 0 && coordinate.y <= 1);
    }
    /* 将归一化坐标转换为像素坐标 */
    std::pair<size_t, size_t> normalizeToPixel(const Vec2f &coordinate) const {
        size_t x = static_cast<size_t>(coordinate.x * (this->width - 1));
        size_t y = static_cast<size_t>(coordinate.y * (this->height - 1));
        return {x, y};
    }

    std::vector<Vec3f> imgData;

public:
    size_t width, height;
    std::string name;

};

class Texture
{
public:
    Texture() = delete;
    Texture(const std::string &filePath)
        : filePath(filePath), image(filePath)
    {
        ASSERT(std::filesystem::exists(filePath), "Invalid texture path for %s !", filePath.c_str());
        ASSERT(!image.empty(), "Failed to load texture %s!", filePath.c_str());
    }
    ~Texture() = default;

    bool loadFromFile(const std::string &filePath)
    {
        this->filePath = filePath;
        return this->image.loadFromFile(filePath);
    }
    Point3f getColorAt(const Vec2f &coordinate) const 
    { 
        return this->image(coordinate);
    }

public:
    std::string filePath;
    Image image;
};