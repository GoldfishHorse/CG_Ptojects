#include "MyTinyNurbs.h"
#include "Utils.h"

#include <Eigen/Core>
#include <Eigen/Dense>

#include <algorithm>

/* 显式实例化 */
template Point4f curvePoint<4>(
    size_t degree, const arrayf &knots,
    const std::vector<Point4f> &control_points, float u);

template std::vector<Vec4f>
curveDerivatives<4>(
    size_t degree, const arrayf &knots,
    const std::vector<Point4f> &control_points,
    size_t num_ders, float u);

template Point4f
surfacePoint<4>(
    size_t degree_u, size_t degree_v,
    const arrayf &knots_u, const arrayf &knots_v,
    const array2<Point4f> &control_points, float u, float v);

template array2<Vec4f>
surfaceDerivatives<4>(
    size_t degree_u, size_t degree_v, const arrayf &knots_u,
    const arrayf &knots_v, const array2<Point4f> &control_points,
    size_t num_ders, float u, float v);

template 
array2Vec3f InterpolateCurve<Vec3f>(
    const array2Vec3f& points, const size_t degree, const arrayf& knots, const arrayf& params);

template 
array2f InterpolateCurve<float>(
    const array2f& points, const size_t degree, const arrayf& knots, const arrayf& params);

/* ----------------------------------------------------- */

/*  确定参数 u 所在的节点区间下标
    c.f. 《The NURBS Book》 Algorithm A2.1 */
size_t findSpan(size_t degree, const arrayf &knots, float u)
{
    ASSERT(knots.size() > 2 + degree, "Invalid argument.");
    const size_t n = knots.size() - degree - 2;
    return u > (knots[n + 1] - std::numeric_limits<float>::epsilon()) ? n : 
            u < (knots[degree] + std::numeric_limits<float>::epsilon()) ? degree : 
            std::distance(knots.cbegin(), std::lower_bound(knots.cbegin() + degree, knots.cbegin() + n + 1, u)) - 1;
}

/*  计算所有非零 B 样条基函数的值
    c.f. 《The NURBS Book》, Algorithm A2.2 */
arrayf bsplineBasis(size_t deg, int span, const arrayf &knots, float u, bool nozeros)
{
    arrayf N(deg + 1, 0.f), left(deg + 1, 0.f), right(deg + 1, 0.f);
    float saved = 0.0, temp = 0.0;
    N[0] = 1.f;
    for (size_t j = 1; j <= deg; ++j)
    {
        left[j] = (u - knots[span + 1 - j]);
        right[j] = knots[span + j] - u;
        saved = 0.0;
        for (size_t r = 0; r < j; ++r)
        {
            temp = N[r] / (right[r + 1] + left[j - r]);
            N[r] = saved + right[r + 1] * temp;
            saved = left[j - r] * temp;
        }
        N[j] = saved;
    }
    if (nozeros)
        return N;
    else
    {
        arrayf N2 (knots.size() - deg - 1, 0.f);
        std::copy(
            N.cbegin(), N.cend(), 
            N2.begin() + (span - deg)
        );
        return N2;
    }
}

/*  计算非零 B 样条基函数及其导数
    c.f. 《The NURBS Book》 Algorithm A2.3 */
