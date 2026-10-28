#pragma once

#include "Defs.h"
#include "Ray.h"

#include <glm/gtc/constants.hpp>


class Camera {
public:
    Vec3f eye {0.f, 0.f, 5.f};
    Vec3f lookAt {0.f, 0.f, 0.f};
    Vec3f up {0.f, 1.f, 0.f};

public:
    virtual ~Camera() = default;

    glm::mat4 getViewMatrix() const {
        return glm::lookAt(
            glm::vec3(eye.x, eye.y, eye.z),
            glm::vec3(lookAt.x, lookAt.y, lookAt.z), 
            glm::vec3(up.x, up.y, up.z) 
        );
    }

    virtual glm::mat4 getProjectionMatrix() const = 0;

};

class PerspectiveCamera : public Camera {
public:
    float fovy = defaultFovy;
    float aspect = 16.0f / 9.0f;
    float znear = 0.1f;
    float zfar = 1e4f;

    /* Stored in degrees because scene configuration files use degrees. */
    static constexpr float defaultFovy = 45.0f;

public:
    PerspectiveCamera() = default;
    PerspectiveCamera(float fovy, float aspect, float znear  = 0.1f,  float zfar  = 1e4f)
        : fovy(fovy), aspect(aspect), znear(znear), zfar(zfar) {}

    ~PerspectiveCamera() = default;

    glm::mat4 getProjectionMatrix() const override {
        return glm::perspective(glm::radians(fovy), aspect, znear, zfar);
    }

};
