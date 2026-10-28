#include "SceneLoader.h"
#include "Material.h"

#include <tinyxml2.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <glm/gtc/type_ptr.hpp>

#include <vector>

bool SceneLoader::loadConfigFile(Scene &scene, PathTracer& pathTracer) const
{
    ASSERT(!scene.model.empty(), "Shall load .obj before load config file!");
    if (!std::filesystem::exists(configFilePath))
    {
        LOG_ERROR("Config file %s does not exist.", configFilePath.string().c_str());
        return false;
    }


    tinyxml2::XMLDocument config;
    config.LoadFile(configFilePath.string().c_str());
    if (config.Error())
    {
        LOG_ERROR("Fails to load config file %s: %s", configFilePath.string().c_str(), config.ErrorStr());
        return false;
    }


    /* 加载相机相关参数 */
    if (tinyxml2::XMLElement* cameraNode = config.FirstChildElement("camera"); cameraNode)
    {
        
        /* 相机类型 */
        std::string cameraType;
        if (const char* type = cameraNode->Attribute("type"); type)
        {
            cameraType = stringToLower(std::string(type));
        }
        else
        {
            LOG_WARNING("Missing camera type, using perspective camera by default.");
            cameraType = "perspective";
        }

        if (cameraType == "perspective")
        {
            if (float fovy = cameraNode->FloatAttribute("fovy"); fovy)
            {
                LOG_INFO("set fovy as %f.", fovy);
                scene.camera.fovy = fovy;
            }
            else
            {
                LOG_WARNING("Missing fovy! Using %f by default.", PerspectiveCamera::defaultFovy);
                scene.camera.fovy = PerspectiveCamera::defaultFovy;
            }

            /* 相机的朝向、向上轴以及位置 */
            Vec3f lookAt = {0.0f, 0.0f, -1.0f};
            Vec3f up = {0.0f, 1.0f, 0.0f};
            Vec3f eye {0.f};
            if (tinyxml2::XMLElement* eyeNode = cameraNode->FirstChildElement("eye");  eyeNode)
            {
                eye =
                    Vec3f{
                        eyeNode->FloatAttribute("x"),
                        eyeNode->FloatAttribute("y"),
                        eyeNode->FloatAttribute("z")
                    };
            } else {
                eye = Vec3f{0};
            }

            if (tinyxml2::XMLElement* lookAtNode = cameraNode->FirstChildElement("lookat"); lookAtNode)
            {
                lookAt = Vec3f{
                    lookAtNode->FloatAttribute("x"),
                    lookAtNode->FloatAttribute("y"),
                    lookAtNode->FloatAttribute("z"),
                };
            }
            if (tinyxml2::XMLElement* upNode = cameraNode->FirstChildElement("up");  upNode)
            {
                up = Vec3f{
                    upNode->FloatAttribute("x"),
                    upNode->FloatAttribute("y"),
                    upNode->FloatAttribute("z")
                };
            }
            
            LOG_INFO("Eye = (%f %f %f)", eye.x, eye.y, eye.z);
            LOG_INFO("LookAt = (%f %f %f).", lookAt.x, lookAt.y, lookAt.z);
            LOG_INFO("Up = (%f %f %f)", up.x, up.y, up.z);
            scene.camera.eye = eye;
            scene.camera.lookAt = lookAt;
            scene.camera.up = up;
        }
        else
        {
            LOG_ERROR("Invalid type of camera %s.", cameraType.c_str());
            // throw std::runtime_error("Invalid type of camera.");
            return false;
        }
        LOG_INFO("Loaded %s camera.", cameraType.c_str());
    }
    else
    {
        LOG_WARNING("Using default camera setting.");
        scene.camera = PerspectiveCamera();
    }

    /* 加载场景中光源的信息 */
    if (tinyxml2::XMLElement* lightNode = config.FirstChildElement("light"); lightNode)
    {

        /* 场景中可能有多个光源 */
        for (; lightNode; lightNode = lightNode->NextSiblingElement("light"))
        {
            const char* materialName = lightNode->Attribute("mtlname");
            if (materialName == nullptr)
            {
                LOG_WARNING("Ignoring a light without an mtlname attribute.");
                continue;
            }
            std::string mtlname{materialName};
            Vec3f radiance{0.0f}; /* 光源的辐射亮度 */
            if (const char* rad = lightNode->Attribute("radiance"); rad)
                radiance = cvtString2Vec3f(std::string(rad));
            if (auto it = scene.model.materials.find(mtlname); it != scene.model.materials.end())
#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
            [[likely]]
#endif
            {
                Material& material = it->second;
                material.emissive = true;
                material.Ke = radiance;
                LOG_INFO("Loaded light %s.", mtlname.c_str());
            }
            else 
#if defined(__has_cpp_attribute) && __has_cpp_attribute(unlikely)
            [[unlikely]]
#endif
            {
                /*  光源应该必然对应一个材质;
                    该分支理论上不应该走到。 */
                LOG_WARNING("Light %s not assigned to a material!", mtlname.c_str());
                Material material {mtlname};
                material.emissive = true;
                scene.model.addMaterial(material);

            }
        }
    }
    else
    {
        LOG_INFO("No lights configured.");
    }

    /* 加载配置文件中关于场景的信息 */
    if (tinyxml2::XMLElement* sceneNode = config.FirstChildElement("scene"); sceneNode)
    {
        size_t width, height; /* 窗口宽高 */
        Vec3f ambient;  /* 场景环境光 */
        if (sceneNode->Attribute("width"))
        {
            width = static_cast<size_t>(sceneNode->UnsignedAttribute("width"));
        }
        else
        {
            LOG_WARNING("Missing window width! Using %zu by default.", Scene::defaultWidth);
            width = Scene::defaultWidth;
        }

        if (sceneNode->Attribute("height"))
        {
            height = static_cast<size_t>(sceneNode->UnsignedAttribute("height"));
        }
        else
        {
            LOG_WARNING("Missing window height! Using %zu by default.", Scene::defaultHeight);
            height = Scene::defaultHeight;
        }

        if (const char * ambientStr = sceneNode->Attribute("ambient"); ambientStr)
        {
            ambient = cvtString2Vec3f(std::string(ambientStr));
        } else {
            ambient = Scene::defaultAmbient;
        }
        scene.init(width, height, ambient);
        LOG_INFO("Set scene width %zu, height %zu.", scene.width, scene.height);
        LOG_INFO("Set scene ambient (%f %f %f)", scene.ambient.r, scene.ambient.g, scene.ambient.b);
    } else {
        scene.init(Scene::defaultWidth, Scene::defaultHeight, Scene::defaultAmbient);
        LOG_INFO("Set scene width %zu, height %zu.", scene.width, scene.height);
        LOG_INFO("Set scene ambient (%f %f %f)", scene.ambient.r, scene.ambient.g, scene.ambient.b);
    }

    /* 加载光线追踪相关参数 */
    if (tinyxml2::XMLElement* pathTraceNode = config.FirstChildElement("pathtrace"); pathTraceNode)
    {
        /* 直接光照中采样的光源数目 */
        if (pathTraceNode->Attribute("lightsamples"))
        {
            pathTracer.lightSamples = static_cast<size_t>(
                pathTraceNode->UnsignedAttribute("lightsamples")
            );
            LOG_INFO("Set PathTrace direct light sample number as %zu", pathTracer.lightSamples);
        }
        /* traceRay 最大递归深度 */
        if (pathTraceNode->Attribute("maxdepth"))
        {
            pathTracer.maxDepth = static_cast<size_t>(pathTraceNode->UnsignedAttribute("maxdepth"));
            LOG_INFO("Set PathTrace max depth as %zu.", pathTracer.maxDepth);
        }
        /*  路径追踪每隔几轮迭代进行一次滤波;
            0 代表不滤波 */
        const char* filterAttribute = pathTraceNode->Attribute("filterfrequency")
            ? "filterfrequency" : "filterfrequence";
        if (pathTraceNode->Attribute(filterAttribute))
        {
            pathTracer.filterFrequency = static_cast<size_t>(
                pathTraceNode->UnsignedAttribute(filterAttribute));
            LOG_INFO("Set PathTrace filter frequency as %zu.", pathTracer.filterFrequency);
        }
        /*  路径追踪每隔几轮迭代保存一次图片
            0 代表不保存 */
        const char* saveAttribute = pathTraceNode->Attribute("saveimgfrequency")
            ? "saveimgfrequency" : "saveimgfrequence";
        if (pathTraceNode->Attribute(saveAttribute))
        {
            pathTracer.saveImgFrequency = static_cast<size_t>(
                pathTraceNode->UnsignedAttribute(saveAttribute));
            LOG_INFO("Set PathTrace save image frequency as %zu.", pathTracer.saveImgFrequency);
        }
        /* 保存图片的目录 */
        if (const char* p = pathTraceNode->Attribute("saveimgpath"); p)
        {
            pathTracer.saveImgPath = std::string(p);
            LOG_INFO("Set PathTrace save image path as %s.", pathTracer.saveImgPath.c_str());
        }
    }


    LOG_INFO("Load config file %s succeeds.", configFilePath.string().c_str());
    return true;
}

