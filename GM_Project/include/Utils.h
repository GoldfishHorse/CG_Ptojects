#pragma once

#include "Defs.h"

#include <cmath>
#include <numeric>
#include <vector>
#include <type_traits>
#include <optional>
#include <stdexcept>


#include <glm/glm.hpp>

#ifndef USING_NANOFLANN
#define USING_NANOFLANN true
#endif
#if USING_NANOFLANN
#if defined(__has_include) && !__has_include(<nanoflann.hpp>)
#error Cannot Find Header nanoflann.hpp!
#endif
#endif

/* 前向声明 */
class RationalCurve3f;
class RationalSurface3f;

/* 2 维数组类 */
template <typename T>
class array2
{
public:
    array2() = default;
    array2(const size_t rows, const size_t cols, const T default_value = T())
    {
        this->resize(rows, cols, default_value); return;
    }
    array2(const size_t rows, const size_t cols, const std::vector<T> &arr)
        : rows_num(rows), cols_num(cols), arr(arr)
    {
        if (arr.size() != rows * cols)
        {
            constexpr const char *err = "Dimensions do not match with size of vector";
            LOG_ERROR("%s", err);
            throw std::runtime_error(err);
        }
        return;
    }
    void resize(const size_t rows, const size_t cols, const T val = T())
    {
        this->arr.resize(rows * cols, val);
        this->rows_num = rows;
        this->cols_num = cols;
        return;
    }
    void clear()
    {
        rows_num = cols_num = (size_t)0;
        this->arr.clear();
        return;
    }
    bool empty() const
    {
        return this->arr.empty();
    }
    T at(const size_t row, const size_t col) const
    {
        ASSERT(row < rows_num && col < cols_num, "Index (%zu, %zu) out of range!", row, col);
        return this->arr[row * this->cols_num + col];
    }
    T &at(const size_t row, const size_t col)
    {
        ASSERT(row < rows_num && col < cols_num, "Index (%zu, %zu) out of range!", row, col);
        return this->arr[row * this->cols_num + col];
    }

    size_t rows() const { return rows_num; }
    size_t cols() const { return cols_num; }
    size_t size() const { return arr.size(); }
    const std::vector<T> &data() const { return this->arr; }

    array2<T> transpose() const
    {
        array2<T> transposed(this->cols_num, this->rows_num);
        for (size_t i = 0; i < this->rows_num; ++i)
        {
            for (size_t j = 0; j < this->cols_num; ++j)
            {
                transposed.at(j, i) = this->at(i, j);
            }
        }
        return transposed;
    }

    class RowIterator
    {
    public:
        RowIterator(array2 &array, size_t row) : array(array), row(row) {}

        T *begin() { return &array.arr[row * array.cols_num]; }
        T *end() { return &array.arr[(row + 1) * array.cols_num]; }

    private:
        array2 &array;
        size_t row;
    };

    class ConstRowIterator
    {
    public:
        ConstRowIterator(const array2 &array, size_t row) : array(array), row(row) {}

        const T *begin() const { return &array.arr[row * array.cols_num]; }
        const T *end() const { return &array.arr[(row + 1) * array.cols_num]; }

    private:
        const array2 &array;
        size_t row;
    };

    class ColIterator
    {
    public:
        ColIterator(array2 &array, size_t col) : array(array), col(col) {}

        class Iterator
        {
        public:
            Iterator(array2 &array, size_t col, size_t index) : array(array), col(col), index(index) {}
            bool operator!=(const Iterator &other) const { return index != other.index; }
            T &operator*() { return array.arr[index * array.cols_num + col]; }
            const Iterator &operator++()
            {
                ++index;
                return *this;
            }

        private:
            array2 &array;
            size_t col, index;
        };

        Iterator begin() { return Iterator(array, col, 0); }
        Iterator end() { return Iterator(array, col, array.rows_num); }

    private:
        array2 &array;
        size_t col;
    };

    class ConstColIterator
    {
    public:
        ConstColIterator(const array2 &array, size_t col) : array(array), col(col) {}

        class Iterator
        {
        public:
            Iterator(const array2 &array, size_t col, size_t index) : array(array), col(col), index(index) {}
            bool operator!=(const Iterator &other) const { return index != other.index; }
            const T &operator*() const { return array.arr[index * array.cols_num + col]; }
            const Iterator &operator++()
            {
                ++index;
                return *this;
            }

        private:
            const array2 &array;
            size_t col, index;
        };

        Iterator begin() const { return Iterator(array, col, 0); }
        Iterator end() const { return Iterator(array, col, array.rows_num); }

    private:
        const array2 &array;
        size_t col;
    };

    RowIterator rowIter(size_t row) { return RowIterator(*this, row); }
    ConstRowIterator rowIter(size_t row) const { return ConstRowIterator(*this, row); }
    ColIterator colIter(size_t col) { return ColIterator(*this, col); }
    ConstColIterator colIter(size_t col) const { return ConstColIterator(*this, col); }

