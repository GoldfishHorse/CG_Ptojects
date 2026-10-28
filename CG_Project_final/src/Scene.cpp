#include "Scene.h"


Ray Scene::getRay(const size_t x, const size_t y, const float jitterStrength) const {
    const float sampleX = static_cast<float>(x) + 0.5f +
                          (random01() - 0.5f) * jitterStrength;
    const float sampleY = static_cast<float>(y) + 0.5f +
                          (random01() - 0.5f) * jitterStrength;
    const Mat4f view = this->camera.getViewMatrix();
    const Vec3f direction   = -Vec3f{view[2]},
                right       = Vec3f{view[0]},
                up          = Vec3f{view[1]};
                
    const float halfViewHeight = std::tan(glm::radians(camera.fovy) * 0.5f);
    const float halfViewWidth = halfViewHeight * this->getAspect();
    const Vec3f viewx = right * (2.0f * halfViewWidth);
    const Vec3f viewy = up * (2.0f * halfViewHeight);
    const Vec3f viewz = direction;
    return Ray (camera.eye,                 
        viewz
        + (sampleX / static_cast<float>(width) - 0.5f) * viewx
        + (sampleY / static_cast<float>(height) - 0.5f) * viewy);
}