bool SceneLoader::loadOBJFile(Scene & scene) const
{
    if (!std::filesystem::exists(objFilePath))
    {
        LOG_ERROR(".obj file %s does not exist.", objFilePath.string().c_str());
        return false;
    }
    if (!std::filesystem::exists(mtlFilePath))
    {
        LOG_ERROR(".mtl file %s does not exist.", mtlFilePath.string().c_str());
        return false;
    }

    tinyobj::ObjReaderConfig objReaderConfig;
    tinyobj::ObjReader objReader;

    objReaderConfig.mtl_search_path = mtlFilePath.parent_path().string();
    if (!objReader.ParseFromFile(objFilePath.string(), objReaderConfig))
    {
        if (!objReader.Error().empty())
        {
            LOG_ERROR("Load .obj fails. Error Message: %s.", objReader.Error().c_str());
        }
        else
        {
            LOG_ERROR("Load .obj fails.");
        }
        return false;
    }
    else if (!objReader.Warning().empty())
    {
        LOG_WARNING("%s", objReader.Warning().c_str());
    }

    const auto &attrib = objReader.GetAttrib();
    const auto &shapes = objReader.GetShapes();
    const auto &materials = objReader.GetMaterials();

    Material* defaultMaterial = scene.model.fetchMaterial("__default__");
    if (defaultMaterial == nullptr)
    {
        Material material{"__default__"};
        material.Kd = Vec3f{0.8f};
        defaultMaterial = scene.model.addMaterial(std::move(material));
    }

    for (const auto &shape : shapes)
    {
        /* 初始化模型的各个面片, 并加入到 scene.model.faces 中 */
        for (size_t f = 0, idxOfs = 0; f < shape.mesh.num_face_vertices.size(); f++)
        {
            Face face{};
            face.material = defaultMaterial;
            const size_t fvs = shape.mesh.num_face_vertices[f];
            if (fvs != 3)
            {
                LOG_WARNING("Skipping non-triangle face with %zu vertices.", fvs);
                idxOfs += fvs;
                continue;
            }
            for (size_t v = 0; v < fvs; ++v)
            {
                tinyobj::index_t idx = shape.mesh.indices[idxOfs + v];
                face.vertices[v] = Vec3f{
                    attrib.vertices[ 3 * idx.vertex_index + 0],
                    attrib.vertices[ 3 * idx.vertex_index + 1],
                    attrib.vertices[ 3 * idx.vertex_index + 2],
                };
                if (idx.texcoord_index >= 0)
                {
                    face.texcoords[v] = Vec2f{
                        attrib.texcoords[2 * idx.texcoord_index + 0],
                        attrib.texcoords[2 * idx.texcoord_index + 1]};
                }
                if (idx.normal_index >= 0)
                {
                    face.normals[v] = Vec3f{
                        attrib.normals[3 * idx.normal_index + 0],
                        attrib.normals[3 * idx.normal_index + 1],
                        attrib.normals[3 * idx.normal_index + 2]};
                }
            }

            const Vec3f geometricNormal = face.getNormal();
            for (Vec3f& normal : face.normals)
                if (isZeroVec(normal))
                    normal = geometricNormal;

            if (!materials.empty())
            {
                int material_id = shape.mesh.material_ids[f];
                if (material_id >= 0 && static_cast<size_t>(material_id) < materials.size())
                {
                    const tinyobj::material_t &material = materials[material_id];

                    Material* pModelMaterial = scene.model.fetchMaterial(material.name);
                    
                    /* 此材质尚未加入到 model.materials 中 */
                    if (pModelMaterial == nullptr)
                    {
                        /* 新增材质并初始化相关参数 */
                        Material modelMaterial = Material{material.name};
                        modelMaterial.Ka = glm::make_vec3(material.ambient);
                        modelMaterial.Kd = glm::make_vec3(material.diffuse);
                        modelMaterial.Ks = glm::make_vec3(material.specular);
                        modelMaterial.Tr = glm::make_vec3(material.transmittance);
                        modelMaterial.Ns = (float)material.shininess;
                        modelMaterial.Ni = (float)material.ior;

                        /* 加载 map_Ka */
                        if (!material.ambient_texname.empty())
                        {
                            const std::filesystem::path ambientTexture {material.ambient_texname};
                            std::string mapPath;
                            if (ambientTexture.is_absolute())
                                mapPath = ambientTexture.string();
                            else 
                                mapPath = (mtlFilePath.parent_path() / ambientTexture).string();
                            Texture * pTextureKa = scene.model.fetchTexture(mapPath);
                            if (pTextureKa == nullptr) {
                                pTextureKa = scene.model.addTexture(Texture {mapPath});
                            }
                            modelMaterial.map_Ka = pTextureKa;
                        }
                        /* 加载 map_Kd */
                        if (!material.diffuse_texname.empty())
                        {
                            const std::filesystem::path diffuseTexture {material.diffuse_texname};

                            std::string mapPath;
                            if (diffuseTexture.is_absolute())
                                mapPath = diffuseTexture.string();
                            else
                                mapPath = (mtlFilePath.parent_path() / diffuseTexture).string();  

                            Texture * pTextureKd = scene.model.fetchTexture(mapPath);
                            if (pTextureKd == nullptr) {
                                pTextureKd = scene.model.addTexture(Texture {mapPath});
                            }
                            modelMaterial.map_Kd = pTextureKd;
                        }
                        /* 加载 map_Ks */
                        if(!material.specular_texname.empty())
                        {
                            const std::filesystem::path specularTexture {material.specular_texname};
                            std::string mapPath;
                            if (specularTexture.is_absolute())
                                mapPath = specularTexture.string();
                            else 
                                mapPath = (mtlFilePath.parent_path() / specularTexture).string();
                            Texture * pTextureKs = scene.model.fetchTexture(mapPath);
                            if (pTextureKs == nullptr) {
                                pTextureKs = scene.model.addTexture(Texture {mapPath});
                            }
                            modelMaterial.map_Ks = pTextureKs;
                        }
                        /* 将新材质加入到模型中 */
                        pModelMaterial = scene.model.addMaterial(modelMaterial);
                    }
                    
                    face.material = pModelMaterial;
                }
            }
            scene.model.faces.push_back(std::move(face));
            idxOfs += fvs;
        }
    }
    scene.name = this->sceneName;
    LOG_INFO("Loaded .obj file %s with %zu faces.", objFilePath.string().c_str(), scene.model.faces.size());
    LOG_INFO("Loaded .mtl file %s .", mtlFilePath.string().c_str());
    return true;
}