array2f bsplineDerBasis(
    size_t deg, int span, const arrayf &knots, float u,
    size_t num_ders)
{
    arrayf left(deg + 1, 0.f), right(deg + 1, 0.f);

    array2f ndu(deg + 1, deg + 1);
    ndu.at(0, 0) = 1.f;

    float saved, temp, d;

    for (size_t j = 1; j <= deg; ++j)
    {
        left[j] = u - knots[span + 1 - j];
        right[j] = knots[span + j] - u;
        saved = 0.f;

        for (size_t r = 0; r < j; ++r)
        {
            /* 下三角 */
            ndu.at(j, r) = right[r + 1] + left[j - r];
            temp = ndu.at(r, j - 1) / ndu.at(j, r);
            /* 上三角 */
            ndu.at(r, j) = saved + right[r + 1] * temp;
            saved = left[j - r] * temp;
        }

        ndu.at(j, j) = saved;
    }

    array2f ders(num_ders + 1, deg + 1, 0.f);

    for (size_t j = 0; j <= deg; ++j)
        ders.at(0, j) = ndu.at(j, deg);

    array2f a{2, deg + 1, 0.f};

    for (int r = 0; r <= static_cast<int>(deg); ++r)
    {
        a.at(0, 0) = 1.f;
        for (int k = 1, s1 = 0, s2 = 1, rk, pk; k <= static_cast<int>(num_ders); ++k)
        {
            d = 0.f;
            rk = r - k;
            pk = deg - k;

            if (r >= k)
            {
                a.at(s2, 0) = a.at(s1, 0) / ndu.at(pk + 1, rk);
                d = a.at(s2, 0) * ndu.at(rk, pk);
            }

            int j1 = std::max(1, -rk);
            int j2 = (r - 1 <= pk) ? k - 1 : deg - r;

            for (int j = j1; j <= j2; ++j)
            {
                a.at(1, j) = (a.at(0, j) - a.at(0, j - 1)) / ndu.at(pk + 1, rk + j);
                d += a.at(1, j) * ndu.at(rk + j, pk);
            }

            if (r <= pk)
            {
                a.at(1, k) = -a.at(0, k - 1) / ndu.at(pk + 1, r);
                d += a.at(1, k) * ndu.at(r, pk);
            }

            ders.at(k, r) = d;
            std::swap(s1, s2);
        }
    }

    for (size_t k = 1, fac = deg; k <= num_ders; ++k)
    {
        for (size_t j = 0; j <= deg; ++j)
        {
            ders.at(k, j) *= fac;
        }
        fac *= deg - k;
    }

    return ders;
}

/* ----------------------------------------------------- */

/*  计算 B 样条曲线上的点
    c.f. 《The NURBS Book》 Algorithm A3.1 */
template <int dim>
Pointf<dim> curvePoint(
    size_t degree, const arrayf &knots,
    const std::vector<Pointf<dim>> &control_points, float u)
{
    if (u < knots.front() || u > knots.back()) 
    {
        LOG_ERROR( "Parameter u = %f is out of range.", u);
        throw std::out_of_range("Parameter u out of range!");
    }

    int span = findSpan(degree, knots, u);
    arrayf N = bsplineBasis(degree, span, knots, u);
    Pointf<dim> points(0.f);
    for (size_t j = 0; j <= degree; ++j)
    {
        points += N[j] * control_points[span - degree + j];
    }
    return points;
}

/*  计算 B 样条曲线上的导矢
    c.f. 《The NURBS Book》 Algorithm A3.2 */
template <size_t dim>
std::vector<Vecf<dim>>
curveDerivatives(
    size_t degree, const arrayf &knots,
    const std::vector<Pointf<dim>> &control_points,
    size_t num_ders, float u)
{
    if (u < knots.front() || u > knots.back()) 
    { 
        LOG_ERROR("Parameter u = %f out of range!", u);
        throw std::out_of_range("Parameter u out of range!"); 
    }

    std::vector<Vecf<dim>> curve_ders(num_ders + 1, Vecf<dim>{0.f});
    int span = findSpan(degree, knots, u);
    const array2f ders = bsplineDerBasis(
        degree, span, knots,
        u, num_ders);
    for (size_t k = 0; k <= std::min(num_ders, degree); ++k)
    {
        for (size_t j = 0; j <= degree; ++j)
        {
            curve_ders[k] += ders.at(k, j) * control_points[span - degree + j];
        }
    }
    return curve_ders;
}

/*  计算 B 样条曲线上的点
    c.f. 《The NURBS Book》 Algorighm A3.5 */
