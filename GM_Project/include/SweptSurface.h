#pragma once

#include "Defs.h"
#include "Utils.h"
#include "MyTinyNurbs.h"
#include "Frame.h"

#include <vector>
#include <functional>


using ShapeControllor =  std::function<Mat4f(float)>;

/* 选择不同的扫掠曲面计算方式 */
enum class SweptSurfaceChoice {
    Approximation,
    Offset,
    Skinning
};

/* 扫掠曲线类 */
class SweptSurface 
{
public:
    SweptSurface() = delete;
    SweptSurface(
        const RationalCurve3f& contour_crv,
        const RationalCurve3f& trajectory_crv,
        const ShapeControllor& shape_controller = [] ( float ) { return Mat4f(1.f); }
    )
        :   contour_crv(contour_crv),
            trajectory_crv(trajectory_crv),
            frames(trajectory_crv),
            shape_controller(shape_controller)
    {
        ASSERT(Checker::curveIsValid(this->contour_crv), "Invalid contour curve!");
        ASSERT(Checker::curveIsValid(this->trajectory_crv), "Invalid trajectory curve!");
        this->init();
    };

    void init();
    
    /* 生成扫掠曲面的三种方法 */

    void ApproximationMethod(const FrameChoice choice);
    void OffsetMethod(const FrameChoice choice);
    void SkinningMethod(const FrameChoice choice);
  
    /* 选择方法生成扫掠曲面和标架 */
    const RationalSurface3f& generateSweptSurface(const SweptSurfaceChoice sschoice, const FrameChoice fchoice);

    const std::vector<arrayVec3f> getFrames() const  { return this->frames.getFramesVec(); };
    const arrayf& getVbar() const { return this->v_bar; }
    const RationalSurface3f& getSweptSurface() const { return this->swept_surf; }
    const RationalCurve3f& getContourCurve () const { return this->contour_crv; }
    const RationalCurve3f& getTrajectoryCurve() const { return this->trajectory_crv; }
    /* 生成轮廓曲线集 */
    std::vector<RationalCurve3f> generateProfileCurves() const;

private:
    /* 生成的扫掠曲线 */
    RationalSurface3f swept_surf;
    
    /* 控制轮廓曲线在扫掠过程中的形状 */
    const ShapeControllor shape_controller;
    /* 轮廓曲线 */
    const RationalCurve3f& contour_crv;
    /* 扫掠曲线 (轨迹曲线) */
    const RationalCurve3f& trajectory_crv;
    /* 插值于 v 方向的 v 参数值 */
    arrayf v_bar;
    /* 标架, 在生成扫掠曲面时计算 */
    MovingFrame frames;
};