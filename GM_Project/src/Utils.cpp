#include "Utils.h"
#include "MyTinyNurbs.h"


#include <glm/gtc/type_ptr.hpp>

#include <algorithm>

#if USING_NANOFLANN
#include <nanoflann.hpp>
#endif

bool Checker::isValidRelation(const size_t degree, const size_t num_knots, const size_t num_ctrl_pts)
{
    return (num_knots - degree - 1) == num_ctrl_pts;
}

bool Checker::isKnotVectorMonotonic(const arrayf &knots)
{
    return std::is_sorted(knots.cbegin(), knots.cend());
}

bool Checker::curveIsValid(const size_t degree, const arrayf &knots, const arrayVec3f &control_points)
{
    return ((degree > 0 && degree < 10) &&
            Checker::isValidRelation(degree, knots.size(), control_points.size()) &&
            Checker::isKnotVectorMonotonic(knots));
}

bool Checker::curveIsValid(const size_t degree, const arrayf &knots, const arrayVec3f &control_points, const arrayf &weights)
{
    return (Checker::curveIsValid(degree, knots, control_points) &&
            weights.size() == control_points.size());
}

bool Checker::curveIsValid(const RationalCurve3f& crv)
{
    return Checker::curveIsValid(crv.degree, crv.knots, crv.control_points, crv.weights);
}

bool Checker::surfaceIsValid(const size_t degree_u, const size_t degree_v, const arrayf &knots_u,
                             const arrayf &knots_v, const array2Vec3f &control_points)
{
    return ((degree_u > 0 && degree_u < 10 && degree_v > 0 && degree_v < 10) &&
            Checker::isValidRelation(degree_u, knots_u.size(), control_points.rows()) &&
            Checker::isValidRelation(degree_v, knots_v.size(), control_points.cols()) &&
            Checker::isKnotVectorMonotonic(knots_u) &&
            Checker::isKnotVectorMonotonic(knots_v));
}

bool Checker::surfaceIsValid( const size_t degree_u, const size_t degree_v, const arrayf& knots_u,
                                const arrayf& knots_v, const array2Vec3f& control_points,
                                const array2f& weights)
{
    return (
        Checker::surfaceIsValid(degree_u, degree_v, knots_u, knots_v, control_points) &&
        control_points.sizeLike(weights)
    );
}

bool Checker::surfaceIsValid(const RationalSurface3f& srf)
{
    return Checker::surfaceIsValid(srf.degree_u, srf.degree_v, srf.knots_u, srf.knots_v,
			srf.control_points, srf.weights);
}


/* 检测曲面是否由自相交。
   用了一个基于采样的方法, 首先根据给定精度在曲面表面生成 sample_count_u × sample_count_v 的均匀网格 
   (相当于将曲面转换成三维网格), 
   然后再检查是否由两个网格之间的顶点低于给定精度。
   由于遍历检查的效率太低, 因此使用 KDTree 加速搜索。 */
std::optional<std::pair<Point2f, Point2f> >  
Checker::surfaceIsSelfIntersection(
    const RationalSurface3f& srf, 
        const size_t sample_count_u, 
        const size_t sample_count_v,
        const float epsilon
)
{
    LOG_INFO("Detecting for self-intersections on surfaces.");

    const float stepsize_u = 1.f / (sample_count_u);
    const float stepsize_v = 1.f / (sample_count_v);
    float ptu = 0.f, ptv;

#if USING_NANOFLANN
    /* 用 KDTree 加速搜索参数空间采样点 */
    using KDTree = nanoflann::KDTreeSingleIndexAdaptor<
        nanoflann::L2_Simple_Adaptor<float, PointCloud>,
        PointCloud,
        3 /* 3D point */,
        size_t
    >;

    PointCloud cloud;
    for (size_t u = 0; u < sample_count_u; ++u, ptu += stepsize_u)
    {
        ptv = 0.f;
        for (size_t v = 0; v < sample_count_v; ++v, ptv += stepsize_v)
        {
            Vec3f point = srf.Point(ptu, ptv);
            cloud.pts.emplace_back(point.x, point.y, point.z );
        }
    }

    KDTree index(3 /* dimensionality */, cloud, { 10 /* max leaf */ });
    index.buildIndex();
    

    for (size_t u = 0; u < sample_count_u; ++u)
    {
        for (size_t v = 0; v < sample_count_v; ++v)
        {
            const Vec3f& query_pt = cloud.pts.at(u * sample_count_v + v);
            std::vector<nanoflann::ResultItem<size_t, float> > ret_matches;
            nanoflann::SearchParameters params;
            const float search_radius = static_cast<float>(epsilon * epsilon);

            const size_t nMatches = index.radiusSearch(glm::value_ptr(query_pt), search_radius, ret_matches, params);
            for (size_t i = 0; i < nMatches; ++i)
            {
                size_t match_idx = ret_matches[i].first;
                if (u * sample_count_v + v != match_idx) /* 跳过同一个点 */
                {
                    size_t match_u = match_idx / sample_count_v;
                    size_t match_v = match_idx % sample_count_v;
                    return std::make_pair(
                        Point2f{ u * stepsize_u, v * stepsize_v },
                        Point2f{ match_u * stepsize_u, match_v * stepsize_v }
                    );
                }
            }
        }
    }
#else 
    /* 暴力搜索的方法, 复杂度为 O( (sample_count_u * sample_count_v)^2 ), 速度较慢 */
    array2Vec3f points (sample_count_u, sample_count_v);
    for (size_t u = 0; u < sample_count_u; ++u, ptu += stepsize_u)
    {
        ptv = 0.f;
        for (size_t v = 0; v < sample_count_v; ++v, ptv += stepsize_v)
            points.at(u, v) = srf.Point(ptu, ptv);
    }
    
    for (size_t u1 = 0; u1 < sample_count_u; ++u1) {
        for (size_t v1 = 0; v1 < sample_count_v; ++v1) {
            for (size_t u2 = 0; u2 < sample_count_u; ++u2) {
                for (size_t v2 = 0; v2 < sample_count_v; ++v2) {
                    if (u1 == u2 && v1 == v2) continue; 
                    if (glm::distance(points.at(u1, v1), points.at(u2, v2)) < epsilon)
                    {
                        return std::make_pair(
                            Point2f{u1 * stepsize_u, v1 * stepsize_v},
                            Point2f{u2 * stepsize_u, v2 * stepsize_v}
                        );
                    }
                }
            }
        }
    }

#endif

    return std::nullopt;
}


std::vector<Point4f> getHomogenousControlPoints(const RationalCurve3f &crv)
{
    std::vector<Point4f> cps(crv.control_points.size());
    std::transform(
        crv.control_points.cbegin(), crv.control_points.cend(),
        crv.weights.cbegin(),
        cps.begin(),
        [](const Point3f &cp, float weight)
        { return cartesianToHomogenous(cp, weight); });
    return cps;
}
array2<Point4f> getHomogenousControlPoints(const RationalSurface3f &srf)
{
    const size_t r = srf.control_points.rows(), c = srf.control_points.cols();
    array2<Point4f> cps(r, c);
    for (size_t i = 0; i < r; ++i)
        for (size_t j = 0; j < c; ++j)
            cps.at(i, j) = Point4f(
                cartesianToHomogenous(
                    srf.control_points.at(i, j),
                    srf.weights.at(i, j)));
    return cps;
}

