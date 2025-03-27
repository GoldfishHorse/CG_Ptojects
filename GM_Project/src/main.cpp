#include "GLProgram.h"
#include <string>

#include "Defs.h"
#include "Utils.h"
#include "MyTinyNurbs.h"
#include "SweptSurface.h"

#include <glm/gtc/constants.hpp>

#include <string_view>
#include <cstdlib>

/* 选择生成扫掠曲线和曲面的方法 */
static SweptSurfaceChoice sschoice = SweptSurfaceChoice::Skinning;
static FrameChoice fchoice = FrameChoice::RotationMinimization;
static bool doTransform = false;
static int subTask = 1;

/* 子任务 1 */
static SweptSurface getSurface1(bool doTransform = false)
{
    static constexpr float PI = glm::pi<float>();
    static auto const shape_controller = [](const float t)
    {
        return Mat4f(std::cos(PI * t), -std::sin(PI * t), 0.0f, 0.0f,
                     std::sin(PI * t), std::cos(PI * t), 0.0f, 0.0f,
                     0.0f, 0.0f, 1.0f, 0.0f,
                     0.0f, 0.0f, 0.0f, 1.0f);
    };

    static const RationalCurve3f contour{
        SFINIT(.degree, 2),
        SFINIT(.knots, arrayf{0, 0, 0, 0.15, 0.3, 0.4, 0.6, 0.7, 0.85, 1, 1, 1}),
        SFINIT(.control_points, arrayVec3f{
                                    Vec3f(0, -0.5, 0.5), Vec3f(0, -0.5, 1), Vec3f(0, 1, 1), Vec3f(0, 1, 0.7), Vec3f(0, 0, 0.5),
                                    Vec3f(0, 1, 0.3), Vec3f(0, 1, 0), Vec3f(0, -0.5, 0), Vec3f(0, -0.5, 0.5)}),
        SFINIT(.weights, arrayf{1, 1, 1, 1, 1, 1, 1, 1, 1})};

    static const RationalCurve3f trace{
        SFINIT(.degree, 3),
        SFINIT(.knots, arrayf{0, 0, 0, 0, 0.33, 0.66, 1, 1, 1, 1}),
        SFINIT(.control_points, arrayVec3f{
                                    Vec3f(0, 0, 0), Vec3f(1, 0.5, 0.5), Vec3f(2, 1, 0),
                                    Vec3f(3, 0.5, -0.5), Vec3f(4, 0, 0), Vec3f(5, -1, 1)}),
        SFINIT(.weights, arrayf{1, 1, 1, 1, 1, 1})};

    return doTransform ? SweptSurface{contour, trace, shape_controller} : SweptSurface{contour, trace};
}

/* 子任务 2 */
static SweptSurface getSurface2(bool doTransform = false)
{
    static constexpr float PI = glm::pi<float>();
    static auto const shape_controller = [](const float t)
    {
        return Mat4f(std::cos(PI * t), -std::sin(PI * t), 0.0f, 0.0f,
                     std::sin(PI * t), std::cos(PI * t), 0.0f, 0.0f,
                     0.0f, 0.0f, 1.0f, 0.0f,
                     0.0f, 0.0f, 0.0f, 1.0f);
    };
    static const RationalCurve3f contour{
        SFINIT(.degree, 3),
        SFINIT(.knots, arrayf{0, 0, 0, 0, 0.2, 0.4, 0.6, 0.8, 1, 1, 1, 1}),
        SFINIT(.control_points, arrayVec3f{
                                    Vec3f(0, 0, 0), Vec3f(1, 2, 0), Vec3f(2, -1, 0), Vec3f(3, 1, 0),
                                    Vec3f(4, -2, 0), Vec3f(5, 2, 0), Vec3f(6, -1, 0), Vec3f(7, 0, 0)}),
        SFINIT(.weights, arrayf{1.0, 1.1, 1.0, 0.9, 0.9, 1.0, 1.0, 1.0})};

    static const RationalCurve3f trace{
        SFINIT(.degree, 2),
        SFINIT(.knots, arrayf{0, 0, 0, 0.2, 0.4, 0.6, 0.8, 1, 1, 1}),
        SFINIT(.control_points, arrayVec3f{
                                    Vec3f(2, 3, 4), Vec3f(0, 3, 4), Vec3f(0, 3, 0), Vec3f(3, 3, 0),
                                    Vec3f(3, 0, 0), Vec3f(3, 0, 2), Vec3f(5, 0, 2)}),
        SFINIT(.weights, arrayf{1.0, 1.2, 1.2, 1.0, 1.0, 1.0, 1.0})};

    return doTransform ? SweptSurface{contour, trace, shape_controller} : SweptSurface{contour, trace};
}