template <size_t dim>
Pointf<dim>
surfacePoint(
    size_t degree_u, size_t degree_v,
    const arrayf &knots_u, const arrayf &knots_v,
    const array2<Pointf<dim>> &control_points, float u, float v)
{
    if (u < knots_u.front() || u > knots_u.back() || v < knots_v.front() || v > knots_v.back()) 
    {   
        LOG_DEBUG("Parameter u = %f or v = %f is out of range.", u, v);
        throw std::out_of_range("Parameter u or v is out of range."); 
    }

    size_t span_u = findSpan(degree_u, knots_u, u),
           span_v = findSpan(degree_v, knots_v, v);
    arrayf Nu = bsplineBasis(degree_u, span_u, knots_u, u),
           Nv = bsplineBasis(degree_v, span_v, knots_v, v);
    Pointf<dim> points{0.0f}, temp;
    for (size_t l = 0; l <= degree_v; ++l)
    {
        temp = Pointf<dim>{0.f};
        for (size_t k = 0; k <= degree_u; ++k)
        {
            temp += Nu[k] * control_points.at(
                                span_u - degree_u + k,
                                span_v - degree_v + l);
        }
        points += Nv[l] * temp;
    }
    return points;
}

/*  计算 B 样条曲面上的点及其所有直到给定阶的偏导矢
    c.f. 《The NURBS Book》 Algorithm A3.6 */
template <size_t dim>
array2<Vecf<dim>>
surfaceDerivatives(
    size_t degree_u, size_t degree_v, const arrayf &knots_u,
    const arrayf &knots_v, const array2<Pointf<dim>> &control_points,
    size_t num_ders, float u, float v)
{
    if (u < knots_u.front() || u > knots_u.back() || v < knots_v.front() || v > knots_v.back()) 
    { 
        LOG_ERROR("Parameter u = %f or v = %f is out of range.", u, v);
        throw std::out_of_range("Parameter u or v is out of range."); 
    }

    array2<Vecf<dim>> surf_ders(
        num_ders + 1,
        num_ders + 1,
        Vecf<dim>(0.f));

    size_t span_u = findSpan(degree_u, knots_u, u),
           span_v = findSpan(degree_v, knots_v, v);
    array2f ders_u = bsplineDerBasis(degree_u, span_u, knots_u, u, num_ders),
            ders_v = bsplineDerBasis(degree_v, span_v, knots_v, v, num_ders);

    std::vector<Vecf<dim>> temp(degree_v + 1, Vecf<dim>{0.0});

    for (size_t k = 0; k <= std::min(num_ders, degree_u); ++k)
    {
        for (size_t s = 0; s <= degree_v; ++s)
        {
            for (size_t r = 0; r <= degree_u; ++r)
            {
                temp[s] += ders_u.at(k, r) * control_points.at(
                                              span_u - degree_u + r,
                                              span_v - degree_v + s);
            }
        }

        for (size_t l = 0; l <= std::min(num_ders - k, degree_v); ++l)
        {
            for (size_t s = 0; s <= degree_v; ++s)
            {
                surf_ders.at(k, l) += ders_v.at(l, s) * temp[s];
            }
        }
    }
    return surf_ders;
}

/*  计算有理 B 样条曲线上的点.
    c.f. 《The NURBS Book》 Algorithm A4.1 */
Point3f RationalCurve3f::Point(const float u) const
{
    return homogenousToCartesian(
        curvePoint(
            this->degree,
            this->knots,
            getHomogenousControlPoints(*this),
            u));
}

/*  计算有理 B 样条曲线的导矢
    c.f. 《The NURBS Book》 Algorithm A4.2 */
