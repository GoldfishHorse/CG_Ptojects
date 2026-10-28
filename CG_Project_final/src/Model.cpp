#include "Model.h"
#include "Utils.h"

#include <algorithm>
// #include <numeric>

void LightArea::addLightFace(const Face& face)
{
    this->faces.push_back(&face);
    this->area +=  face.area();
    return;
}

void LightArea::finalize()
{
    accumulatedAreas.clear();
    accumulatedAreas.reserve(faces.size());
    float accumulated = 0.0f;
    for (const Face* face : faces)
    {
        accumulated += face->area();
        accumulatedAreas.push_back(accumulated);
    }
    area = accumulated;
}

const Face* LightArea::getRandomFaceByArea() const
{
    if (faces.empty() || area <= 0.0f || accumulatedAreas.size() != faces.size())
    {
        LOG_WARNING("Empty light Area!");
        return nullptr;
    }
    const float sampleArea = area * random01();
    const auto it = std::lower_bound(accumulatedAreas.cbegin(), accumulatedAreas.cend(), sampleArea);
    const size_t index = static_cast<size_t>(std::distance(accumulatedAreas.cbegin(), it));
    return faces.at(std::min(index, faces.size() - 1));
}


void Model::buildKDTree() {
    
    if (this->empty()) {
        LOG_ERROR("Empty model!");
    } else {
        if (!this->kdTree.empty()) {
            LOG_WARNING("Re-building KDTree!");
            this->kdTree.release();
        }
        std::vector<Face*> facePtrVector(this->faces.size());
        std::transform(this->faces.begin(), this->faces.end(), facePtrVector.begin(),
                [](Face& face) { return &face; });
        this->kdTree.build(facePtrVector);
    }
    LOG_INFO("Builded KD-Tree for the model.");
    return;
    
}

void Model::initLights() 
{
    if (this->empty())
    {
        LOG_ERROR("Empty model!");
    } else {
        if (!this->lightAreas.empty())
        {
            LOG_WARNING("Re-initing Lights!");
            this->lightAreas.clear();
        }
        this->accumulatedLightAreas.clear();
        this->totalLightArea = 0.0f;

        std::map<std::string, LightArea> lightAreaMap;
        for (const Face& face: this->faces)
        {
            const Material& material = *face.material;
            if (!material.emissive)
                continue;
            if (auto it = lightAreaMap.find(material.name);
                it == lightAreaMap.end())
            {
                LightArea lightArea;
                lightArea.name = material.name;
                lightArea.material = &material;
                lightArea.addLightFace(face);
                lightAreaMap[material.name] = std::move(lightArea);
            } else {
                it->second.addLightFace(face);
            }
        }
        if (lightAreaMap.empty())
        {
            LOG_WARNING("No light detected in the scene!");
        } else {
            LOG_INFO("Inited %zu light areas.", lightAreaMap.size());
        }

        std::transform(
            lightAreaMap.cbegin(), 
            lightAreaMap.cend(),
            std::back_inserter(this->lightAreas),
            [] (const std::pair<std::string, LightArea>& p) 
            {
                return p.second;
            }
        );
        this->accumulatedLightAreas.reserve(this->lightAreas.size());
        for (LightArea& lightArea : this->lightAreas)
        {
            lightArea.finalize();
            this->totalLightArea += lightArea.area;
            this->accumulatedLightAreas.push_back(this->totalLightArea);
        }
    }
    return;
}

BoundingBox Model::getBoundingBox() const
{
    BoundingBox ret = BoundingBox{
        SFINIT(.minPoint, POINT_MAX),
        SFINIT(.maxPoint, POINT_LOWEST)};
    if (this->empty())
    {
        LOG_ERROR("Empty Model!");
    }
    else if (this->kdTree.empty())
    {
        for (const Face& face : this->faces)
        {
            ret.extend(face.getBoundingBox());
        }
    }
    else
    {
        ret = *(this->kdTree.root);
    }
    return ret;
}

bool Model::hit(
    __INOUT__   Ray &ray, 
    __OUT__     Intersection &intersection) const {
    bool ret;
    if (this->empty()) {
        LOG_ERROR("Empty Model!");
        ret = false;
    } else if (this->kdTree.empty()) {
        ret = false;
        for (const Face& face : this->faces)
            ret = face.hit(ray, intersection) || ret;
    } else {
        ret = this->kdTree.root->hit(ray, intersection);
    }
    return ret;
}

Material* Model::fetchMaterial(const std::string& mtlName)
{
    if (const auto it = this->materials.find(mtlName);
        it == this->materials.end())
    {
        return nullptr;
    } else {    
        return &(it->second);
    }
}

Texture* Model::fetchTexture(const std::string& mapPath)
{
    if (const auto it = this->textures.find(mapPath);
        it == this->textures.end())
    {
        return nullptr;
    } else {
        return &(it->second);
    }
}

Material* Model::addMaterial(const Material& material)
{
    if (this->materials.find(material.name) != this->materials.end())
    {
        LOG_WARNING("Material %s already exists!", material.name.c_str());
    }
    auto [it, _] = this->materials.insert_or_assign(material.name, material);
    return &(it->second);
}


Texture* Model::addTexture(const Texture& texture)
{
    if (this->textures.find(texture.filePath) != this->textures.end())
    {
        LOG_WARNING("Texture %s already exists!", texture.filePath.c_str());
    }
    auto [it, _] = this->textures.insert_or_assign(texture.filePath, texture);
    return (&it->second);
}


Material* Model::addMaterial(Material&& material)
{
    if (this->materials.find(material.name) != this->materials.end())
    {
        LOG_WARNING("Material %s already exists!", material.name.c_str());
    }
    auto [it, _] = this->materials.insert_or_assign(material.name, std::move(material));
    return &(it->second);
}
Texture* Model::addTexture(Texture&& texture)
{
    if (this->textures.find(texture.filePath) != this->textures.end())
    {
        LOG_WARNING("Texture %s already exists!", texture.filePath.c_str());
    }
    auto [it, _] = this->textures.insert_or_assign(texture.filePath, std::move(texture));
    return (&it->second);
}


const Face* Model::sampleLight(float& pdfArea) const
{
    pdfArea = 0.0f;
    if (lightAreas.empty() || totalLightArea <= 0.0f ||
        accumulatedLightAreas.size() != lightAreas.size())
    {
        return nullptr;
    }

    const float sampleArea = totalLightArea * random01();
    const auto it = std::lower_bound(
        accumulatedLightAreas.cbegin(), accumulatedLightAreas.cend(), sampleArea);
    const size_t index = static_cast<size_t>(
        std::distance(accumulatedLightAreas.cbegin(), it));
    const LightArea& lightArea = lightAreas.at(std::min(index, lightAreas.size() - 1));
    pdfArea = 1.0f / totalLightArea;
    return lightArea.getRandomFaceByArea();
}
