#include "defs.h"
#include "utils.h"
#include "Model.h"
#include "ui.h" /* for camera and etc, not for UI */

#include <iostream>
#include <string>
#include <chrono>
#include <map>
#include <filesystem>

static constexpr INT width = 800;
static constexpr INT height = 600;

static const std::map<ZBufferChoice, std::string> ChoiceName =
    {
        {ZBufferChoice::NONE_ZBUFFER, "NoZBuffer"},
        {ZBufferChoice::NAIVE_ZBUFFER, "NaiveZBuffer"},
        {ZBufferChoice::SCANLINE_ZBUFFER, "ScanlineZBuffer"},
        {ZBufferChoice::HIERARCHICAL_ZBUFFER, "HierarchialZBuffer"},
        {ZBufferChoice::KDTREE_HIERARCHICAL_ZBUFFER, "KDTreeHierchialZBuffer"}};

static void showHelp()
{
    std::cout <<
        R"(Usage: program [OPTIONS]
Options:
  --file -F          .obj file path
  --yaw -Y           angle of yaw in radians (float)
  --pitch -P         angle of pitch in radians (float)
  --output -O        output image path, with .bmp as suffix, default is ./output.bmp
  --zbuffer -Z       Z-buffer choice (no, naive, scanline, hierarchical, kdtree)
  --all              run all Z-buffer choices, and ignore the --zbuffer argument.
  --help -H          show this help information
)";
    return;
}