arrayVec3f RationalCurve3f::Derivatives(const size_t num_ders, const float u) const
{
    arrayVec3f curve_ders{num_ders + 1, Vec3f{0.f}};

    const std::vector<Point4f> Cwders = curveDerivatives<4>(
        this->degree,
        this->knots,
        getHomogenousControlPoints(*this),
        num_ders,
        u);

    for (size_t k = 0; k <= num_ders; ++k)
    {
        Vec3f v = truncateHomogenous(Cwders[k]);
        for (size_t i = 1; i <= k; ++i)
        {
            v -= static_cast<float>(binom(k, i)) * Cwders[i].w * curve_ders[k - i];
        }
        curve_ders[k] = v / Cwders[0].w;
    }
    return curve_ders;
}

/* 计算有理 B 样条曲线上某点的曲率.
    c.f. 陈维桓《微分几何》§ 2.3， 式 3.13. */
float RationalCurve3f::Curvature(const float u) const
{
    const arrayVec3f ders = this->Derivatives(2, u);
    const Vec3f &d1 = ders[1], &d2 = ders[2];
    return glm::length(glm::cross(d2, d1)) / std::pow(glm::length(d1), 3);
}

/* 计算有理 B 样条曲面上的点。
    c.f. 《The NURBS Book》Algorithm A4.3 */
Point3f RationalSurface3f::Point(const float u, const float v) const
{
    return homogenousToCartesian(
        surfacePoint<4>(
            this->degree_u,
            this->degree_v,
            this->knots_u,
            this->knots_v,
            getHomogenousControlPoints(*this),
            u,
            v));
}

/*  计算有理 B 样条曲面上的偏导矢
    c.f. 《The NURBS Book》Algorithm A4.4 */
array2Vec3f RationalSurface3f::Derivatives(const size_t num_ders, const float u, const float v) const
{
    const array2<Vec4f> homo_ders = surfaceDerivatives<4>(
        this->degree_u,
        this->degree_v,
        this->knots_u,
        this->knots_v,
        getHomogenousControlPoints(*this),
        num_ders,
        u,
        v);

    array2Vec3f surf_ders{num_ders + 1, num_ders + 1};
    Vec3f tmp;

    for (size_t k = 0; k < num_ders + 1; ++k)
    {
        for (size_t l = 0; l < num_ders - k + 1; ++l)
        {
            Vec3f der = truncateHomogenous(homo_ders.at(k, l));
            for (size_t j = 1; j < l + 1; ++j)
            {
                der -= static_cast<float>(binom(l, j)) *
                       homo_ders.at(0, j).w *
                       surf_ders.at(k, l - j);
            }
            for (size_t i = 1; i < k + 1; ++i)
            {
                der -= static_cast<float>(binom(k, i)) *
                       homo_ders.at(i, 0).w *
                       surf_ders.at(k - i, l);
                tmp = Vec3f{0.f};
                for (size_t j = 1; j < l + 1; ++j)
                {
                    tmp -= static_cast<float>(binom(l, j)) *
                           homo_ders.at(i, j).w *
                           surf_ders.at(k - 1, l - j);
                }
                der -= static_cast<float>(binom(k, i)) * tmp;
            }
            der *= 1.0f / homo_ders.at(0, 0).w;
            surf_ders.at(k, l) = der;
        }
    }
    return surf_ders;
}

/* 计算有理 B 样条曲面的法向量 */
Vec3f RationalSurface3f::Normal(const float u, const float v) const
{
    array2Vec3f ptder = this->Derivatives((size_t)1, u, v);
    return crossAndNormalize(ptder.at(0, 1), ptder.at(1, 0));
}

/*  为全局曲面插值提供计算参数
    c.f. 《The NURBS Book》, Chapter 9.2.5, Algorithm A9.3.  */
