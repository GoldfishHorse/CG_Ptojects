#include "PathTracer.h"
#include <omp.h>
#include <string>
#include <chrono>


PathTracer::PathTracer(
    Scene *scene,
    const size_t lightSamples,
    const size_t maxDepth,
    const size_t filterFrequence,
    const size_t saveImgFrequence ,
    const std::string& saveImgPath
)
    : scene(scene),
      iterCnt(0),
      lightSamples(lightSamples),
      maxDepth(maxDepth),
      filterFrequence(filterFrequence),
      saveImgFrequence(saveImgFrequence),
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

Ray PathTracer::nextRaySampling(Ray &ray, const Intersection &intersection)
{
    const Material &material = *(intersection.face->material);
    Ray ret;


    // if has Refraction
    if (material.Ni > 1.f + DISTANCE_EPS)
    {
        float n1, n2;
        float cosIn = glm::dot(ray.direction, intersection.normal);
        Vec3f normal;
        if (cosIn > 0)
        {
            // out of glass
            normal = -intersection.normal;
            n1 = material.Ni;
            n2 = 1.f;
        }
        else
        {
            // in to the glass
            normal = intersection.normal;
            n1 = 1.f;
            n2 = material.Ni;
        }


        if (calculateFresnel(cosIn, n1, n2) < random01())
        {
            if (auto direction = ray.refract(normal, n1 / n2); direction.has_value())
            {
                ret = Ray(intersection.point, direction.value(), Ray::RayType::TRANSMISSION);
            }
            else
            {
                // only specular for refraction materia
                ret = Ray(intersection.point, ray.reflect(normal), Ray::RayType::SPECULAR);
            }
            return ret;
        }
    }

    // reflection
    const float kdNorm = static_cast<float>(material.getDiffusion(intersection.getTexCoord()).length()),
                ksNorm = static_cast<float>(material.getSpecularity(intersection.getTexCoord()).length());
    const bool isSpecular = random01() > kdNorm / (kdNorm + ksNorm);
    if (isSpecular)
    {
        ret = Ray(
            intersection.point,
            BRDFImportanceSampling(
                ray.reflect(intersection.normal), true, material.Ns),
            Ray::RayType::SPECULAR);
    }
    else
    {
        ret = Ray(
            intersection.point,
            BRDFImportanceSampling(intersection.normal),
            Ray::RayType::DIFFUSE);
    }

    return ret;
}

Vec3f PathTracer::traceRay(Ray &ray, size_t depth)
{
    Vec3f ret;
    if (Intersection intersection; this->scene->hit(ray, intersection))
    {
        const Material& material = *intersection.face->material;
        const Point2f texcoord = intersection.getTexCoord();

        if (depth >= this->maxDepth)
        {
            /* 达到最大递归深度 */
            ret = material.Ke + material.getAmbience(texcoord) * this->scene->ambient;
        } else {
            Vec3f indirectLight, directLight;

            Ray nextRay = this->nextRaySampling(ray, intersection);
            switch (nextRay.type)
            {
                case Ray::RayType::DIFFUSE:
                    indirectLight = material.getDiffusion(texcoord) * 
                                    this->traceRay(nextRay, depth + 1);
                    break;
                
                case Ray::RayType::SPECULAR:
                    indirectLight = material.getSpecularity(texcoord) * 
                                    this->traceRay(nextRay, depth + 1);
                    break;
                
                case Ray::RayType::TRANSMISSION:
                    indirectLight = material.Tr * this->traceRay(nextRay, depth + 1);
                    break;
                default:
                    indirectLight = Vec3f{0};
                    break;
            }

            
            if (!(material.emissive))
                directLight = this->directLight(ray, intersection);
            else 
                directLight = Vec3f{0};

            ret =   material.Ke + 
                    directLight + 
                    indirectLight + 
                    material.getAmbience(texcoord) * this->scene->ambient;

        }
    } else {
        ret = this->scene->ambient;
    }

    return ret;
}


