#pragma once

#include "Defs.h"
#include "Scene.h"
#include "PathTracer.h"

#include <vector>
#include <filesystem>

/*  加载给定的模型 (.obj)、设置参数 (.xml) 和材质 (.mtl) 文件,
    并初始化场景 (scene) 和路径追踪器 (PathTracer)。 */
class SceneLoader {
public:
    SceneLoader(
        const std::string& objFilePath, 
        const std::string& configFilePath, 
        const std::string& mtlFilePath,
        const std::string& sceneName = ""
    ) 
        : objFilePath(objFilePath), configFilePath(configFilePath), mtlFilePath(mtlFilePath), sceneName(sceneName)
    {
        ASSERT(
            this->objFilePath.extension() == ".obj" &&
            (this->configFilePath.extension() == ".xml" ) &&
            this->mtlFilePath.extension() == ".mtl",
            "Invalid file extension!"
        );

        /* 相对路径转换成绝对路径 */
        if (this->objFilePath.is_relative())
        {
            this->objFilePath = std::filesystem::path(PROJECT_SOURCE_DIR) / (this->objFilePath);
        }
        if (this->configFilePath.is_relative())
        {
            this->configFilePath = std::filesystem::path(PROJECT_SOURCE_DIR) / (this->configFilePath);
        }
        if (this->mtlFilePath.is_relative())
        {
            this->mtlFilePath = std::filesystem::path(PROJECT_SOURCE_DIR) / (this->mtlFilePath);
        }
    }

    /*
    SceneLoader(
        const std::string& scenePath,
        const std::string& sceneName
    )
        :   objFilePath(std::filesystem::path(scenePath) / (sceneName + ".obj")),
            configFilePath(std::filesystem::path(scenePath) / (sceneName + ".xml")),
            mtlFilePath(std::filesystem::path(scenePath) / (sceneName + ".mtl")),
            sceneName(sceneName)
    {} */

    // DISALLOW_COPY_AND_MOVE(SceneLoader);

    /* 使用 .obj 文件和 .mtl 文件加载模型到 Scene 中 */
    bool loadScene(Scene& scene) const {
        bool res = this->loadOBJFile(scene);
        // res &= this->LoadConfigFile(scene);

        if (! res) {
            LOG_ERROR("Load Scene %s Fails!", sceneName.c_str());
        }
        return res;
    }

    /* 分别加载参数 (相机、场景、路径追踪等) 到 scene 和 pathTracer 中 */
    bool loadConfig(Scene& scene, PathTracer& pathTracer) const {
        bool res = this->loadConfigFile(scene, pathTracer);
        if (! res) {
            LOG_ERROR("Load Config File %s Fails!", configFilePath.string().c_str());
        }
        
        scene.model.initLights();
        return res;
    }

private:
    bool loadOBJFile(Scene& scene) const ;
    bool loadConfigFile(Scene& scene, PathTracer& pathTracer) const ;
    
    std::filesystem::path objFilePath;
    std::filesystem::path configFilePath;
    std::filesystem::path mtlFilePath;
    const std::string sceneName;
};