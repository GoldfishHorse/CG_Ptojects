#pragma once


#include "defs.h"

#include <vector>
#include <limits>

/* 顶点结构体 */
struct Vertex
{
	Point3f position;
	Vec3f normal;
};

/* 面结构体, 仅包含顶点 id 而不包含具体顶点 */
struct Face
{
	static constexpr size_t invalidIndex = std::numeric_limits<size_t>::max();
	std::vector<size_t> vIdx;
	std::vector<size_t> vnIdx;
	std::vector<size_t> vtIdx;
	Vec3f normal;
	Color color;
};

/* 包围盒结构体 */
struct BoundingBox
{
	Point3f minPoint, maxPoint;
};

/* 处理 .obj 三维模型的类 */
class Model
{
public:
	std::vector<Vertex> vertices;
	std::vector<Vec3f> normals;
	std::vector<Vec2f> texcoords;
	std::vector<Face> faces;

	/* 整个模型的包围盒 */
	BoundingBox bbox;
	/* 模型中各个面的包围盒 */
	std::vector<BoundingBox> bboxFace;

	Model() = default;
	Model(const std::string &path);
	
	/** @brief 该类是否已经加载模型 */
	bool empty() const { return this->vertices.empty(); }
	
	/** @brief 加载模型 */
	bool load(const std::string &path);
	
	/** @brief 根据窗口大小调整模型, 包括将模型移动到窗口中心, 以及将模型进行合适的缩放。*/
	void project(const INT windowWidth, const INT windowHeight);

	/** @brief 将模型移动到窗口中心 */
	void moveToWindowsCenter(const INT windowWidth, const INT windowHeight);

	/** @brief 对模型进行平移和缩放 */
	void transfrom(const Vec3f &translation, const FLOAT scale);

	/** @brief 对模型进行旋转、平移和缩放 */
	void transfrom(const Quaternion &rotation, const Vec3f &translation, const FLOAT scale);

	/** @brief 根据变换矩阵对模型进行空间变换 */
	void transfrom(const Matrix4x4f &matrix);

	/** @brief 计算某个面投影至平面上的点的深度 */
	FLOAT calculateZ(const size_t faceIdx, const Vec2f &pixel) const;
	
private:
	/** @brief 解析 .obj 中代表面的 f 行 */
	bool parseFace(const std::string &line, Face &face);
	/** @brief 计算三角形面投影到平面上点的深度 */
	static FLOAT calculateZTriangle(const Vec2f &pixel, const Point3f &A, const Point3f &B, const Point3f &C);
	/** @brief 在模型进行空间变换后需要重新计算包围盒 */
	void calculateBBox();
};
