#include "utils.h"

// #include <gdiplus.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <cctype>

/* https://zhuanlan.zhihu.com/p/426939341 */
std::vector<std::string> stringSplit(const std::string &str, const char delim)
{
    std::stringstream ss(str);
    std::string item;
    std::vector<std::string> elems;
    while (std::getline(ss, item, delim))
    {
        if (!item.empty())
        {
            elems.push_back(item);
        }
    }
    return elems;
}

Vec3f calculateNormal(const Point3f &a, const Point3f &b, const Point3f &c)
{
    Vec3f edge1 = b - a;
    Vec3f edge2 = c - b;
    Vec3f normal;
    D3DXVec3Cross(&normal, &edge1, &edge2);
    D3DXVec3Normalize(&normal, &normal);
    return normal;
}

Color clampColor(const Color &color)
{
    using std::clamp;
    return Color{
        clamp(color.r, 0.0f, 1.0f),
        clamp(color.g, 0.0f, 1.0f),
        clamp(color.b, 0.0f, 1.0f),
        clamp(color.a, 0.0f, 1.0f)};
}

Vec3f vec3SelectMax(const Vec3f &v1, const Vec3f &v2)
{
    return Vec3f{
        std::fmax(v1.x, v2.x),
        std::fmax(v1.y, v2.y),
        std::fmax(v1.z, v2.z)};
}

Vec3f vec3SelectMin(const Vec3f &v1, const Vec3f &v2)
{
    return Vec3f{
        std::fmin(v1.x, v2.x),
        std::fmin(v1.y, v2.y),
        std::fmin(v1.z, v2.z)};
}

INT saveImage(const char *filename, const Color *data, INT width, INT height)
{
    if (data == nullptr)
    {
        LOG_ERROR("Invalid argument data!");
        return EXIT_FAILURE;
    }

    const std::filesystem::path path{filename};
    if (!(path.extension() == ".bmp"))
    {
        LOG_ERROR("Invalid filename! .bmp extension required.");
        return EXIT_FAILURE;
    }
    if (std::filesystem::exists(path))
    {
        LOG_WARNING("File %s exists! The file will be overwritten.", filename);
        std::filesystem::remove(path);
    }

    /* 文件头和信息头 */
    BITMAPFILEHEADER fileHeader = {};
    BITMAPINFOHEADER infoHeader = {};

    /* BMP 文件头 */
    fileHeader.bfType = 0x4D42;
    fileHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + width * height * 3;
    fileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    /* BMP 信息头 */
    infoHeader.biSize = sizeof(BITMAPINFOHEADER);
    infoHeader.biWidth = width;
    infoHeader.biHeight = height;
    infoHeader.biPlanes = 1;
    infoHeader.biBitCount = 24;
    infoHeader.biCompression = BI_RGB;
    infoHeader.biSizeImage = 0;
    infoHeader.biXPelsPerMeter = 0;
    infoHeader.biYPelsPerMeter = 0;
    infoHeader.biClrUsed = 0;
    infoHeader.biClrImportant = 0;

    /* 保存位图 */
    std::ofstream file(filename, std::ios::out | std::ios::binary);
    if (!file) {
        LOG_ERROR("Failed to create image file.");
        return EXIT_FAILURE;
    }
    file.write(reinterpret_cast<char*>(&fileHeader), sizeof(fileHeader));
    file.write(reinterpret_cast<char*>(&infoHeader), sizeof(infoHeader));

    /* 将颜色数据写入位图数据 */
    for (INT y = 0; y < height; ++y) {
        for (INT x = 0; x < width; ++x) {
            D3DCOLOR color = (D3DCOLOR)data[y * width + x];
            file.put(static_cast<BYTE>(color));
            file.put(static_cast<BYTE>(color >> 8));
            file.put(static_cast<BYTE>(color >> 16));
        }
    }

    file.close();

    return EXIT_SUCCESS;
}

std::string stringToLower(const std::string &s)
{
    std::string ret;
    ret.reserve(s.size());
    std::transform(s.cbegin(), s.cend(), std::back_inserter(ret),
        [](const unsigned char character) {
            return static_cast<char>(std::tolower(character));
        });
    return ret;
}
