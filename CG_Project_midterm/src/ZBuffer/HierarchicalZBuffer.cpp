#include "utils.h"
#include "HierarchicalZBuffer.h"

#include <algorithm>

PQuadNode ZPyramid::build(INT xMin, INT yMin, INT xMax, INT yMax)
{
	PQuadNode node = new QuadNode(xMin, yMin, xMax, yMax);

	if (!node->isSinglePixel())
	{
		INT hx = (xMin + xMax) / 2, hy = (yMin + yMax) / 2;
		node->children.push_back(this->build(xMin, yMin, hx, hy));
		if (hx < xMax)
			node->children.push_back(this->build(1 + hx, yMin, xMax, hy));
		if (hy < yMax)
			node->children.push_back(this->build(xMin, 1 + hy, hx, yMax));
		if (hx < xMax && hy < yMax)
			node->children.push_back(this->build(1 + hx, 1 + hy, xMax, yMax));
	}
	return node;
}

void HierarchicalZBuffer::renderModel(const Model &model)
{
	LOG_INFO("Hierarchical ZBuffer rendering...");
	this->init();
	for (size_t i = 0; i < model.faces.size(); ++i)
	{
		this->renderFace(i, model, this->zPyramid.root);
	}
	return;
}

void HierarchicalZBuffer::renderFace(const size_t faceIdx, const Model &model, PQuadNode pNode)
{
	/* 这里应该不用判断 */
	// if (!pNode->includes(model.bboxFace[faceIdx])) {
	// 	/* 结点并不包含该面 */
	// 	return;
	// }

	if (model.bboxFace[faceIdx].maxPoint.z <= pNode->z)
	{
		/* face 的包围盒的深度低于该结点的深度 */
		return;
	}

	if (
		auto it = std::find_if(pNode->children.cbegin(), pNode->children.cend(),
							   [&bbox = model.bboxFace[faceIdx]](const PQuadNode pChildNode) -> bool
							   {
								   return pChildNode->includes(bbox);
							   });
		it != pNode->children.cend())
	{
		/* 存在一个子结点包含该 face */
		this->renderFace(faceIdx, model, *it);
	}
	else
	{
		/* 不存在子结点包含该 face */
		/* 更新 zPyramid 和 frameBuffer */
		this->update(faceIdx, model, pNode);
	}
	return;
}

void HierarchicalZBuffer::update(const size_t faceIdx, const Model &model, PQuadNode pNode)
{
	if (pNode->intersect(model.bboxFace[faceIdx]))
	{

		/* 四叉树结点代表单个像素, 说明走到了四叉树最底层, 即 Z-Buffer */
		if (pNode->isSinglePixel())
		{
			FLOAT z = model.calculateZ(
				faceIdx,
				Vec2f{
					(FLOAT)pNode->xMin + 0.5f,
					(FLOAT)pNode->yMin + 0.5f});
			if (!isLowest(z) /* 像素落在该面外 */ &&
				z > pNode->z /* 深度大于 zBuffer 中的深度 */
			)
			{
				pNode->z = z;
				frameBuffer.at(pNode->yMin * this->zPyramid.width + pNode->xMin) = model.faces.at(faceIdx).color;
			}
		}
		else
		{
			for (PQuadNode pChildNode : pNode->children)
			{
				/* 递归更新子结点的深度值 */
				this->update(faceIdx, model, pChildNode);
			}
			/* 更新自身的深度值: 所有子结点中深度值最小的值 */
			pNode->z = (*std::min_element(
							pNode->children.cbegin(),
							pNode->children.cend(),
							[](const PQuadNode p, const PQuadNode q) -> bool
							{
								return p->z < q->z;
							}))
						   ->z;
		}
	}
	return;
}