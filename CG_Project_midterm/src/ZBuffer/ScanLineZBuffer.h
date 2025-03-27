#pragma once

#include <vector>

#include "ZBuffer.h"
#include "Model.h"

/* 分类多边形 */
struct ClassifiedPolygon
{
	Plane plane;
	INT dy; /* 多边形跨越的扫描线数目 */
	size_t faceIdx;

	void scanUpdate()
	{
		--dy;
		return;
	}
};

/* 分类边 */
struct ClassifiedEdge
{
	FLOAT x;  /* 边上端点的x坐标 */
	FLOAT dx; /* 相邻两条扫描线交点的 x 坐标差, dx = -1/k */
	INT dy;	  /* 边跨越的扫描线数目 */
	size_t faceIdx;
};

/* 活化边 */
struct ActiveEdge
{
	FLOAT xl, xr;	/* 左右交点的x坐标 */
	FLOAT dxl, dxr; /* (左右交点边上)两相邻扫描线交点的 x 坐标之差 */
	INT dyl, dyr;	/* 以和左右交点所在边相交的扫描线数为初值，向下每处理一条扫描线就减 1 */
	FLOAT zl;		/* 左交点处多边形所在平面的深度值； */
	FLOAT dzx;		/* 沿扫描线向右走过一个像素时，多边形所在平面的深度增量。 dzx = -a/c */
	FLOAT dzy;		/* 沿y方向向下移过一根扫描线时，多边形所在平面的深度增量。dzy = b/c */
	size_t faceIdx;

	void scanUpdate()
	{
		--dyl;
		--dyr;
		xl += dxl;
		xr += dxr;
		zl += dzx * dxl + dzy;
		return;
	}
};

/* 扫描线 Z-Buffer 算法 */
class ScanLineZBuffer : public ZBuffer
{
public:
	ScanLineZBuffer() = delete;
	ScanLineZBuffer(const INT windowWidth, const INT windowHeight)
		: ZBuffer(windowWidth, windowHeight) {};
	~ScanLineZBuffer() = default;
	void init();
	void renderModel(const Model &model) override;

private:
	/* 分类多边形表 */
	std::vector<std::vector<ClassifiedPolygon>> classifiedPolygonTable;
	/* 分类边表 */
	std::vector<std::vector<ClassifiedEdge>> classifiedEdgeTable;
	/* 活化多边形表 */
	std::vector<ClassifiedPolygon> activePolygonTable;
	/* 活化边表 */
	std::vector<ActiveEdge> activeEdgeTable;
	/** @brief 将分类多边形之投影中与扫描线相交的边加入到活化边表 */
	void addActiveEdge(const INT y, const ClassifiedPolygon &activePolygon);
	/** @brief 在某条活化边扫描结束后, 检查其所在面是否有新的边成为活化边 */
	void replaceActiveEdge(const INT y, ActiveEdge &activeEdge);
	/** @brief 计算给定面片中, 找出分类边表上第 y 列的边 */
	std::vector<ClassifiedEdge> calculateRelatedEdge(const INT y, const size_t faceIdx) const;
	/** @brief 构造分类边表和分类多边形表 */
	void constructTable(const Model &model);
	/** @brief 逐行扫描, 渲染模型 */
	void scan(const Model &model);
};
