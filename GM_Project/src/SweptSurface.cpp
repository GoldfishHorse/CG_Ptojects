#include "SweptSurface.h"

#include <algorithm>
#include <stdexcept>

void SweptSurface::init()
{
    this->swept_surf = RationalSurface3f{};
    this->v_bar.clear();
    this->frames.clear();
    return;
}

std::vector<RationalCurve3f> SweptSurface::generateProfileCurves() const
{
    ASSERT(!this->v_bar.empty(), "empty v_bar!");
    ASSERT(!this->frames.empty(), "empty frames!");
    LOG_INFO("Generating Profile Curves...");

    std::vector<RationalCurve3f> profile_crvs;
    const FrameSeries& frameSeries = this->frames.getFrameSeries();
    for (size_t i = 0; i < this->v_bar.size(); ++i)
    {
        RationalCurve3f profile_crv {
            SFINIT( .degree,  this->contour_crv.degree),
            SFINIT( .knots, this->contour_crv.knots),
            SFINIT( .control_points,  arrayVec3f(this->contour_crv.control_points.size(), Vec3f{0.f}) ),
            SFINIT( .weights,  this->contour_crv.weights)
        };
        for (size_t j = 0; j < this->contour_crv.control_points.size(); ++j)
        {
            profile_crv.control_points[j] =
                this->trajectory_crv.Point(this->v_bar[i]) +
                homogenousToCartesian(
                    Mat4f{frameSeries.at(i)} *
                    shape_controller(this->v_bar[i]) *
                    glm::transpose(Mat4f{frameSeries.front()}) *
                    cartesianToHomogenous(this->contour_crv.control_points[j], 1.f));
        }
        profile_crvs.push_back(std::move(profile_crv));
    }
    return profile_crvs;
}

const RationalSurface3f& SweptSurface::generateSweptSurface(const SweptSurfaceChoice sschoice, const FrameChoice fchoice)
{
    switch (sschoice)
    {
        case (SweptSurfaceChoice::Approximation):
            this->ApproximationMethod(fchoice);
            break;
        case (SweptSurfaceChoice::Offset):
            this->OffsetMethod(fchoice);
            break;
        case (SweptSurfaceChoice::Skinning):
            this->SkinningMethod(fchoice);
            break;
        default:
            constexpr const char* err = "Invalid choice!";
            LOG_ERROR("%s", err);
            throw std::invalid_argument(err);
            break;
    }
    return this->getSweptSurface();
}

void SweptSurface::ApproximationMethod(const FrameChoice choice)
{
    LOG_INFO("Using Approximation Method Generating Swept Surface...");
    const size_t u_num = this->contour_crv.control_points.size();
    const size_t v_num = this->trajectory_crv.control_points.size();
    const float u_stepsize = 1.f / static_cast<float>(u_num - 1);
    const float v_stepsize = 1.f / static_cast<float>(v_num - 1);

    this->v_bar.resize(v_num);
    std::generate(
        this->v_bar.begin(),
        this->v_bar.end(),
        [v_stepsize, v = -v_stepsize]() mutable
        {
            return (v += v_stepsize);
        });
    this->frames.ComputeFrameSeries(choice, v_bar);
    array2Vec3f surf_points(
        u_num, v_num, Vec3f{0.f});
    for (size_t i = 0; i < u_num; ++i)
    {
        for (size_t j = 0; j < v_num; ++j)
        {
            const float u = u_stepsize * i;
            const float v = v_stepsize * j;
            const Frame frame = this->frames.ComputeSingleFrame(choice, v);
            surf_points.at(i, j) =
                this->trajectory_crv.Point(v) +
                homogenousToCartesian(
                    glm::transpose(Mat4f{frame}) *
                    shape_controller(v) *
                    Mat4f{this->frames.getFrameSeries().front()} *
                    Vec4f{this->contour_crv.Point(u), 1.0f});
        }
    }
    /* 用数据点插值 */
    RationalSurface3f srf = GlobalSurfaceInterpolation(
        this->contour_crv.degree,
        this->trajectory_crv.degree,
        surf_points);

    /* 计算控制顶点的权重 */
    for (size_t i = 0; i < u_num; ++i)
        for (size_t j = 0; j < v_num; ++j)
            srf.weights.at(i, j) = this->contour_crv.weights[i] * this->trajectory_crv.weights[j];
    
    this->swept_surf = std::move(srf);
    return;
}

/* Bloomenthal, Mark, and Richard F. Riesenfeld. "Approximation of sweep surfaces by tensor product NURBS." 
    Curves and surfaces in Computer Vision and Graphics II. Vol. 1610. SPIE, 1992. */
