#include <iostream>
#include <vector>
#include <algorithm>

#include "utils.h"
#include "ScanLineZBuffer.h"

void ScanLineZBuffer::init()
{
	this->ZBuffer::init(); /* init framebuffer */

	classifiedPolygonTable.clear();
	classifiedEdgeTable.clear();
	activePolygonTable.clear();
	activeEdgeTable.clear();

	zBuffer.resize(width);

	classifiedPolygonTable.resize(height);
	classifiedEdgeTable.resize(height);
	return;
}

void ScanLineZBuffer::renderModel(const Model &model)
{
	LOG_INFO("Scanline ZBuffer rendering...");
	this->init();
	this->constructTable(model);
	this->scan(model);
	return;
}

void ScanLineZBuffer::constructTable(const Model &model)
{
	using std::floor;
	/* 构建分类边表 */
	for (size_t i = 0; i < model.faces.size(); ++i)
	{
		const Face &face = model.faces[i];
		const std::vector<size_t> &vIdx = model.faces[i].vIdx;
		for (size_t j = 0; j < vIdx.size(); ++j)
		{
			Point3f p1 = model.vertices[vIdx[j]].position;
			Point3f p2 = model.vertices[vIdx[(j + 1) % vIdx.size()]].position;

			if ((INT)floor(p1.y) == (INT)floor(p2.y))
				continue;
			if (p1.y < p2.y)
				std::swap(p1, p2);

			classifiedEdgeTable[(size_t)floor(p1.y)].push_back(
				ClassifiedEdge{
					.x = p1.x, 
					.dx = -(floor(p1.x) - floor(p2.x)) / (floor(p1.y) - floor(p2.y)),
					.dy = (INT)(floor(p1.y) - floor(p2.y)),
					.faceIdx = i,
				});
		}

		/* 构建分类多边形表 */
		const Point3f &pos = model.vertices[face.vIdx[0]].position;
		classifiedPolygonTable[(size_t)floor(model.bboxFace[i].maxPoint.y)].push_back(
			ClassifiedPolygon{
				.plane = Plane{
					/* 平面的点法式方程
						n_x(x-x0) + n_y(y-y0) + n_z(z-z0) = 0 */
					face.normal.x,
					face.normal.y,
					face.normal.z,
					-(face.normal.x * pos.x + face.normal.y * pos.y + face.normal.z * pos.z)},
				.dy = (INT)(floor(model.bboxFace[i].maxPoint.y) - floor(model.bboxFace[i].minPoint.y)),
				.faceIdx = i});
	}
	// LOG_DEBUG("Construct Table succeeds.");
	return;
}

std::vector<ClassifiedEdge>
ScanLineZBuffer::calculateRelatedEdge(const INT y, const size_t faceIdx) const
{
	std::vector<ClassifiedEdge> relatedEdge;
	std::copy_if(
		this->classifiedEdgeTable.at(y).cbegin(),
		this->classifiedEdgeTable.at(y).cend(),
		std::back_inserter(relatedEdge),
		[faceIdx](const ClassifiedEdge &e) -> bool
		{
			return e.faceIdx == faceIdx;
		});
	return relatedEdge;
}

/* 把该多边形在 oxy 平面上的投影和扫描线相交的边加入到活化边表中 */
void ScanLineZBuffer::addActiveEdge(const INT y, const ClassifiedPolygon &activePolygon)
{
	ASSERT(y >= 0 && y < (INT)(classifiedEdgeTable.size()), "Invalid argument y = %d!", y);

	if (equalf(activePolygon.plane.c, 0.0f))
	{
		/* 平面垂直于投影面 */
		return;
	}

	auto relatedEdge = calculateRelatedEdge(y, activePolygon.faceIdx);

	if (relatedEdge.size() % 2 != 0)
	{
		const char *err = "Error to find even number of edges intersecting the current scan line.";
		LOG_ERROR("%s", err);
		throw std::runtime_error(err);
	}

	std::sort(
		relatedEdge.begin(),
		relatedEdge.end(),
		[](const ClassifiedEdge &e1, const ClassifiedEdge &e2) -> bool
		{
			return std::tie(e1.x, e1.dx) < std::tie(e2.x, e2.dx);
		});

	for (auto it = relatedEdge.cbegin(); it != relatedEdge.cend(); ++it)
	{
		ActiveEdge activeEdge{
			.xl = it->x,
			.dxl = it->dx,
			.dyl = it->dy,
			.zl = -(activePolygon.plane.d + activePolygon.plane.a * (it->x) + activePolygon.plane.b * y) / (activePolygon.plane.c),
			.dzx = -(activePolygon.plane.a / activePolygon.plane.c),
			.dzy = activePolygon.plane.b / activePolygon.plane.c,
			.faceIdx = activePolygon.faceIdx};
		++it;
		activeEdge.xr = it->x;
		activeEdge.dxr = it->dx;
		activeEdge.dyr = it->dy;
		activeEdgeTable.push_back(std::move(activeEdge));
	}
	return;
}

