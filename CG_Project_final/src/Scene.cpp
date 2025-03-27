#include "Scene.h"


Ray Scene::getRay(const size_t x, const size_t y, const float noiseMap) const {    
    float randomX = (random01() - 0.5f) * noiseMap;
    float randomY = (random01() - 0.5f) * noiseMap;
    const Mat4f view = this->camera.getViewMatrix();
    const Vec3f direction   = -Vec3f{view[2]},
                right       = Vec3f{view[0]},
                up          = Vec3f{view[1]};
                
    const Vec3f viewx = right * 2 * tanf(glm::radians(camera.fovy * this->getAspect() / 2.f));
    const Vec3f viewy = up * 2 * tanf(glm::radians(camera.fovy / 2.f));
    const Vec3f viewz = direction;
    return Ray (camera.eye,                 
        viewz
        + ((x + randomX) / width - 0.5f) * viewx
        + ((y + randomY) / height - 0.5f) * viewy);
}