#pragma once

#include "Defs.h"
#include "Utils.h"

#include <optional>

class Ray {
public:
    enum class RayType { DIFFUSE, SPECULAR, TRANSMISSION };

    Ray(Vec3f origin = Vec3f{0.f},
        Vec3f direction = Vec3f{0.f, 0.f, 1.f},
        RayType type = RayType::DIFFUSE) 
        :   origin(origin),
            direction(normalize(direction)),
            tmin(DISTANCE_EPS),
            tmax(FLOAT_MAX),
            type(type)
        {}

    /* 反射 */
    Vec3f reflect(const Vec3f &normal) const {
        return this->direction - 2.0f * normal * 
                glm::dot(this->direction, normal);
    }
    /* 折射; 若折射无法发生则返回 std::nullopt */
    std::optional<Vec3f> refract(const Vec3f &normal, float eta) const {
        /* Ref: https://www.cnblogs.com/night-ride-depart/p/7429618.html */
        const float cosi = glm::dot(this->direction, normal);
        const float cost2 = 1.0f - eta * eta * (1.0f - cosi * cosi);
        if (cost2 >= 0.0f) {
            return eta * this->direction - (eta * cosi + std::sqrt(cost2)) * normal;
        } else {
            /* 全反射 */
            return std::nullopt;
        }
    }

public:
    Vec3f origin, direction;
    float tmin, tmax;
    RayType type;
};