/* 	
	如果有些边在这条扫描线处结束了，而其所在的多边形仍在活化多边形表内，
	则可以从分类多边形表中找到该多边形在 oxy 平面上的投影与扫描线相交的新边或边对，
	修改或加到活化边表中去。
 */
void ScanLineZBuffer::replaceActiveEdge(const INT y, ActiveEdge &activeEdge)
{
	ASSERT(y >= 0 && y < (INT)(classifiedEdgeTable.size()), "Invalid argument y = %d!", y);
	ASSERT(activeEdge.dyl == 0 || activeEdge.dyr == 0, "Invalid argument activeEdge!");

	auto relatedEdge = calculateRelatedEdge(y, activeEdge.faceIdx);
	if (relatedEdge.empty())
	{
		// const char *err = "The active polygon does not contain the active edge to be deleted.";
		// LOG_ERROR("%s", err);
		// throw std::runtime_error(err);
		return;
	}
	else
	{
		if (activeEdge.dyl == 0 && activeEdge.dyr == 0)
		{
			/* 两条边同时扫描完 */
			switch (relatedEdge.size())
			{
			case (size_t)1:
			{
				/* 三角形, 最后一条边平行于 x 轴 */
				break;
			}
			case (size_t)2:
			{
				/* 四边形, 新的两条边同时取代现在的两条边 */
				ClassifiedEdge &e1 = relatedEdge[0];
				ClassifiedEdge &e2 = relatedEdge[1];
				if (std::tie(e1.x, e1.dx) > std::tie(e2.x, e2.dx))
				{
					std::swap(e1, e2);
				}
				activeEdge.xl = e1.x;
				activeEdge.dxl = e1.dx;
				activeEdge.dyl = e1.dy;
				activeEdge.xr = e2.x;
				activeEdge.dxr = e2.dx;
				activeEdge.dyr = e2.dy;
			}
			default:
				break;
			}
		}
		else if (activeEdge.dyl == 0)
		{
			/* 左侧边扫描完 */
			const ClassifiedEdge &e = relatedEdge[0];
			activeEdge.xl = e.x;
			activeEdge.dxl = e.dx;
			activeEdge.dyl = e.dy;
		}
		else if (activeEdge.dyr == 0)
		{
			/* 右侧边扫描完 */
			const ClassifiedEdge &e = relatedEdge[0];
			activeEdge.xr = e.x;
			activeEdge.dxr = e.dx;
			activeEdge.dyr = e.dy;
		}
	}
	return;
}

void ScanLineZBuffer::scan(const Model &model)
{
	/* 从上向下扫描 */
	for (INT y = height - 1; y >= 0; --y)
	{
		std::fill(zBuffer.begin(), zBuffer.end(), std::numeric_limits<FLOAT>::lowest());
		for (const ClassifiedPolygon &p : classifiedPolygonTable.at(y))
		{
			activePolygonTable.push_back(p);
			addActiveEdge(y, p);
		}
		for (ActiveEdge &e : activeEdgeTable)
		{
			for (INT x = (INT)std::floor(e.xl); x <= (INT)std::ceil(e.xr); ++x)
			{
				if (e.zl > zBuffer.at((size_t)x))
				{
					/* 更新 zBuffer */
					zBuffer.at(x) = e.zl;
					frameBuffer.at(y * width + x) = model.faces.at(e.faceIdx).color;
				}
			}
			if (e.dyl == 0 || e.dyr == 0)
			{
				replaceActiveEdge(y, e);
			}
			e.scanUpdate();
		}
		for (ClassifiedPolygon &p : activePolygonTable)
		{
			p.scanUpdate();
		}

		/* 移除扫描完的活化边以及活化多边形 */
		activePolygonTable.erase(
			std::remove_if(
				activePolygonTable.begin(),
				activePolygonTable.end(),
				[](const ClassifiedPolygon &p) -> bool
				{
					return p.dy < 0;
				}),
			activePolygonTable.end());

		activeEdgeTable.erase(
			std::remove_if(
				activeEdgeTable.begin(),
				activeEdgeTable.end(),
				[](const ActiveEdge &p) -> bool
				{
					return p.dyl < 0 || p.dyr < 0;
				}),
			activeEdgeTable.end());
	}
	return;
}