std::pair<arrayf, arrayf> SurfaceMeshParams(const array2Vec3f &surf_points)
{
    /* 行数 */
    size_t u_num = surf_points.rows();
    /* 列数 */
    size_t v_num = surf_points.cols();
    /* 非退化的行数 */
    size_t u_num_nondegenerate = u_num;
    /* 非退化的列数 */
    size_t v_num_nondegenerate = v_num;
    arrayf u_params, v_params, distances;
    u_params.resize(u_num, 0.f);
    v_params.resize(v_num, 0.f);
    u_params.back() = v_params.back() = 1.f;

    /* 计算 u_params */
    distances.resize(u_num);
    distances.front() = 0.f;
    for (size_t col = 0; col < v_num; ++col)
    {
        for (size_t row = 1; row < u_num; ++row)
            distances[row] = glm::distance(surf_points.at(row - 1, col), surf_points.at(row, col));
        std::partial_sum(distances.begin(), distances.end(), distances.begin()); /* 计算累积距离 */
        const float total_distance = distances.back();                           /* 第 row 行的总弦长 */
        if (equalf(total_distance, 0.f))
        {
            /* 该列退化 */
            --v_num_nondegenerate;
        }
        else
        {
            for (size_t row = 1; row < u_num - 1; ++row)
            {
                /* Equation (9.6) in 《The NURBS 》 */
                u_params[row] += distances[row] / total_distance;
            }
        }
    }
    if (v_num_nondegenerate == 0)
    {
        goto ERR;
    }
    else
    {
        std::for_each(u_params.begin() + 1, u_params.end() - 1, 
                [v_num_nondegenerate](float &u)
                      { u /= v_num_nondegenerate; });
    }

    /* 用同样的方法计算 v_params */
    distances.clear();
    distances.resize(v_num);
    distances.front() = 0.f;
    for (size_t row = 0; row < u_num; ++row)
    {
        for (size_t col = 1; col < v_num; ++col)
            distances[col] = glm::distance(surf_points.at(row, col - 1), surf_points.at(row, col));
        std::partial_sum(distances.begin(), distances.end(), distances.begin());
        const float total_distance = distances.back(); /* 第 col 列的总弦长 */
        if (equalf(total_distance, 0.f))
        {
            /* 该行退化 */
            --u_num_nondegenerate;
        }
        else
        {
            for (size_t col = 1; col < v_num - 1; ++col)
            {
                v_params[col] += distances[col] / total_distance;
            }
        }
    }
    if (u_num_nondegenerate == 0)
    {
        goto ERR;
    }
    else
    {
        std::for_each(v_params.begin() + 1, v_params.end()  - 1, 
                    [u_num_nondegenerate](float &v)
                      { v /= u_num_nondegenerate; });
    }

    return std::make_pair(u_params, v_params);
ERR:
    constexpr const char *err = "Invalid surface mesh parameters.";
    LOG_ERROR("%s", err);
    throw std::runtime_error(err);
    return {};
}

/* 使用取平均值的方法生成节点矢量
   即:  u_0 = ... = u_p = 0, u_{m-p} = .. = u_m = 1,
        u_j+p = \frac{1}{p}\sum_{i=j}{j+p-1}\bar{u}_i, j = 1,2,...,n-p.
    c.f. 《The NURBS Book》, Chapter 9.2.1, Equation 9.8.
    参数 p 代表 阶数
*/
arrayf MeanKnotVector(const arrayf &params, const size_t p, bool duplicate)
{
    const size_t n = params.size();
    if (p >= n) 
    { 
        LOG_ERROR("Parameter p = %zu should be less than the size of params %zu.", p, n);
        throw std::invalid_argument("Parameter p should be less than the size of params."); 
    }

    arrayf knots;
    if (duplicate)
    {
        /* 两端保留重复值 */
        knots.resize(n + p + 1);
        for (size_t i = 0; i <= p; ++i)
        {
            knots[i] = 0.f;
        }
        for (size_t i = n; i <= n + p; ++i)
        {
            knots[i] = 1.f;
        }
        for (size_t j = 1; j < n - p; ++j)
        {
            knots[j + p] = std::accumulate(params.cbegin() + j, params.cbegin() + j + p, 0.f) / p;
        }
    }
    else
    {
        /* 两端去掉重复值 */
        knots.resize(n - p - 1, 0.f);
        knots.back() = 1.f;
        for (size_t i = 1; i < n - p - 2; ++i)
        {
            knots[i] = std::accumulate(params.cbegin() + i + 1, params.cbegin() + i + 1 + p, 0.f) / p;
        }
    }
    return knots;
}