    /* 两个 array2 的大小是否相同 */
    template <typename S>
    bool sizeLike(const array2<S> &other) const
    {
        return this->rows_num == other.rows() && this->cols_num == other.cols();
    }

private:
    size_t rows_num, cols_num;
    std::vector<T> arr;
};

/* 为数组类型定义别名 */
using arrayf = std::vector<float>;
using array2f = array2<float>;
using arrayVec3f = std::vector<Vec3f>;
using array2Vec3f = array2<Vec3f>;

/* 静态类, 用于曲线/曲面的相关检查 */
class Checker
{
public:
    static bool isValidRelation(const size_t degree, const size_t num_knots, const size_t num_ctrl_pts);
    static bool isKnotVectorMonotonic(const arrayf &knots);
    static bool curveIsValid(const size_t degree, const arrayf &knots, const arrayVec3f &control_points);
    static bool curveIsValid(const size_t degree, const arrayf &knots, const arrayVec3f &control_points, const arrayf &weights);
    static bool curveIsValid(const RationalCurve3f &crv);

    static bool surfaceIsValid(const size_t degree_u, const size_t degree_v, const arrayf &knots_u,
                               const arrayf &knots_v, const array2Vec3f &control_points);
    static bool surfaceIsValid(const size_t degree_u, const size_t degree_v, const arrayf &knots_u,
                               const arrayf &knots_v, const array2Vec3f &control_points,
                               const array2f &weights);
    static bool surfaceIsValid(const RationalSurface3f &srf);


    /* 曲面是否自相交 */
    static std::optional<std::pair<Point2f, Point2f> > 
    surfaceIsSelfIntersection(
        const RationalSurface3f& srf, 
        const size_t sample_count_u = 250, 
        const size_t sample_count_v = 500,
        const float epsilon = 1e-3f
    );
};

#if USING_NANOFLANN
/* 适配器类，用于将点集适配到 nanoflann 的 k-d 树 */
struct PointCloud
{
    arrayVec3f pts;

    /* 必须为 nanoflann 提供的适配器接口 */
    inline size_t kdtree_get_point_count() const { return pts.size(); }

    inline float kdtree_get_pt(const size_t idx, const size_t dim) const
    {
        if (dim == 0) return pts[idx].x;
        else if (dim == 1) return pts[idx].y;
        else return pts[idx].z;
    }

    /* nanoflann 使用的 Bounding Box 计算（可选）*/
    template <class BBOX>
    bool kdtree_get_bbox(BBOX&) const { return false; }
};
#endif


/* 计算二项式系数 */
inline size_t binom(size_t n, size_t k)
{
    return static_cast<size_t>(1 / ((n + 1) * std::beta(n - k + 1, k + 1)));
}

/* 比较两个浮点数 */
inline bool equalf(float a, float b, float eps = std::numeric_limits<float>::epsilon())
{
    return (std::abs(a - b) < eps) ? true : false;
}

/* 是否是一个零向量 */
inline bool isZeroVec(const Vec3f &v)
{
    return equalf(glm::length(v), 0.f);
}

/* 归一化向量 */
inline Vec3f normalize(const Vec3f &v)
{
    const float len = glm::length(v);
    return equalf(len, 0.f) ? v : (v / len);
}

/* 齐次坐标到笛卡尔坐标 */
inline Vec3f homogenousToCartesian(const Vec4f &pt)
{
    if (equalf(pt.w, 0.f))
        return Vec3f{pt};
    else
        return Vec3f{pt / pt.w};
}

/* 笛卡尔坐标到齐次坐标 */
inline Vec4f cartesianToHomogenous(const Vec3f &pt, const float w)
{   
    if (equalf(w, 0.f))
        return Vec4f{pt, 0.f};
    else
        return Vec4f{pt * w, w};
}

/* 截断齐次坐标 */
inline Vec3f truncateHomogenous(const Vec4f &pt)
{
    return Vec3f{pt};
}


/* 将有理 B 样条曲线上的控制点与权重转换成 4 维齐次坐标的形式, 用于点/导数等的计算 */
std::vector<Point4f> getHomogenousControlPoints(const RationalCurve3f &crv);
/* 将有理 B 样条曲面上的控制点与权重转换成 4 维齐次坐标的形式, 用于点/导数等的计算 */
array2<Point4f> getHomogenousControlPoints(const RationalSurface3f &srf);

/* 叉乘并归一化, 用于计算标架 */
inline Vec3f crossAndNormalize(const Vec3f &x, const Vec3f y)
{
    return normalize(glm::cross(x, y));
}


/* （编译期）获取维度 */
template<typename T, typename Enable = void>
struct Dimension;

template<typename T>
struct Dimension<T, std::enable_if_t<std::is_scalar_v<T> > > {
    static constexpr size_t value = 1;
};

template<glm::length_t L, typename T>
struct Dimension<glm::vec<L, T> > {
    static constexpr size_t value = static_cast<size_t>(L);
};