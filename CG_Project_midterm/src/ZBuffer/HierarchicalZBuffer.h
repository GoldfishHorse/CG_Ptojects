#pragma once
#include "Model.h"
#include "ZBuffer.h"
#include <vector>

struct QuadNode;
typedef QuadNode *PQuadNode;

/* 四叉树的结点 */
struct QuadNode
{
	/* 像素矩形的边界 */
	INT xMin, yMin, xMax, yMax;
	/* 该矩形对应的深度 */
	FLOAT z;
	/* 该结点的子结点 */
	std::vector<PQuadNode> children;
	QuadNode() = default;
	QuadNode(const INT xMin, const INT yMin, const INT xMax, const INT yMax)
		: xMin(xMin),
		  yMin(yMin),
		  xMax(xMax),
		  yMax(yMax),
		  z(std::numeric_limits<FLOAT>::lowest()) {};
	~QuadNode() noexcept { this->release(); }
	/** @brief 释放结点及其子结点 */
	void release() noexcept
	{
		for (PQuadNode child : children)
		{
			if (child != nullptr)
			{
				child->release();
				delete child;
			}
		}
		children.clear();
		return;
	}
	/** @brief 该结点对应矩形是否包含二维点 */
	bool contains(const INT x, const INT y) const
	{
		return x >= xMin && y >= yMin && x <= xMax && y <= yMax;
	}
	/** @brief 该结点对应矩形是否包含二维点 */
	bool contains(const Vec2f &point) const
	{
		return this->contains(point.x, point.y);
	}

	/** @brief 该结点对应矩形是否包含了另一个矩形 */
	bool includes(const INT xMinOther, const INT yMinOther,
				  const INT xMaxOther, const INT yMaxOther) const
	{
		return xMinOther >= xMin && yMinOther >= yMin &&
			   xMaxOther <= xMax && yMaxOther <= yMax;
	}
	/** @brief 该结点对应矩形是否包含了包围盒在平面上投影得到的矩形 */
	bool includes(const BoundingBox &bbox) const
	{
		return this->includes(
			(INT)std::floor(bbox.minPoint.x), (INT)std::floor(bbox.minPoint.y),
			(INT)std::floor(bbox.maxPoint.x), (INT)std::floor(bbox.maxPoint.y));
	}

	/** @brief 该结点对应矩形是否与另一个矩形不交 */
	bool distinct(const INT xMinOther, const INT yMinOther,
				  const INT xMaxOther, const INT yMaxOther) const
	{
		return xMinOther > xMax || yMinOther > yMax ||
			   xMaxOther < xMin || yMaxOther < yMin;
	}
	/** @brief 该结点对应矩形是否与包围盒在平面上投影得到的矩形不交 */
	bool distinct(const BoundingBox &bbox) const
	{
		return this->distinct(
			(INT)std::floor(bbox.minPoint.x), (INT)std::floor(bbox.minPoint.y),
			(INT)std::floor(bbox.maxPoint.x), (INT)std::floor(bbox.maxPoint.y));
	}

	/** @brief 该结点矩形是否与另一个矩形相交 */
	bool intersect(const BoundingBox &bbox) const
	{
		return !this->distinct(bbox);
	}

	/** @brief 该结点是否是单个像素 */
	bool isSinglePixel() const
	{
		return xMin == xMax && yMin == yMax;
	}

	/** @brief 重新初始化该结点及其子结点的 z 值 */
	void resetZ()
	{
		this->z = std::numeric_limits<FLOAT>::lowest();
		for (PQuadNode pChildNode : this->children)
		{
			pChildNode->resetZ();
		}
		return;
	}
};

/**
 * 用于实现层次 Z-Buffer 的 Z-Pyramid, 即用四叉树分层的 Z-Buffer 结构。
 * 最后一层代表 Z-Buffer。
 */
class ZPyramid
{
public:
	ZPyramid(INT width, INT height) : width(width), height(height),
									  root(nullptr) { this->init(); };
	~ZPyramid() noexcept { this->release(); }
	void init()
	{
		this->root = this->build(0, 0, width - 1, height - 1);
	};

	bool empty() const { return root == nullptr; }

	const INT width, height; /* 像素窗口宽度和高度 */
	PQuadNode root; /* 四叉树根节点 */

	void release() noexcept
	{
		if (root != nullptr)
		{
			root->release();
			root = nullptr;
		}
		return;
	}
	/** @brief 重新设置深度值, 从而无需再重复构建四叉树 */
	void reset()
	{
		if (this->root == nullptr)
		{
			this->init();
		}
		else
		{
			this->root->resetZ();
		}
		return;
	}

private:
	/** @brief 构建四叉树 */
	PQuadNode build(INT xMin, INT yMin, INT xMax, INT yMax);
};

/* 	Greene, N., Kass, M., & Miller, G. (1993, September). Hierarchical Z-buffer visibility.
	In Proceedings of the 20th annual conference on Computer graphics and interactive techniques (pp. 231-238). */
/* 层次 Z-Buffer */
class HierarchicalZBuffer : public ZBuffer
{
public:
	HierarchicalZBuffer() = delete;
	HierarchicalZBuffer(const INT windowsWidth, const INT windowHeight)
		: ZBuffer(windowsWidth, windowHeight),
		  zPyramid(windowsWidth, windowHeight) {};
	virtual ~HierarchicalZBuffer() = default;
	void init()
	{
		this->ZBuffer::init(); /* init frameBuffer */
		// zPyramid.release();
		// zPyramid.init();
		this->zPyramid.reset();
	}
	void renderModel(const Model &model) override;

protected:
	/** @brief 递归寻找包含给定面的四叉树结点, 并进行渲染 */
	void renderFace(const size_t faceIdx, const Model &model, PQuadNode pNode);
	/** @brief 在找到给定面对应的四叉树结点后, 更新 frameBuffer 和 zPyramid */
	void update(const size_t faceIdx, const Model &model, PQuadNode pNode);
	ZPyramid zPyramid;
};