static int parseArgument(
    _In_ int argc,
    _In_ char **argv,
    _Out_ std::filesystem::path &filePath,
    _Out_ FLOAT &yaw,
    _Out_ FLOAT &pitch,
    _Out_ std::filesystem::path &outputPath,
    _Out_ ZBufferChoice &zBufferChoice,
    _Out_ bool &runAllZBuffers)
{
    /* 设置默认值 */
    yaw = pitch = 0.0f;
    outputPath = "./output.bmp";
    zBufferChoice = ZBufferChoice::NAIVE_ZBUFFER;
    runAllZBuffers = false;

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "--file" || arg == "-F")
        {
            if (i + 1 < argc)
            {
                filePath = argv[++i];
                if (!(filePath.extension() == ".obj"))
                {
                    LOG_ERROR("Invalid file! .obj required.");
                    return EXIT_FAILURE;
                }
            }
            else
            {
                LOG_ERROR("Error: Missing file path.");
                return EXIT_FAILURE;
            }
        }
        else if (arg == "--yaw" || arg == "-Y")
        {
            if (i + 1 < argc)
            {
                try
                {
                    yaw = std::stof(argv[++i]);
                }
                catch (std::invalid_argument &e)
                {
                    LOG_ERROR("Invalid argument --yaw! Exception info: %s.", e.what());
                    return EXIT_FAILURE;
                }
            }
            else
            {
                LOG_WARNING("Missing yaw angle. Using 0.0f by default.");
            }
        }
        else if (arg == "--pitch" || arg == "-P")
        {
            if (i + 1 < argc)
            {
                try
                {
                    pitch = std::stof(argv[++i]);
                }
                catch (std::invalid_argument &e)
                {
                    LOG_ERROR("Invalid argument --pitch! Exception info: %s.", e.what());
                    return EXIT_FAILURE;
                }
            }
            else
            {
                LOG_WARNING("Missing pitch angle. Using 0.0f by default.");
            }
        }
        else if (arg == "--output" || arg == "-O")
        {
            if (i + 1 < argc)
            {
                outputPath = argv[++i];
            }
            else
            {
                LOG_WARNING("Midding output path! Using ./output.bmp by default.");
            }
            if (!outputPath.has_extension())
            {
                if (!std::filesystem::is_directory(outputPath))
                {
                    std::filesystem::create_directory(outputPath);
                }
                outputPath = outputPath / "output.bmp";
            }
            else if (!(outputPath.extension() == ".bmp"))
            {
                LOG_ERROR("Suffix .bmp required in output path!");
                return EXIT_FAILURE;
            }
        }
        else if (arg == "--zbuffer" || arg == "-Z")
        {
            if (i + 1 < argc)
            {
                const std::string option = stringToLower(std::string{argv[++i]});
                if (option == "no")
                {
                    zBufferChoice = ZBufferChoice::NONE_ZBUFFER;
                }
                else if (option == "naive")
                {
                    zBufferChoice = ZBufferChoice::NAIVE_ZBUFFER;
                }
                else if (option == "scanline")
                {
                    zBufferChoice = ZBufferChoice::SCANLINE_ZBUFFER;
                }
                else if (option == "hierarchical")
                {
                    zBufferChoice = ZBufferChoice::HIERARCHICAL_ZBUFFER;
                }
                else if (option == "kdtree")
                {
                    zBufferChoice = ZBufferChoice::KDTREE_HIERARCHICAL_ZBUFFER;
                }
                else
                {
                    zBufferChoice = ZBufferChoice::NAIVE_ZBUFFER;
                    LOG_WARNING("Missing z-buffer option, using naive zbuffer by default.");
                }
            }
            else
            {
                zBufferChoice = ZBufferChoice::NAIVE_ZBUFFER;
                LOG_WARNING("Missing z-buffer option, using naive zbuffer by default.")
            }
        }
        else if (arg == "--all")
        {
            runAllZBuffers = true;
        }
        else if (arg == "--help" || arg == "-H" || arg == "-h")
        {
            showHelp();
            return EXIT_FAILURE; /* end process */
        }
        else
        {
            LOG_ERROR("Unknown option: %s", arg.c_str());
            return EXIT_FAILURE;
        }
    }
    if (filePath.empty())
    {
        LOG_ERROR("Missing required option --file/-F.");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

void renderWithZBufferChoice(Model &model, const std::filesystem::path &outputPath, ZBufferChoice zBufferChoice)
{
    ZBuffer *zBuffer = nullptr;

    switch (zBufferChoice)
    {
    case ZBufferChoice::NONE_ZBUFFER:
    {
        zBuffer = new ZBuffer{width, height};
        break;
    }
    case ZBufferChoice::NAIVE_ZBUFFER:
    default:
    {
        zBuffer = new NaiveZBuffer{width, height};
        break;
    }
    case ZBufferChoice::SCANLINE_ZBUFFER:
    {
        zBuffer = new ScanLineZBuffer{width, height};
        break;
    }
    case ZBufferChoice::HIERARCHICAL_ZBUFFER:
    {
        zBuffer = new HierarchicalZBuffer{width, height};
        break;
    }
    case ZBufferChoice::KDTREE_HIERARCHICAL_ZBUFFER:
    {
        zBuffer = new KDTreeHierarchicalZBuffer{width, height};
        break;
    }
    }

    auto start = std::chrono::steady_clock::now();
    zBuffer->renderModel(model);
    auto end = std::chrono::steady_clock::now();
    double duration = std::chrono::duration<double>(end - start).count();
    LOG_INFO("Elapsed: %.3f seconds for %s.", duration, ChoiceName.at(zBufferChoice).c_str());

    int result = saveImage(outputPath.string().c_str(), zBuffer->getFrameBuffer(), width, height);

    if (result == EXIT_FAILURE)
    {
        LOG_ERROR("Failed to save image %s.", outputPath.string().c_str());
    }
    else
    {
        LOG_INFO("Successfully saved image %s.", outputPath.string().c_str());
    }
    delete zBuffer;
}

int main(int argc, char *argv[])
{
    std::filesystem::path filePath, outputPath;
    FLOAT yaw, pitch;
    ZBufferChoice zBufferChoice;
    bool runAllZBuffers;
    
    int result = parseArgument(
        argc, argv,
        filePath,
        yaw,
        pitch,
        outputPath,
        zBufferChoice,
        runAllZBuffers);

    if (result == EXIT_FAILURE)
    {
        return 0;
    }

    LOG_INFO("Screen resolution: %d x %d.", width, height);

    Model model{filePath.string()};
    Shader shader;
    Camera camera;
    Matrix4x4f rotationMatrix;

    shader.shadeModel(model);
    camera.lookAt(yaw, pitch);
    rotationMatrix = camera.getViewMatrix();
    if (!D3DXMatrixIsIdentity(&rotationMatrix))
        model.transfrom(rotationMatrix);
    model.project(width, height);

    if (runAllZBuffers)
    {
        for(const auto& [choice, name] : ChoiceName) {
            renderWithZBufferChoice(
                model,
                outputPath.parent_path() / (outputPath.stem().string() + "_" + name + ".bmp"),
                choice
            );
        }
    }
    else
    {
        renderWithZBufferChoice(model, outputPath, zBufferChoice);
    }

    return EXIT_SUCCESS;
}
