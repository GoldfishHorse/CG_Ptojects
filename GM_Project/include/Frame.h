#include "Defs.h"
#include "Utils.h"

#include <vector>
#include <map>

/* 标架类 */
struct Frame {
    Vec3f N; /* 法向量 */
    Vec3f B; /* 次法向量 */
    Vec3f T; /* 切向量 */

    float u; /* 该标架对应的曲线参数 */

    explicit operator arrayVec3f () const {
        return arrayVec3f{N, B, T};
    }
    explicit operator Mat3f () const {
        return Mat3f{N, B, T};
    }
    /* 标架转换成齐次坐标矩阵 (旋转矩阵) */
    explicit operator Mat4f () const {
        return Mat4f{Vec4f{N, 0.f}, Vec4f{B, 0.f}, Vec4f{T, 0.f}, Vec4f{0.f, 0.f, 0.f, 1.f} };
    }
};
using FrameSeries = std::vector<Frame>;

/* 选择不同的标架计算方式 */
enum class FrameChoice {
    FrenetSerret,
    ProjectionNormal,
    RotationMinimization
};

class MovingFrame {
public:
    MovingFrame() = delete;
    MovingFrame(const RationalCurve3f& curve): curve(curve) {};

    /* 三种不同的标架实现 */

    /* 用于计算单个标架 */

    Frame FrenetSerretFrame(const float u) const;
    Frame ProjectionNormalFrame(const float u) const;
    Frame RotationMinimizationFrame(const float u) const;

    /* 用于计算一系列标架, 初始化 frameSeries 成员变量 */

    void FrenetSerretFrameSeries(const arrayf& params);
    void ProjectionNormalFrameSeries(const arrayf& params);
    void RotationMinimizationFrameSeries(const arrayf& params);
    

    void clear() { this->frameSeries.clear(); } ;
    bool empty() const { return this->frameSeries.empty(); }
    const FrameSeries& getFrameSeries() const { return this->frameSeries; }
    const std::vector<arrayVec3f> getFramesVec() const ;

    /* 用 params 参数初始化 frameSeries 成员变量 */
    void ComputeFrameSeries (const FrameChoice choice , const arrayf& params);
    /* 根据已知 frameSeries 计算单个的标架 */
    Frame ComputeSingleFrame (const FrameChoice choice, const float u) const;
private:
    /* Rotation Minimization 标架求解器 */
    Frame RMFSolver(const Vec3f& B0, const float t0, const float tend, const float dt = 0.01f) const;
    const RationalCurve3f& curve;
    FrameSeries frameSeries;
};