using Matnf = Eigen::MatrixXf;
using Vecnf = Eigen::VectorXf;

/*  将 std::vector 转换成 Eigen::VectorXf
    https://stackoverflow.com/questions/17036818/initialise-eigenvector-with-stdvector */
static inline Vecnf cvt2EigenVec(const arrayf &arr)
{
    return Eigen::Map<const Vecnf>(arr.data(), arr.size());
}

/* 曲线插值中方程组的求解 */
template <typename T>
array2<T> InterpolateCurve(const array2<T> &points, const size_t degree, const arrayf &knots, const arrayf &params)
{
    const size_t rows = points.rows(), cols = points.cols();
    Matnf A (cols, cols);
    for (size_t c = 0; c < cols; ++c)
    {
        const size_t span = findSpan(degree, knots, params[c]);
        const arrayf cc = bsplineBasis(degree, span, knots, params[c], false);

        A.row(c) = cvt2EigenVec(cc);
    }
    
    /* 插值于 (r, 0), ..., (r, n-1) 的曲线 */
    array2<T> R{rows, cols};
    Vecnf b (cols), sol;

    constexpr size_t dimension = Dimension<T>::value;
    if constexpr (dimension == (size_t)1)
    {
        for (size_t r = 0; r < rows; ++r)
        {
            for (size_t c = 0; c < cols; ++c)
                b(c) = points.at(r, c);

            sol = A.partialPivLu().solve(b);
            // sol = A.fullPivLu().solve(b);
            // sol = A.colPivHouseholderQr().solve(b);
            for (size_t c = 0; c < cols; ++c)
                R.at(r, c) = sol[c];
        }
    }
    else
    {

        for (size_t r = 0; r < rows; ++r)
        {
            for (int d = 0; d < dimension; ++d)
            {
                for (size_t c = 0; c < cols; ++c)
                    b(c) = points.at(r, c)[d];

                sol = A.partialPivLu().solve(b);
                // sol = A.fullPivLu().solve(b);
                // sol = A.colPivHouseholderQr().solve(b);
                for (size_t c = 0; c < cols; ++c)
                    R.at(r, c)[d] = sol[c];
            }
        }
    }
    return R;
}

/*  对给定的点进行全局曲面插值
    c.f. 《The NURBS Book》, Chapter 9.2.1, Algorithm A9.1 &
    Chapter 9.2.5, Algorithm A9.4 */
RationalSurface3f GlobalSurfaceInterpolation(
    const size_t degree_u, const size_t degree_v,
    const array2Vec3f &surf_points)
{
    const size_t rows = surf_points.rows(), cols = surf_points.cols();
    const auto [u_params, v_params] = SurfaceMeshParams(surf_points);
    const arrayf knots_u = MeanKnotVector(u_params, degree_u);
    const arrayf knots_v = MeanKnotVector(v_params, degree_v);

    const array2Vec3f R = InterpolateCurve(surf_points.transpose(), degree_u, knots_u, u_params).transpose();

    const array2Vec3f cps = InterpolateCurve(R, degree_v, knots_v, v_params);
 
 
    return RationalSurface3f{
        SFINIT(.degree_u    ,       degree_u),
        SFINIT(.degree_v    ,       degree_v),
        SFINIT(.knots_u     ,       std::move(knots_u)),
        SFINIT(.knots_v     ,       std::move(knots_v)),
        SFINIT(.control_points ,    std::move(cps)),
        SFINIT(.weights      ,      array2f{rows, cols, 1.f})
    };
}