void SweptSurface::OffsetMethod(const FrameChoice choice)
{
    LOG_INFO("Using Offset Method Generating Swept Surface...");
    const size_t u_num = this->contour_crv.control_points.size();
    const size_t v_num = this->trajectory_crv.control_points.size();
    const float v_stepsize = 1.f / static_cast<float>(v_num - 1);
    RationalSurface3f srf{
        this->contour_crv.degree,
        this->trajectory_crv.degree,
        this->contour_crv.knots,
        this->trajectory_crv.knots,
        array2Vec3f{u_num, v_num, Vec3f{0.f}},
        array2f{u_num, v_num, 0.f}};
    this->v_bar.resize(v_num);
    std::generate(v_bar.begin(), v_bar.end(), [v_stepsize, v = -v_stepsize]() mutable
                  { return v += v_stepsize; });

    this->frames.ComputeFrameSeries(choice, v_bar);

    for (size_t i = 0; i < u_num; ++i)
    {
        for (size_t j = 0; j < v_num; ++j)
        {
            const float v = this->v_bar[j];
            const Frame frame = this->frames.ComputeSingleFrame(choice, v);

            const Point3f V1 =
                homogenousToCartesian(
                    Mat4f{frame} *
                    shape_controller(v) *
                    glm::transpose( Mat4f{this->frames.getFrameSeries().front()} ) *
                    cartesianToHomogenous(this->contour_crv.control_points[i], 1.0f)  /* The offset vector */
                    );
            /* Frenet frame */
            const Frame fs_frame = this->frames.ComputeSingleFrame(FrameChoice::FrenetSerret, v);
            const Vec3f V2{
                glm::dot(fs_frame.N, V1), /* vn */
                glm::dot(fs_frame.B, V1), /* vb */
                glm::dot(fs_frame.T, V1)  /* vt */
            };

            const Point3f &E = this->trajectory_crv.control_points[j];
            const Point3f S = this->trajectory_crv.Point(v);

            const Vec3f Ntilde = fs_frame.N - this->trajectory_crv.Curvature(v) * (E - S);
            const float lambda = glm::length(Ntilde);

            // ASSERT(!equalf(lambda, 0.f), "Invalid point!");
            // const Vec3f Nhat = Ntilde / lambda;
            // const Vec3f That = glm::cross(Nhat, fs_frame.B);

            const Vec3f Nhat = equalf(lambda, 0.f) ? Vec3f{0.f} : Ntilde / lambda;
            const Vec3f That = glm::cross(Nhat, fs_frame.B);

            srf.control_points.at(i, j) = E + lambda * (V2.x * Nhat + V2.z * That) + V2.y * fs_frame.B;
            srf.weights.at(i, j) = this->contour_crv.weights[i] * this->trajectory_crv.weights[j];
        }
    }

    this->swept_surf = std::move(srf);
    return;
}

/* 《The NURBS Book》 Algorithm A10.1 */
void SweptSurface::SkinningMethod(const FrameChoice choice)
{
    LOG_INFO("Using Skinning Method Generating Swept Surface...");
    const size_t u_num = this->contour_crv.control_points.size();
    const size_t v_num = this->trajectory_crv.knots.size();
    const size_t q = this->trajectory_crv.degree;
    const size_t nsect = v_num - q - 1;
    // this->v_bar.resize(nsect);
    // this->v_bar.front() = 0.f; this->v_bar.back() = 1.f;
    this->v_bar = MeanKnotVector(this->trajectory_crv.knots, q, false);
    this->frames.ComputeFrameSeries(choice, v_bar);

    RationalSurface3f srf{
        this->contour_crv.degree,
        this->trajectory_crv.degree,
        this->contour_crv.knots,
        this->trajectory_crv.knots,
        array2Vec3f{u_num, nsect, Vec3f{0.f}},
        array2f{u_num, nsect, 0.f}};

    /* 生成 nsect 个实例 */
    std::vector<RationalCurve3f> profiles = this->generateProfileCurves();

    ASSERT(profiles.size() == nsect && profiles.front().control_points.size() == u_num, "Generate profile fails.");

    for (size_t i = 0; i < nsect /*profiles.size()*/; ++i)
        std::for_each(profiles[i].weights.begin(), profiles[i].weights.end(),
                      [w = this->trajectory_crv.weights[i]](float &pw)
                      { pw *= w; });

    /* 对这些实例进行插值 */
    array2Vec3f cpMat {u_num, nsect};
    for (size_t r = 0; r < u_num; ++r)
        for (size_t c = 0; c < nsect; ++c)
            cpMat.at(r, c) = profiles[c].control_points[r];
    srf.control_points = InterpolateCurve(cpMat, srf.degree_v, srf.knots_v, v_bar);

    array2f wMat{u_num, nsect};
    for (size_t r = 0; r < u_num; ++r)
        for (size_t c = 0; c < nsect; ++c)
            wMat.at(r, c) = profiles[c].weights[r];

    srf.weights = InterpolateCurve(wMat, srf.degree_v, srf.knots_v, v_bar);
    
    this->swept_surf = std::move(srf);
    return;
}