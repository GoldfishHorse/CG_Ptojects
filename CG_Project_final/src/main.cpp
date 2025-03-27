#include "SceneLoader.h"
#include "Model.h"
#include "SceneViewer.h"

#include <string>


static void printHelp() {
    std::puts(R"(
Usage: main [options]
Options:
    --objFilePath, -obj <path>        Path to the .obj file.
    --configFilePath, -config <path>  Path to the config (.xml) file.
    --mtlFilePath, -mtl <path>        Path to the .mtl file.
    --sceneName, -n <name>            Name of the scene.
    --help, -h                        Print this help message.)");
    return;
}
/* 解析命令行参数 */
static bool parseCommandLineArgs(
    __IN__  int argc, 
    __IN__  char* argv[],
    __OUT__ std::string& objFilePath,
    __OUT__ std::string& configFilePath,
    __OUT__ std::string& mtlFilePath,
    __OUT__ std::string& sceneName
) {

    objFilePath =  "";
    configFilePath = "";
    mtlFilePath =  "";
    sceneName = "";

    for (int i = 1; i < argc; ++i) {
        std::string arg = stringToLower(std::string(argv[i]));
        if (arg == "--objfilepath" || arg == "-obj") {
            if (i + 1 < argc) {
                objFilePath = argv[++i];
            } else {
                LOG_ERROR("Error: Missing value for --objFilePath/-Obj option.");
                return false;
            }
        } else if (arg == "--configfilepath" || arg == "-config") {
            if (i + 1 < argc) {
                configFilePath = argv[++i];
            } else {
                LOG_ERROR("Error: Missing value for --configFilePath/-Config option.");
                return false;
            }
        } else if (arg == "--mtlfilepath" || arg == "-mtl") {
            if (i + 1 < argc) {
                mtlFilePath = argv[++i];
            } else {
                LOG_ERROR("Error: Missing value for --mtlFilePath/-Mtl option.");
                return false;
            }
        } else if (arg == "--scenename" || arg == "-n") {
            if (i + 1 < argc) {
                sceneName = argv[++i];
            } else {
                LOG_ERROR("Error: Missing value for --sceneName option.");
                return false;
            }
        } else if (arg == "--help" || arg == "-h") {
            printHelp();
            /* 应退出程序, 因此需要返回 false。 */
            return false; 
        } 
         else {
            LOG_ERROR("Error: Unknown option %s.", arg.c_str());
            return false;
        }
    }
    
    /* Argument Check */
    if (objFilePath.empty() &&  configFilePath.empty() && mtlFilePath.empty() && sceneName.empty() )
    {
        /* 所有参数都未给出, 设置为默认值 */
        objFilePath = "./example-scenes-cg24/veach-mis/veach-mis.obj";
        configFilePath = "./example-scenes-cg24/veach-mis/veach-mis.xml";
        mtlFilePath = "./example-scenes-cg24/veach-mis/veach-mis.mtl";
        sceneName = "veach-mis";
    } else if (sceneName.empty())
    {
        LOG_ERROR("Missing value for --sceneName option.");
        return false;
    } else if (objFilePath.empty() || configFilePath.empty(), mtlFilePath.empty())
    {
        /* 这三个参数必须同时全部给出, 不能仅给出某几个 */
        LOG_ERROR("Error: Missing required options (--objFilePath, --configFilePath, --mtlFilePath).");
        return false;
    }

    return true;
}

int main(int argc, char* argv[]) {
    /* 读取命令行参数 */
    std::string objFilePath, configFilePath,
                mtlFilePath, sceneName;
    bool success = parseCommandLineArgs(
        argc, argv,
        objFilePath, configFilePath,
        mtlFilePath, sceneName
    );
    if (success)
    {
        SceneLoader sceneLoader =  SceneLoader(
            objFilePath, configFilePath, 
            mtlFilePath, sceneName
        );

        /* 加载场景 */
        Scene scene;
        sceneLoader.loadScene(scene);
        scene.model.buildKDTree();

        /* 初始化 PathTracer */
        PathTracer pathTracer;
        sceneLoader.loadConfig(scene, pathTracer);
        pathTracer.init(&scene);

        /* 显示场景 */
        SceneViewer displayer;
        displayer.init(&pathTracer);
        displayer.viewScene();
    }
    return 0;
}