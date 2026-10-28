#include "SceneLoader.h"
#include "SceneViewer.h"

#include <cstdlib>
#include <filesystem>
#include <string>

namespace {

struct CommandLineOptions {
    std::string objFilePath;
    std::string configFilePath;
    std::string mtlFilePath;
    std::string sceneName;
    std::filesystem::path outputPath{"result/headless.png"};
    size_t iterations = 1;
    bool headless = false;
};

enum class ParseResult { success, help, error };

void printHelp()
{
    std::puts(R"(
CPU Monte Carlo Path Tracer

Usage:
  cpu_path_tracer [scene options] [render options]

Scene options:
  --obj, -obj <path>        OBJ geometry file
  --config, -config <path>  XML render configuration
  --mtl, -mtl <path>        MTL material file
  --name, -n <name>         Scene/window name

Render options:
  --headless                Render without opening a window
  --iterations, -i <count>  Iterations for headless rendering (default: 1)
  --output, -o <path>       Headless PNG/JPG/BMP output path
  --help, -h                Show this help

With no scene options, the bundled example-scenes-cg24/veach-mis paths are used.)");
}

bool readValue(int argc, char* argv[], int& index, std::string& value, const char* option)
{
    if (index + 1 >= argc) {
        LOG_ERROR("Missing value for %s.", option);
        return false;
    }
    value = argv[++index];
    return true;
}

ParseResult parseCommandLine(int argc, char* argv[], CommandLineOptions& options)
{
    for (int index = 1; index < argc; ++index) {
        const std::string argument = stringToLower(argv[index]);
        if (argument == "--obj" || argument == "--objfilepath" || argument == "-obj") {
            if (!readValue(argc, argv, index, options.objFilePath, "--obj")) return ParseResult::error;
        } else if (argument == "--config" || argument == "--configfilepath" || argument == "-config") {
            if (!readValue(argc, argv, index, options.configFilePath, "--config")) return ParseResult::error;
        } else if (argument == "--mtl" || argument == "--mtlfilepath" || argument == "-mtl") {
            if (!readValue(argc, argv, index, options.mtlFilePath, "--mtl")) return ParseResult::error;
        } else if (argument == "--name" || argument == "--scenename" || argument == "-n") {
            if (!readValue(argc, argv, index, options.sceneName, "--name")) return ParseResult::error;
        } else if (argument == "--output" || argument == "-o") {
            std::string value;
            if (!readValue(argc, argv, index, value, "--output")) return ParseResult::error;
            options.outputPath = value;
        } else if (argument == "--iterations" || argument == "-i") {
            std::string value;
            if (!readValue(argc, argv, index, value, "--iterations")) return ParseResult::error;
            try {
                options.iterations = std::stoull(value);
            } catch (const std::exception&) {
                LOG_ERROR("Invalid iteration count: %s", value.c_str());
                return ParseResult::error;
            }
            if (options.iterations == 0) {
                LOG_ERROR("Iteration count must be greater than zero.");
                return ParseResult::error;
            }
        } else if (argument == "--headless") {
            options.headless = true;
        } else if (argument == "--help" || argument == "-h") {
            printHelp();
            return ParseResult::help;
        } else {
            LOG_ERROR("Unknown option: %s", argv[index]);
            return ParseResult::error;
        }
    }

    const bool noSceneArguments = options.objFilePath.empty() && options.configFilePath.empty() &&
                                  options.mtlFilePath.empty() && options.sceneName.empty();
    if (noSceneArguments) {
        options.objFilePath = "example-scenes-cg24/veach-mis/veach-mis.obj";
        options.configFilePath = "example-scenes-cg24/veach-mis/veach-mis.xml";
        options.mtlFilePath = "example-scenes-cg24/veach-mis/veach-mis.mtl";
        options.sceneName = "veach-mis";
    } else if (options.objFilePath.empty() || options.configFilePath.empty() ||
               options.mtlFilePath.empty() || options.sceneName.empty()) {
        LOG_ERROR("--obj, --config, --mtl and --name must be provided together.");
        return ParseResult::error;
    }
    return ParseResult::success;
}

} // namespace

int main(int argc, char* argv[])
{
    CommandLineOptions options;
    const ParseResult parseResult = parseCommandLine(argc, argv, options);
    if (parseResult == ParseResult::help)
        return EXIT_SUCCESS;
    if (parseResult == ParseResult::error)
        return EXIT_FAILURE;

    SceneLoader sceneLoader(
        options.objFilePath, options.configFilePath,
        options.mtlFilePath, options.sceneName);

    Scene scene;
    if (!sceneLoader.loadScene(scene))
        return EXIT_FAILURE;
    scene.model.buildKDTree();

    PathTracer pathTracer;
    if (!sceneLoader.loadConfig(scene, pathTracer))
        return EXIT_FAILURE;
    pathTracer.init(&scene);

    if (options.headless) {
        for (size_t iteration = 0; iteration < options.iterations; ++iteration)
            pathTracer.renderScene();
        if (!scene.frameBuffer.writeToFile(options.outputPath))
            return EXIT_FAILURE;
        LOG_INFO("Wrote %zu-iteration render to %s.",
                 options.iterations, options.outputPath.string().c_str());
        return EXIT_SUCCESS;
    }

    SceneViewer viewer(&pathTracer);
    viewer.viewScene();
    return EXIT_SUCCESS;
}