static inline SweptSurface getSurface(int num, bool doTransform = false)
{
    switch (num)
    {
        case 1: default:
            return getSurface1(doTransform);
        case 2:
            return getSurface2(doTransform);
    }
}

static inline void printHelp() {
    puts(R"(Usage: ./project.exe [options]
Options:
  -s, --surface [0|1|2]  Set Swept Surface Algorithm (0: Approximation, 1: Offset, 2: Skinning), 2 by default.
  -f, --frame [0|1|2]    Set Frame Algorithm (0: FrenetSerret, 1: ProjectionNormal, 2: RotationMinimization), 2 by default.
  -t, --task [1|2]       Select surface to display (between 2 selections), 1 by default.
  --transfrom            Allowing shape control.
  -h, --help             Show this help message.)");
    return;
}


static void parseArguments(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg = argv[i];
        if ((arg == "-s" || arg == "--surface") && i + 1 < argc) {
            const int value = std::stoi(argv[++i]);
            switch (value) {
                case 0:
                    ::sschoice = SweptSurfaceChoice::Approximation;
                    break;
                case 1:
                    ::sschoice = SweptSurfaceChoice::Offset;
                    break;
                case 2:
                    ::sschoice = SweptSurfaceChoice::Skinning;
                    break;
                default:
                    LOG_WARNING("Invalid value for -s or --surface. Using 2 by default.");
                    break;
            }
        } else if ((arg == "-f" || arg == "--frame") && i + 1 < argc) {
            const int value = std::stoi(argv[++i]);
            switch (value) {
                case 0:
                    ::fchoice = FrameChoice::FrenetSerret;
                    break;
                case 1:
                    ::fchoice = FrameChoice::ProjectionNormal;
                    break;
                case 2:
                    ::fchoice = FrameChoice::RotationMinimization;
                    break;
                default:
                    LOG_WARNING("Invalid value for -f or --frame. Using 2 by default.");
                    break;
            }
        } else if ((arg == "-t" || arg == "--task") && i + 1 < argc) {
            const int value = std::stoi(argv[++i]);
            switch (value) {
                case 1:
                    ::subTask = 1;
                    break;
                case 2:
                    ::subTask = 2;
                    break;
                default:
                    LOG_WARNING("Invalid value for -t or --task. Using 1 by default.");
                    break;
            }
        }
        else if (arg == "--transfrom") {
            ::doTransform = true;
        }
        else if (arg == "-h" || arg == "--help") {
            printHelp();
            exit(0);
        } else {
            LOG_WARNING("Invalid option %s.", arg.data());
        }
    }
    return;
}

int main(int argc, char* argv[])
{
    parseArguments(argc, argv);

    GLProgram program;
    SweptSurface ss = getSurface(::subTask, ::doTransform);

    ss.generateSweptSurface(::sschoice, ::fchoice);
    const RationalCurve3f &contour = ss.getContourCurve();
    const RationalCurve3f &trace = ss.getTrajectoryCurve();
    const RationalSurface3f &sweep = ss.getSweptSurface();

    const auto profiles = ss.generateProfileCurves();
    const auto frames = ss.getFrames();
    const arrayf &v_bar = ss.getVbar();

    if (
        const auto pts = Checker::surfaceIsSelfIntersection(sweep, 300ULL, 600ULL);
        pts.has_value())
    {
        const auto &[p1, p2] = pts.value();
        LOG_WARNING(
            "The surface has self-intersection! The intersection point are (u1, v1) = (%f, %f) and (u2, v2) = (%f, %f)",
            p1.x, p1.y, p2.x, p2.y);
    }
    program.init({contour}, {trace}, {profiles}, {frames}, {sweep});
    program.setClearColor(0.05f, 0.18f, 0.25f, 1.0f);
    program.run({contour}, {trace}, {profiles}, {frames}, {v_bar}, {sweep});
    program.cleanup();

    return 0;
}