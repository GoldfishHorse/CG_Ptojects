#include "ZBuffer.h"

#include <algorithm>
void ZBuffer::init()
{
	// frameBuffer.clear();
	frameBuffer.resize(width * height);
	std::fill(frameBuffer.begin(), frameBuffer.end(), Color{D3DCOLOR_WHITE /* 0xFFFFFFFF */});
	zBuffer.clear();
}

/* 无 ZBuffer 直接渲染 */
void ZBuffer::renderModel(const Model &model)
{
	LOG_INFO("Rendering without ZBuffer...");
	this->init();
	for (size_t i = 0; i < model.faces.size(); ++i)
	{
		for (INT y = (INT)(std::floor(model.bboxFace[i].minPoint.y));
			 y <= (INT)(std::floor(model.bboxFace[i].maxPoint.y));
			 ++y)
		{
			for (INT x = (INT)(std::floor(model.bboxFace[i].minPoint.x));
				 x <= (INT)(std::floor(model.bboxFace[i].maxPoint.x));
				 ++x)
			{
				frameBuffer.at(y * this->width + x) = model.faces[i].color;
			}
		}
	}
}