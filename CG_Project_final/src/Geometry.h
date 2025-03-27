#pragma once

#include "Defs.h"
#include "Utils.h"
#include "Material.h"
#include "Ray.h"

/* 包围盒 */
struct BoundingBox
{
	Point3f minPoint, maxPoint;

    Point3f center() const {
        return (minPoint + maxPoint) / 2;
    }
    
    /* 合并两个包围盒, 即返回包围 b1 和 b2 的最小包围盒 */
    static BoundingBox merge(const BoundingBox& b1, const BoundingBox& b2) {
        return BoundingBox {
            SFINIT(.minPoint, cwiseMin(b1.minPoint, b2.minPoint)),
            SFINIT(.maxPoint, cwiseMax(b1.maxPoint, b2.maxPoint))
        };
    }

    BoundingBox merge (const BoundingBox& other) const { return BoundingBox::merge(*this, other); }
    void extend (const BoundingBox& other) { *this = this->merge(other); }

    /* 包围盒与射线是否相交 */
    bool intersect (const Ray& ray) const;
} ;

struct Face;

/* 用于记录相交点的信息 */
struct Intersection {
    const Face* face = nullptr;
    Point3f point = Vec3f{0};
    Vec3f normal  = Vec3f{0};

    Point2f getTexCoord() const;
};

/* 模型的一个 (三角形) 面片 */
struct Face {
    Vec3f vertices[3];
    Vec2f texcoords[3];
    Vec3f normals[3];

    /* 面片材质 */
    const Material* material;
    /* 面片的 (轴对齐) 包围盒 */
    BoundingBox getBoundingBox() const {
        return BoundingBox {
            SFINIT(.minPoint, cwiseMin({vertices[0], vertices[1], vertices[2]})),
            SFINIT(.maxPoint, cwiseMax({vertices[0], vertices[1], vertices[2]}))
        };
    }
    /* 计算重心坐标 */
    Point3f barycentric(const Point3f& point) const;
    /* 计算纹理坐标, 在面片材质存在纹理贴图时使用 */
    Point2f getTexCoord(const Point3f& point) const;
    /* 该面片的面积 */
    float area() const;
    /* 该面片的法向量 */
    Vec3f getNormal() const;
    /* 三角形面片与射线求交; 若不相交则返回 false; 
        否则将求交结果放入 intersection 参数 */
    bool hit(
        __INOUT__   Ray &ray, 
        __OUT__     Intersection &intersection,  
        float eps = DISTANCE_EPS) const;

    /* 在面片上随机选择一点 */
    Point3f randomSample() const;
};