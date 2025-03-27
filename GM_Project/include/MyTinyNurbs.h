#pragma once


#include "Defs.h"
#include "Utils.h"

#include <vector>

struct RationalCurve3f 
{
public:
    size_t degree;
    arrayf knots;
    arrayVec3f control_points;
    arrayf weights;
    // RationalCurve3f() = default;
    // RationalCurve3f(const size_t degree, const arrayf& knots,
	// 		const arrayVec3f& control_points, const arrayf weights)
	// 		: degree(degree), knots(knots), control_points(control_points), weights(weights) {}
    Point3f Point(const float u) const ;
    arrayVec3f Derivatives(const size_t num_ders, const float u) const ;
    float Curvature(const float u) const;
    bool empty() const  { return this->control_points.empty(); }
};

struct RationalSurface3f
{
public:
    size_t degree_u, degree_v;
    arrayf  knots_u, knots_v;
    array2Vec3f control_points;
    array2f weights;
    // RationalSurface3f() = default;
    // RationalSurface3f(const size_t degree_u, const size_t degree_v, const arrayf &knots_u,
    //                 const arrayf &knots_v, const array2Vec3f &control_points,
    //                 const array2f &weights)
    //     : degree_u(degree_u), degree_v(degree_v), knots_u(knots_u), knots_v(knots_v),
    //       control_points(control_points), weights(weights)
    // {}

    Point3f Point(const float u, const float v) const ;
    Point3f Point(const Point2f p) const { return Point(p.x, p.y); }
    array2Vec3f Derivatives (const size_t num_ders, const float u, const float v) const;
    array2Vec3f Derivatives (const size_t num_ders, const Point2f p) const { return Derivatives(num_ders, p.x, p.y); }
    Point3f Normal(const float u, const float v) const ;
    Point3f Normal(const Point2f p) const { return Normal(p.x, p.y); }
    bool empty() const  { return this->control_points.empty(); }
};


/* 重写 include/tinynurbs/core/basis.h */
size_t findSpan(size_t degree, const arrayf &knots, float u);
arrayf bsplineBasis(size_t deg, int span, const arrayf &knots, float u, bool nozeros = true);
array2f bsplineDerBasis(
    size_t deg, int span, const arrayf &knots, float u,
    size_t num_ders);

/* 重写曲线、曲面相关函数 */
template <int dim>
Pointf<dim> curvePoint(
    size_t degree, const arrayf &knots,
    const std::vector<Pointf<dim> > &control_points, float u);


template <size_t dim>
std::vector<Vecf<dim> >
curveDerivatives(
    size_t degree, const arrayf &knots,
    const std::vector<Pointf<dim> > &control_points,
    size_t num_ders, float u);


template <size_t dim>
Pointf<dim> 
surfacePoint(
    size_t degree_u, size_t degree_v,
    const arrayf& knots_u, const arrayf& knots_v,
    const array2< Pointf<dim> >& control_points, float u, float v) ;

template <size_t dim>
array2< Vecf<dim> >
surfaceDerivatives(
    size_t degree_u, size_t degree_v, const arrayf &knots_u,
    const arrayf &knots_v, const array2< Pointf<dim> > &control_points,
    size_t num_ders, float u, float v);


/*  为全局曲面插值提供计算参数
    c.f. 《The NURBS Book》, Chapter 9.2.5, Algorithm A9.3.  */
std::pair<arrayf, arrayf> SurfaceMeshParams(const array2Vec3f& surf_points);

/*  使用取平均值的方法生成节点矢量
    c.f. 《The NURBS Book》, Chapter 9.2.1, Equation 9.8. 
    参数 p 代表 阶数  */
arrayf MeanKnotVector(const arrayf& params, const size_t p, bool duplicate = true);

/* 对给定点进行曲线插值 */
template<typename T>
array2<T> InterpolateCurve(const array2<T>& points, const size_t degree, const arrayf& knots, const arrayf& params);

/*  对给定的点进行全局曲面插值
    c.f. 《The NURBS Book》, Chapter 9.2.1, Algorithm A9.1 & 
    Chapter 9.2.5, Algorithm A9.4 */
RationalSurface3f GlobalSurfaceInterpolation(
    const size_t degree_u, const size_t degree_v,
    const array2Vec3f& surf_points
);


