#include "Geometry.h"

bool BoundingBox::intersect(const Ray &ray) const
{
    /* Robust slab test. The explicit parallel-axis case avoids 0 * inf NaNs. */
    float tEnter = ray.tmin;
    float tExit = ray.tmax;
    for (glm::length_t axis = 0; axis < 3; ++axis)
    {
        const float direction = ray.direction[axis];
        if (std::abs(direction) <= FLOAT_EPS)
        {
            if (ray.origin[axis] < minPoint[axis] || ray.origin[axis] > maxPoint[axis])
                return false;
            continue;
        }

        float t0 = (minPoint[axis] - ray.origin[axis]) / direction;
        float t1 = (maxPoint[axis] - ray.origin[axis]) / direction;
        if (t0 > t1)
            std::swap(t0, t1);
        tEnter = std::max(tEnter, t0);
        tExit = std::min(tExit, t1);
        if (tEnter > tExit)
            return false;
    }
    return true;
}

Point2f Intersection::getTexCoord() const
{ return this->face->getTexCoord(this->point); }


Point3f Face::barycentric(const Point3f &point) const
{
    const Vec3f edge1 = this->vertices[1] - this->vertices[0],
                edge2 = this->vertices[2] - this->vertices[0];

    const Vec3f v2_ = point - this->vertices[0];
    const float d20 = glm::dot(v2_, edge1);
    const float d21 = glm::dot(v2_, edge2);

    const float d00 = glm::dot(edge1, edge1);
    const float d01 = glm::dot(edge1, edge2);
    const float d11 = glm::dot(edge2, edge2);
    const float d = d00 * d11 - d01 * d01;

    float u, v, w;
    if (equalf(d, 0.f))
    {
        v = w = 0.f;
    } else {
        v = (d11 * d20 - d01 * d21) / d;
        w = (d00 * d21 - d01 * d20) / d;
    }
    u = 1.f - v - w;
    return Point3f{u, v, w};
}

Point2f Face::getTexCoord(const Point3f& point) const
{
    const Point3f bc = this->barycentric(point);
    Point2f texCoord =  this->texcoords[0] * bc.x + 
                        this->texcoords[1] * bc.y +
                        this->texcoords[2] * bc.z;
    /* 保证纹理坐标都在[0, 1]的范围内 */
    while (texCoord.x < 0 || texCoord.x > 1) {
        texCoord.x += texCoord.x < 0 ? 1 : -1;
    }
    while (texCoord.y < 0 || texCoord.y > 1) {
        texCoord.y += texCoord.y < 0 ? 1 : -1;
    }
    return texCoord;
}

float Face::area() const {
    return getArea(this->vertices[0], this->vertices[1], this->vertices[2]);
}

Vec3f Face::getNormal() const {
    return calculateNormal(this->vertices[0], this->vertices[1], this->vertices[2]);
}


bool Face::hit(
    __INOUT__   Ray &ray, 
    __OUT__     Intersection &intersection, 
                float eps) const
{

    /* https://www.cnblogs.com/graphics/archive/2010/08/09/1795348.html */
    /* Möller-Trumbore 算法 */
    float t, u, v;
    const Vec3f edge1 = this->vertices[1] - this->vertices[0],
                edge2 = this->vertices[2] - this->vertices[0];

    const Vec3f P = glm::cross(ray.direction, edge2); 
    float det = glm::dot(edge1, P);

    Vec3f T;
    if (det > 0)
    {
        T = ray.origin - this->vertices[0];
    }
    else
    {
        T = this->vertices[0] - ray.origin;
        det = -det;
    }
    if (det < eps)
    {
        return false;
    }
    u = glm::dot(T, P);
    if (u < 0.0f || u > det)
    {
        return false;
    }
    Vec3f Q = glm::cross(T, edge1);
    v = glm::dot(ray.direction, Q);
    if (v < 0.0f || u + v > det)
    {
        return false;
    }
    t = glm::dot(edge2, Q) / det;
    if (!(t <= ray.tmax && t >= ray.tmin))
    {
        return false;
    }

    intersection.point = ray.origin + ray.direction * t;
    const Point3f bc = this->barycentric(intersection.point);
    Vec3f shadingNormal = normals[0] * bc.x + normals[1] * bc.y + normals[2] * bc.z;
    intersection.normal = isZeroVec(shadingNormal) ? this->getNormal() : normalize(shadingNormal);
    intersection.face = this;
    ray.tmax = t;
    return true;
}


Point3f Face::randomSample() const
{
    /* 在三角形上均匀随机取点 https://www.cs.princeton.edu/~funk/tog02.pdf */
    const float r1 = random01(),
                r2 = random01();
    return (1-glm::sqrt(r1)) * this->vertices[0] + 
           glm::sqrt(r1) * (1 - r2) * this->vertices[1] + 
           glm::sqrt(r1) * r2 * this->vertices[2];
}

