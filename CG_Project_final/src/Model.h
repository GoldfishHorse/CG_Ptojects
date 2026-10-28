#pragma once

#include "Defs.h"
#include "Utils.h"
#include "Geometry.h"
#include "Material.h"
#include "Ray.h"
#include "KDTree.h"

#include <vector>
#include <map>


struct LightArea {
public:
    std::string name;
    std::vector<const Face *> faces;
    std::vector<float> accumulatedAreas;
    const Material* material = nullptr;
    float area = 0.f;

    void addLightFace(const Face& face);
    void finalize();
    /* 根据面积加权随机选择一个 Face */
    const Face* getRandomFaceByArea() const;
};

class Model {
public:
    Model() = default;
    ~Model() = default;

    bool empty() const { return this->faces.empty(); }
    BoundingBox getBoundingBox() const;
    /* 射线是否与模型相交 */
    bool hit(
        __INOUT__   Ray &ray, 
        __OUT__     Intersection &intersection) const;

    void buildKDTree() ;
    /* 将发光 (emissive = true) 的面分类入各个 LightArea */
    void initLights() ;

    /* getter/setter */

    Material* addMaterial(const Material& material);
    Texture* addTexture(const Texture& texture);
    Material* addMaterial(Material&& material);
    Texture* addTexture(Texture&& texture);
    Material* fetchMaterial(const std::string& mtlName);
    Texture* fetchTexture(const std::string& mapPath);
    
    /* 在所有发光三角形的表面并集上均匀采样。 */
    const Face* sampleLight(float& pdfArea) const;

public:
    std::vector<Face> faces;
    std::map<std::string, Texture> textures;
    std::map<std::string, Material> materials;
    std::vector<LightArea> lightAreas;
    std::vector<float> accumulatedLightAreas;
    float totalLightArea = 0.0f;

    /* 使用 KD-Tree 管理模型, 加速求交计算 */
    KDTree kdTree;
};
