#include "PathTracer.h"
#include <omp.h>
#include <string>
#include <chrono>


PathTracer::PathTracer(
    Scene *scene,
    const size_t lightSamples,
    const size_t maxDepth,
    const size_t filterFrequency,
    const size_t saveImgFrequency,
    const std::string& saveImgPath
)
    : scene(scene),
      iterCnt(0),
      lightSamples(lightSamples),
      maxDepth(maxDepth),
      filterFrequency(filterFrequency),
      saveImgFrequency(saveImgFrequency),
      saveImgPath(saveImgPath)
{
    if (scene != nullptr)
        this->init(scene);
}

Vec3f PathTracer::BRDFImportanceSampling(const Vec3f &direction, bool isSpecular, float Ns)
{
    const float phi = random01() * 2 * glm::pi<float>();
    const float theta = isSpecular ? 
                    /* Sampling specular term */
                    std::acos( std::pow(random01(), 1 / (Ns + 1))) :
                    /* cosine-weighted Lambertian */ 
                    std::asin(std::sqrt(random01()));

    const Vec3f sample {
        std::sin(theta) * std::cos(phi), 
        std::cos(theta), 
        std::sin(theta) * std::sin(phi)};
    Vec3f front;
    if (std::fabs(direction.x) > std::fabs(direction.y))
    {
        front = normalize(Vec3f{direction.z, 0, -direction.x});
    }
    else
    {
        front = normalize(Vec3f({0, -direction.z, direction.y}));
    }
    const Vec3f right = glm::cross(direction, front);
    return normalize(sample.x * right + sample.y * direction + sample.z * front);
}

PathTracer::BounceSample PathTracer::nextRaySampling(
    const Ray &ray, const Intersection &intersection)
{
    const Material &material = *(intersection.face->material);
    const Point2f texcoord = intersection.getTexCoord();
    Vec3f normal = intersection.normal;
    if (isZeroVec(normal))
        return {};

    const bool entering = glm::dot(ray.direction, normal) < 0.0f;
    const Vec3f orientedNormal = entering ? normal : -normal;

    /* Dielectric interface: sample reflection/refraction according to Fresnel.
       The event probability is the Fresnel weight, so it cancels from the
       Monte Carlo throughput for an ideal interface. */
    if (material.Ni > 1.0f + DISTANCE_EPS &&
        glm::compMax(material.Tr) > DISTANCE_EPS)
    {
        const float n1 = entering ? 1.0f : material.Ni;
        const float n2 = entering ? material.Ni : 1.0f;
        const float fresnel = calculateFresnel(
            glm::dot(-ray.direction, orientedNormal), n1, n2);
        const auto refracted = ray.refract(orientedNormal, n1 / n2);

        if (!refracted.has_value() || random01() < fresnel)
        {
            const Vec3f direction = ray.reflect(orientedNormal);
            const Vec3f reflectance = glm::compMax(material.Ks) > DISTANCE_EPS
                ? material.Ks : Vec3f{1.0f};
            return {Ray(intersection.point + orientedNormal * DISTANCE_EPS,
                        direction, Ray::RayType::SPECULAR),
                    reflectance, true};
        }

        return {Ray(intersection.point - orientedNormal * DISTANCE_EPS,
                    refracted.value(), Ray::RayType::TRANSMISSION),
                material.Tr, true};
    }

    const Vec3f kd = material.getDiffusion(texcoord);
    const Vec3f ks = material.getSpecularity(texcoord);
    const float kdWeight = std::max(glm::compMax(kd), 0.0f);
    const float ksWeight = std::max(glm::compMax(ks), 0.0f);
    const float weightSum = kdWeight + ksWeight;
    if (weightSum <= DISTANCE_EPS)
        return {};

    const float diffuseProbability = kdWeight / weightSum;
    if (random01() < diffuseProbability)
    {
        const Vec3f direction = BRDFImportanceSampling(orientedNormal);
        return {Ray(intersection.point + orientedNormal * DISTANCE_EPS,
                    direction, Ray::RayType::DIFFUSE),
                kd / diffuseProbability, true};
    }

    const float specularProbability = 1.0f - diffuseProbability;
    const Vec3f direction = BRDFImportanceSampling(
        ray.reflect(orientedNormal), true, std::max(material.Ns, 0.0f));
    const float phongWeight = (material.Ns + 2.0f) / (material.Ns + 1.0f) *
                              std::max(glm::dot(orientedNormal, direction), 0.0f);
    return {Ray(intersection.point + orientedNormal * DISTANCE_EPS,
                direction, Ray::RayType::SPECULAR),
            ks * (phongWeight / specularProbability), true};
}

