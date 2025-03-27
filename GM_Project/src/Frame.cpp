#include "Frame.h"
#include "MyTinyNurbs.h"

#include <glm/gtx/quaternion.hpp> /* for glm::length2 */

#include <algorithm>
#include <stdexcept>

const std::vector<arrayVec3f> MovingFrame::getFramesVec() const
{
    if (this->frameSeries.empty())
    {
        LOG_WARNING("Empty frameseries!");
        return {};
    } else {
        std::vector<arrayVec3f> ret;
        ret.resize(this->frameSeries.size());
        std::transform(
            this->frameSeries.cbegin(),
            this->frameSeries.cend(),
            ret.begin(),
            [](const Frame& frame) -> arrayVec3f
            {
                return arrayVec3f(frame);
            });
        return ret;
    }
}

void MovingFrame::ComputeFrameSeries(const FrameChoice choice, const arrayf &params)
{
    if (!this->frameSeries.empty())
    {
        LOG_WARNING("Re-computing frame!");
        this->frameSeries.clear();
    }

    switch (choice)
    {
    case FrameChoice::FrenetSerret:
        this->FrenetSerretFrameSeries(params);
        break;
    case FrameChoice::ProjectionNormal:
        this->ProjectionNormalFrameSeries(params);
        break;
    case FrameChoice::RotationMinimization:
        this->RotationMinimizationFrameSeries(params);
        break;
    default:
        constexpr const char* err = "Invalid choice!";
        LOG_ERROR("%s", err);
        throw std::invalid_argument(err);
    }
    return;
}

Frame MovingFrame::ComputeSingleFrame (const FrameChoice choice, const float u) const
{
    Frame ret;
    switch (choice)
    {
    case FrameChoice::FrenetSerret:
        ret = this->FrenetSerretFrame(u);
        break;
    case FrameChoice::ProjectionNormal:
        ret = this->ProjectionNormalFrame(u);
        break;
    case FrameChoice::RotationMinimization:
        ret = this->RotationMinimizationFrame(u);
        break;
    default:
        constexpr const char* err = "Invalid choice!";
        LOG_ERROR("%s", err);
        throw std::invalid_argument(err);
    }
    return ret;
}

Frame MovingFrame::FrenetSerretFrame(const float u) const
{
    const arrayVec3f ders = this->curve.Derivatives(2, u);
    // const Vec3f N = normalize(ders[2]);         /* 主法向量 */
    // const Vec3f T = normalize(ders[1]);         /* 单位切向量 */
    // const Vec3f B = crossAndNormalize(T, N);    /* 次法向量 */

    Vec3f B = crossAndNormalize(ders[1], ders[2]);
    if (equalf(B.length(), 0.f)) 
        B = Vec3f{0.f, 1.f, 0.f};
    Vec3f T = normalize(ders[1]);
    if (equalf(T.length(), 0.f))
        T = Vec3f{0.f, 0.f, 1.f};
    Vec3f N =  crossAndNormalize(B, T);

    return Frame{
        SFINIT(.N,  std::move(N)), 
        SFINIT(.B , std::move(B)),
        SFINIT(.T,  std::move(T)), 
        SFINIT(.u,  u) };
}

void MovingFrame::FrenetSerretFrameSeries(const arrayf &params)
{
    if (!this->frameSeries.empty())
    {
        LOG_WARNING("Re-computing frame!");
        this->frameSeries.clear();
    }
    LOG_INFO("Using Frenet-Serret Method to Compute Frames...");
    arrayf params_copy{params};
    std::sort(params_copy.begin(), params_copy.end());

    this->frameSeries.resize(params_copy.size());
    std::transform(
        params_copy.cbegin(), params_copy.cend(),
        this->frameSeries.begin(),
        [&](const float u)
        { return this->FrenetSerretFrame(u); });
    return;
}

Frame MovingFrame::ProjectionNormalFrame(const float u) const
{

    Frame ret;
    if (this->frameSeries.empty())
    {
        ret = this->FrenetSerretFrame(u);
    }
    else
    {
        if (
            const auto it = std::lower_bound(
                this->frameSeries.crbegin(),
                this->frameSeries.crend(),
                u,
                [](const Frame &frame, const float u1)
                {
                    // return frame.u < u1;
                    return u1 < frame.u;
                });
            it != this->frameSeries.crend())
        {
            if (equalf(it->u, u))
                ret = *it ;
            else 
            {
                Vec3f tmpT = this->curve.Derivatives(1, u)[1];
                Vec3f T = !equalf(tmpT.length(), 0.f) ? normalize(tmpT) : Vec3f{0.f, 0.f, 1.f};
                Vec3f tmpB = it->B - (glm::dot(it->B, T) * T);
                Vec3f B = !equalf(tmpB.length(), 0.f) ? normalize(tmpB) : Vec3f{0.f, 1.f, 0.f};
                Vec3f N = crossAndNormalize(T, B);

                ret = Frame{
                        SFINIT(.N,  std::move(N)), 
                        SFINIT(.B , std::move(B)),
                        SFINIT(.T,  std::move(T)), 
                        SFINIT(.u,  u) };
            }
        }
        else
        {
            ret = this->FrenetSerretFrame(u);
        }
    }
    return ret;
}

