#include "Material.h"
#include "Utils.h"


#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <algorithm>

Material::Material(const std::string& name) 
    :   name(name), Ka(Vec3f{0}), Kd(Vec3f{0}), Ks(Vec3f{0}), Ke(Vec3f{0}),
        Tr(Vec3f{0}), Ns(60.f), Ni(1.f), emissive(false),
        map_Ka(nullptr), map_Kd(nullptr), map_Ks(nullptr)
    {}

Point3f Material::getAmbience(const Point2f& texCoord) const 
{
    return this->map_Ka == nullptr ? this->Ka : this->map_Ka->getColorAt(texCoord);
}
Point3f Material::getDiffusion(const Point2f& texCoord) const
{
    return this->map_Kd == nullptr ? this->Kd : this->map_Kd->getColorAt(texCoord);
}
Point3f Material::getSpecularity(const Point2f& texCoord) const
{
    return this->map_Ks == nullptr ? this->Ks : this->map_Ks->getColorAt(texCoord);
}
Point3f Material::getEmission() const
{
    return this->emissive ? this->Ke : Vec3f{0};
}


Image::Image(const std::filesystem::path &filePath)
{
    this->name = filePath.filename().string();
    if (!this->loadFromFile(filePath))
    {
        LOG_ERROR("Load %s fails.", filePath.string().c_str());
        this->width = this->height = (size_t)0;
        this->name.clear();
    }
}

void Image::init(const size_t width, const size_t height)
{
    this->width = width; this->height = height;
    this->imgData.resize(width * height);
    std::fill(this->imgData.begin(), this->imgData.end(), Vec3f{0});
}

bool Image::loadFromFile(const std::filesystem::path &filePath)
{
    int ret;
    if (!std::filesystem::exists(filePath))
    {
        LOG_ERROR("Image file %s not exist!", filePath.string().c_str());
        ret = false;
    } else {
        int width, height, channels;
        int success = stbi_info(filePath.string().c_str(), &width, &height, &channels);
        if (!success || width * height == 0)
        {
            LOG_ERROR("Image file %s is invalid!", filePath.string().c_str());
            ret = false;
        } else {
            stbi_uc * pImgData = stbi_load(filePath.string().c_str(), &width, &height, &channels, 0);
            if (!pImgData)
            {
                const char *const reason = stbi_failure_reason() ? stbi_failure_reason() : "[Unknown Reason]";
                LOG_ERROR("Cannot load image %s: %s", filePath.string().c_str(), reason);
                ret = false;
            }
            else
            {
                this->width = static_cast<size_t>(width);
                this->height = static_cast<size_t>(height);
                LOG_INFO("Load %s succeeds. width: %zu, height: %zu.",
                    filePath.string().c_str(), this->width, this->height);

                if (channels < 3) {
                    LOG_WARNING("%s is not RGB image!", this->name.c_str());
                } else {
                    channels = std::min(3, channels);
                }

                this->name = filePath.filename().string();
                
                this->imgData.resize(this->width * this->height);
                
                switch (channels)
                {
                    case  3:
                    {
                        /* RGB image */
                        for (size_t i = 0; i < this->size(); ++i)
                        {
                            this->imgData.at(i) = 
                                Point3f{
                                    static_cast<float>( pImgData[i * 3]    )   ,
                                    static_cast<float>( pImgData[i * 3 + 1])   ,
                                    static_cast<float>( pImgData[i * 3 + 2])
                                } /
                                255.f;
                        }
                        ret = true;
                        break;
                    }
                    case 1:
                    {
                        for (size_t i = 0; i < this->size(); ++i)
                        {
                            this->imgData.at(i) = Point3f{
                                static_cast<float>(pImgData[i]) / 255.f
                            };
                        }
                        ret = true;
                        break;
                    }
                    default:
                    {
                        LOG_ERROR("Invalid image channels %d for %s.", channels, this->name.c_str());
                        ret = false;
                    }
                }

                stbi_image_free(pImgData);
            }
        }
    }
    
    return ret;
}