Vec3f PathTracer::traceRay(Ray &ray, size_t depth)
{
    if (Intersection intersection; this->scene->hit(ray, intersection))
    {
        const Material& material = *intersection.face->material;
        /* With next-event estimation, diffuse BSDF paths must not count an
           emitter again or the same light transport path is double counted. */
        const Vec3f emitted = (depth == 0 || ray.type != Ray::RayType::DIFFUSE)
            ? material.getEmission() : Vec3f{0.0f};
        if (depth >= this->maxDepth || material.emissive)
            return emitted;

        const Vec3f direct = directLight(intersection);
        const BounceSample bounce = nextRaySampling(ray, intersection);
        if (!bounce.valid)
            return emitted + direct;

        Ray nextRay = bounce.ray;
        return emitted + direct + bounce.throughput * traceRay(nextRay, depth + 1);
    }
    return this->scene->ambient;
}


Vec3f PathTracer::directLight(const Intersection &intersection) {
    Vec3f ret = Vec3f{0};
    const Material& material = *intersection.face->material;
    const Vec3f d = material.getDiffusion(intersection.getTexCoord());
    if (lightSamples > 0 && glm::compMax(d) > DISTANCE_EPS) {
        /* 对场景中的光源进行随机采样 */
        for (size_t _ = 0; _ < lightSamples; ++_) 
        {
            float pdfArea = 0.0f;
            const Face* lightFace = this->scene->model.sampleLight(pdfArea);
            if (lightFace == nullptr || pdfArea <= 0.0f)
                break;

            const Point3f lightPoint = lightFace->randomSample();
            const Vec3f toLight = lightPoint - intersection.point;
            const float distanceSquared = glm::dot(toLight, toLight);
            if (distanceSquared <= glm::pow2(DISTANCE_EPS))
                continue;

            const float distance = std::sqrt(distanceSquared);
            const Vec3f wi = toLight / distance;
            const Vec3f surfaceNormal = intersection.normal;
            const float cosSurface = std::max(glm::dot(surfaceNormal, wi), 0.0f);
            const float cosLight = std::max(glm::dot(lightFace->getNormal(), -wi), 0.0f);
            if (cosSurface <= 0.0f || cosLight <= 0.0f)
                continue;

            Ray shadowRay(intersection.point + surfaceNormal * DISTANCE_EPS, wi);
            shadowRay.tmax = distance - 2.0f * DISTANCE_EPS;
            Intersection blocker;
            if (shadowRay.tmax > shadowRay.tmin && this->scene->hit(shadowRay, blocker))
                continue;

            const Vec3f emission = lightFace->material->getEmission();
            ret += (d / glm::pi<float>()) * emission *
                   (cosSurface * cosLight / (distanceSquared * pdfArea));
        }
        ret /= static_cast<float>(lightSamples);
    }            

    return ret;
}

void PathTracer::renderScene()
{
    if (this->scene == nullptr)
    {
        LOG_ERROR("PathTracer not inited yet!");
        return;
    }

    ++(this->iterCnt);
    this->timer.start();

    /* error C3016: “y”: OpenMP“for”语句中的索引变量必须是有符号的整型 */
    const size_t pixelCount = this->scene->width * this->scene->height;
    #pragma omp parallel for schedule(dynamic, 64)
    for (ssize_t index = 0; index < static_cast<ssize_t>(pixelCount); ++index)
    {
        const size_t pixelIndex = static_cast<size_t>(index);
        const size_t x = pixelIndex % this->scene->width;
        const size_t y = pixelIndex / this->scene->width;
        Ray ray = this->scene->getRay(x, y);
        const Vec3f color = traceRay(ray, 0);
        this->scene->frameBuffer[pixelIndex] =
            (this->scene->frameBuffer[pixelIndex] * static_cast<float>(iterCnt - 1) + color) /
            static_cast<float>(iterCnt);
    }


    LOG_INFO("Iteration : %zu", this->iterCnt);

    // auto t_end = std::chrono::high_resolution_clock::now();
    // float tloop = std::chrono::duration_cast<std::chrono::duration<float>>(t_end - t_start).count();
    this->timer.stop();
    // time_sum += tloop;
    
    LOG_INFO("Time: %f s Sum: %f s", 
        this->timer.getElapsedTime(), 
        this->timer.getTotalTime());
    
    if (filterFrequency > 0 && (this->iterCnt) % filterFrequency == 0)
    {
        LOG_INFO("Applying Bilateral Filter...");
        this->bilateralFilter.applyBilateralFilter(
            this->scene->frameBuffer
        );
    }

    if (saveImgFrequency > 0 && (this->iterCnt) % saveImgFrequency == 0)
    {
        std::filesystem::path p = 
            std::filesystem::path(this->saveImgPath) / (std::to_string(this->iterCnt) + ".png");
        LOG_INFO("Saving to image %s .", p.string().c_str());
        this->scene->frameBuffer.writeToFile(p);
    }

    return;
}