Vec3f PathTracer::directLight(Ray &ray, const Intersection &intersection) {
    Vec3f ret = Vec3f{0};
    const Material& material = *intersection.face->material;
    const Vec3f d = material.getDiffusion(intersection.getTexCoord());
    if ( d != Vec3f{0} ) {
        /* 对场景中的光源进行随机采样 */
        for (size_t _ = 0; _ < lightSamples; ++_) 
        {   
            Point3f rgbLocal = Point3f{0};
            const LightArea& lightArea = *(this->scene->model.getRandomLightArea());
            const float areaSize = lightArea.area;
            // const Face& lightFace = *(lightArea.getRandomFaceByWeights());
            const Face& lightFace = *(lightArea.getRandomFace());
            const Point3f lightPoint = lightFace.randomSample();
            const Vec3f lightDirection = lightPoint - intersection.point;
            Ray lightRay = Ray(
                intersection.point, 
                lightDirection
            );

            if ( Intersection tmpInst;
                    this->scene->hit(lightRay, tmpInst)  &&  
                    // tmpInst.face == intersection.face
                    vecClose(tmpInst.point, intersection.point) )
            {
                const Vec3f lightDirNormalized = normalize(lightDirection);
                const float cos_i = std::max(glm::dot(intersection.normal, lightDirNormalized), 0.0f);
                const float cos_o = std::max(glm::dot(-lightDirNormalized, lightFace.getNormal()), 0.0f);
                const float f0 = cos_i * cos_o / (glm::pow2(lightDirection.length()));
                const Vec3f energy = f0 * areaSize * (lightArea.material->Ke) / lightSamples;
                
                const float cos = glm::dot(lightDirNormalized, intersection.normal);
                if (cos > 0.f) {
                    rgbLocal += cos * d * energy / glm::pi<float>();
                }
            }
            
            ret += rgbLocal;
        }
        ret = glm::clamp(ret, Vec3f{0.f}, Vec3f{1.f});
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
    #pragma omp parallel for num_threads(20)
    for (ssize_t y = 0; y < static_cast<ssize_t>(this->scene->height); ++y)
    {
        #pragma omp parallel for num_threads(20)
        for (ssize_t x = 0; x <  static_cast<ssize_t>(this->scene->width); ++x)
        {
            /* 对当前坐标进行光线追踪 */
            Ray ray = this->scene->getRay(x, y);
            Vec3f color = traceRay(ray, 0);

            /* 更新当前坐标的帧缓存 */
            const size_t idx = this->scene->frameBuffer.calculateIndex(x, y);
            this->scene->frameBuffer[idx] = Vec3f{
                this->scene->frameBuffer[idx] * (iterCnt - 1) + color
            } / iterCnt;
        }
    }


    LOG_INFO("Iteration : %zu", this->iterCnt);

    // auto t_end = std::chrono::high_resolution_clock::now();
    // float tloop = std::chrono::duration_cast<std::chrono::duration<float>>(t_end - t_start).count();
    this->timer.stop();
    // time_sum += tloop;
    
    LOG_INFO("Time: %f s Sum: %f s", 
        this->timer.getElapsedTime(), 
        this->timer.getTotalTime());
    
    if (filterFrequence > 0 && (this->iterCnt) % filterFrequence == 0) 
    {
        LOG_INFO("Applying Bilateral Filter...");
        this->bilateralFilter.applyBilateralFilter(
            this->scene->frameBuffer
        );
    }

    if (saveImgFrequence > 0 && (this->iterCnt) % saveImgFrequence == 0)
    {
        std::filesystem::path p = 
            std::filesystem::path(this->saveImgPath) / (std::to_string(this->iterCnt) + ".png");
        LOG_INFO("Saving to image %s .", p.string().c_str());
        this->scene->frameBuffer.writeToFile(p);
    }

    return;
}