bool Image::writeToFile(const std::filesystem::path& filePath)
{
    std::string ext = filePath.extension().string();
    std::filesystem::path pp = filePath.parent_path();
    if (pp.is_relative())
    {
        pp = std::filesystem::path(PROJECT_SOURCE_DIR) / pp;
    }
    if (!std::filesystem::exists(pp))
    {   
        bool ret;
        try {
            ret = std::filesystem::create_directories(pp);
        } catch ( const std::exception& e ) {
            LOG_ERROR("Create directory %s fails: %s.", pp.string().c_str(), e.what());
            throw e;
        } 
        if (!ret)
        {
            LOG_ERROR("Create directory %s fails.", pp.string().c_str());
        }
    }
    std::vector<unsigned char> dataToWrite = 
        cvtVectorVec3f2UnsignedChar(
            yAxisFilp(this->imgData, this->width, this->height),
            true
        );

    int success = 0;
    if (ext == ".jpg" || ext == ".jpeg")
        success = stbi_write_jpg(
            filePath.string().c_str(),
            static_cast<int>(this->width),
            static_cast<int>(this->height),
            3,
            dataToWrite.data(),
            100
        );
    else if (ext == ".png")
        success = stbi_write_png(
            filePath.string().c_str(),
            static_cast<int>(this->width),
            static_cast<int>(this->height),
            3,
            dataToWrite.data(),
            static_cast<int>(this->width * 3)
        );
    else if (ext == ".bmp")
        success = stbi_write_bmp(
            filePath.string().c_str(),
            static_cast<int>(this->width),
            static_cast<int>(this->height),
            3,
            dataToWrite.data()
        );
    else {
        LOG_ERROR("Invalid filePath %s: extension %s is not supported.",
            filePath.string().c_str(), ext.c_str());
    }
    if (!success)
    {
        LOG_ERROR("Write image to %s fails: %s", 
            filePath.string().c_str(), stbi_failure_reason());
    }
    return bool(success);
}


Point3f Image::at(const size_t x, const size_t y) const
{
    if (this->empty())
    {
        LOG_ERROR("Empty Image!");
        throw std::runtime_error("Accessing Empty Image!");
    } else if (!(this->chkPixelCoord(x, y)))
    {
        LOG_ERROR("Coordinate (%zu, %zu) out of range for image %s!", x, y, this->name.c_str());
        throw std::runtime_error("Coordinate out of range!");
    } 
    return this->imgData.at(this->calculateIndex(x, y));
}
Point3f& Image::at(const size_t x, const size_t y) 
{
    if (this->empty())
    {
        LOG_ERROR("Empty Image!");
        throw std::runtime_error("Accessing Empty Image!");
    } else if (!(this->chkPixelCoord(x, y)))
    {
        LOG_ERROR("Coordinate (%zu, %zu) out of range for image %s!", x, y, this->name.c_str());
        throw std::out_of_range("Coordinate out of range!");
    } 
    return this->imgData.at(this->calculateIndex(x, y));
}
Point3f Image::at(const Vec2f& coord) const
{
    if (this->empty())
    {
        LOG_ERROR("Empty Image!");
        throw std::runtime_error("Accessing Empty Image!");
    } else if (!(this->chkPixelCoord(coord)))
    {
        LOG_ERROR("Coordinate (%f, %f) out of range for image %s!", coord.x, coord.y, this->name.c_str());
        throw std::out_of_range("Coordinate out of range!");
    }
    const auto [x, y] = this->normalizeToPixel(coord);
    return this->imgData.at(this->calculateIndex(x, y));
}
Point3f& Image::at(const Vec2f& coord)
{
    if (this->empty())
    {
        LOG_ERROR("Empty Image!");
        throw std::runtime_error("Accessing Empty Image!");
    } else if (!(this->chkPixelCoord(coord)))
    {
        LOG_ERROR("Coordinate (%f, %f) out of range for image %s!", coord.x, coord.y, this->name.c_str());
        throw std::out_of_range("Coordinate out of range!");
    }
    const auto [x, y] = this->normalizeToPixel(coord);
    return this->imgData.at(this->calculateIndex(x, y));
}
Point3f Image::at(const size_t idx) const
{
    if (this->empty())
    {
        LOG_ERROR("Empty Image!");
        throw std::runtime_error("Accessing Empty Image!");
    } else if (idx >= this->imgData.size())
    {
        LOG_ERROR("Index %zu out of range!", idx);
        throw std::out_of_range("Index out of range!");
    }
    return this->imgData.at(idx);
}
Point3f& Image::at(const size_t idx)
{
    if (this->empty())
    {
        LOG_ERROR("Empty Image!");
        throw std::runtime_error("Accessing Empty Image!");
    } else if (idx >= this->imgData.size())
    {
        LOG_ERROR("Index %zu out of range!", idx);
        throw std::out_of_range("Index out of range!");
    }
    return this->imgData.at(idx);
}