void MovingFrame::ProjectionNormalFrameSeries(const arrayf &params)
{
    if (!this->frameSeries.empty())
    {
        LOG_WARNING("Re-computing frame!");
        this->frameSeries.clear();
    }
    LOG_INFO("Using Projection Normal Method to Compute Frames...");
    arrayf params_copy{params};
    std::sort(params_copy.begin(), params_copy.end());
    this->frameSeries.resize(params_copy.size());
    this->frameSeries.front() = this->FrenetSerretFrame(params_copy.front());
    for (size_t i = 1; i < params_copy.size(); ++i) 
    {
        const Frame &previous = this->frameSeries[i - 1];

        Vec3f tmpT = this->curve.Derivatives(1, params_copy[i])[1];
        Vec3f T = !equalf(tmpT.length(), 0.f) ? normalize(tmpT) : Vec3f{0.f, 0.f, 1.f};
        Vec3f tmpB = previous.B - (glm::dot(previous.B, T) * T);
        Vec3f B = !equalf(tmpB.length(), 0.f) ? normalize(tmpB) : Vec3f{0.f, 1.f, 0.f};
        Vec3f N = crossAndNormalize(T, B);
        this->frameSeries[i] = Frame{
            SFINIT(.N,  std::move(N)), 
            SFINIT(.B , std::move(B)),
            SFINIT(.T,  std::move(T)), 
            SFINIT(.u,  params_copy[i]) };
    }
    return;
}


/* 使用欧拉方法迭代
    https://en.wikipedia.org/wiki/Euler_method */
Frame MovingFrame::RMFSolver(const Vec3f& B0, const float t0, const float tend, const float dt) const
{
    ASSERT(dt > 0.f, "Invalid parameter dt = %f!", dt);
    Vec3f b = B0, dr1, dr2, db;
    for (float t = t0; t < tend; t += dt)
    {
        const arrayVec3f ders = this->curve.Derivatives(2, t);
        dr1 = ders[1], dr2 = ders[2];
        if (equalf(dr1.length(), 0.f))
            dr1 = Vec3f{0.f, 0.f, 1.f};

        db = -(glm::dot(b, dr2) / glm::length2(dr1)) * dr1; /* 欧拉方法迭代一步 */
        b += db * dt;
    }
    const Vec3f T = normalize(dr1);
    const Vec3f B = normalize(b);
    const Vec3f N = crossAndNormalize(T, B);
    return Frame{
        SFINIT(.N,  std::move(N)), 
        SFINIT(.B , std::move(B)),
        SFINIT(.T,  std::move(T)), 
        SFINIT(.u,  tend) };
}

/*  Klok, F. (1986). Two moving coordinate frames for sweeping along a 3D trajectory.
    Computer Aided Geometric Design, 3(3), 217–229. doi:10.1016/0167-8396(86)90039-7  */
/*  if we denote the space curve by r(t),
    this normalized vector field is a solution to the differential equation
    f'(t) = -((f(t) · r''(t))/|| r'(t) ||^2) r'(t)
    Then, the triad of orthonormal vectors {u,f,u × f}, where u(t) is the unit tangent of the curve r(t),
    forms an RMF of r(t).

    c.f. Han, C. Y. (2008). Nonexistence of rational rotation-minimizing frames on cubic curves.
        Computer Aided Geometric Design, 25(4-5), 298–304. doi:10.1016/j.cagd.2007.09.006 
    or, Bloomenthal, M., & Riesenfeld, R. F. (1992). Approximation of sweep surfaces by tensor product NURBS. 
        Curves and Surfaces in Computer Vision and Graphics II. doi:10.1117/12.135162  */
void MovingFrame::RotationMinimizationFrameSeries(const arrayf& params) 
{
     if (!this->frameSeries.empty())
    {
        LOG_WARNING("Re-computing frame!");
        this->frameSeries.clear();
    }
    LOG_INFO("Using Rotation Minimization Method to Compute Frames...");
    arrayf params_copy{params};
    std::sort(params_copy.begin(), params_copy.end());
    this->frameSeries.resize(params_copy.size());
    this->frameSeries.front() = this->FrenetSerretFrame(params_copy.front());
    constexpr float dt = 0.01f;
    for (size_t i = 1; i < params_copy.size(); ++i)
    {
        this->frameSeries[i] = this->RMFSolver(
             this->frameSeries[i - 1].B, 
             this->frameSeries[i - 1].u, 
             params_copy[i],
             dt
        );
    }
    return;
}

Frame MovingFrame::RotationMinimizationFrame(const float u) const
{
    Frame ret;

    static constexpr float dt = 0.01f; /* 迭代步长 */

    if (this->frameSeries.empty())
    {
        ret = this->FrenetSerretFrame(u);
    }
    else
    {
        if (
            const auto it = std::lower_bound(
                this->frameSeries.crbegin(),
                this->frameSeries.crend(),
                u,
                [](const Frame &frame, const float u1)
                {
                    return u1 < frame.u;
                });
            it != this->frameSeries.crend())
        {
            // LOG_DEBUG("it->u = %f", it->u);
            // LOG_DEBUG("input u = %f", u);
            /* 从前一个点开始来迭代计算 */
            if (equalf(it->u, u))
                ret = *it ;
            else
                ret = this->RMFSolver(it->B, it->u, u, dt);
        } else {
            ret = this->FrenetSerretFrame(u);
        }
    }
    return ret;
}