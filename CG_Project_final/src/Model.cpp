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

const Face* LightArea::getRandomFace() const
{
    if (this->faces.empty())
    {
        LOG_WARNING("Empty light Area!");
        return nullptr;
    } else {
        return this->faces.at(randomInteger(this->faces.size()));
    }
}

const Face * LightArea::getRandomFaceByArea() 
{
    const Face * ret = nullptr;
    if (this->faces.empty())
    {
        LOG_WARNING("Empty light Area!");
    } else {
        const float sampleArea = this->area * random01();
        if (this->accumulatedAreas.empty())
        {
            /* build this->accumulatedAreas */
            this->accumulatedAreas.push_back(this->faces.front()->area());
            for (auto it = this->faces.cbegin() + 1; it != this->faces.cend(); ++it)
            {
                this->accumulatedAreas.push_back((*it)->area() + this->accumulatedAreas.back());
            }
        }
        if (
            auto it = std::lower_bound(
                this->accumulatedAreas.cbegin(),
                this->accumulatedAreas.cend(),
                sampleArea
            );
            it != this->accumulatedAreas.cend()
        ) {
            ret = this->faces.at(
                std::distance(this->accumulatedAreas.cbegin(), it)
            );
        }
        
    }
    return ret;
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
        for (const Face& face : this->faces) {
            ret = face.hit(ray, intersection);
            if (ret)
                break;
        }
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


const LightArea* Model::getRandomLightArea() const
{
    if (this->lightAreas.empty())
    {
        LOG_WARNING("No light area in the scene!");
        return nullptr;
    } else if (this->lightAreas.size() == (size_t(1))) {
        return &(this->lightAreas.front());
    } 
    else {
        return &(this->lightAreas.at(randomInteger(this->lightAreas.size())));
    }
}