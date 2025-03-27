#include <algorithm>
#include "NaiveZBuffer.h"
#include "Shader.h"
#include "utils.h"

void NaiveZBuffer::init()
{
	this->ZBuffer::init(); /* init framebuffer */
	zBuffer.resize(width * height);
	std::fill(zBuffer.begin(), zBuffer.end(), std::numeric_limits<FLOAT>::lowest());
	return;
}


/* 普通 Z-Buffer: 逐个检查模型的面 */
void NaiveZBuffer::renderModel(const Model &model)
{
	LOG_INFO("Naive ZBuffer rendering...");
	this->init();
	for (size_t i = 0; i < model.faces.size(); ++i)
	{
		/* 该面的包围盒在平面上投影为矩形, 遍历该矩形中的像素 */
		for (INT y = (INT)(std::floor(model.bboxFace[i].minPoint.y));
			 y <= (INT)(std::floor(model.bboxFace[i].maxPoint.y));
			 ++y)
		{
			for (INT x = (INT)(std::floor(model.bboxFace[i].minPoint.x));
				 x <= (INT)(std::floor(model.bboxFace[i].maxPoint.x));
				 ++x)
			{
				const INT location = y * this->width + x;
				// const INT location = (height - 1 - y) * this->width + x;
				FLOAT z = model.calculateZ(i, Vec2f{(FLOAT)x + 0.5f, (FLOAT)y + 0.5f});
				if (!isLowest(z) /* 像素落在该面外 */ &&
					z > zBuffer.at(location) /* 深度大于 zBuffer 中的深度 */
				)
				{
					zBuffer.at(location) = z;
					frameBuffer.at(location) = model.faces[i].color;
				}
			}
		}
	}
	return;
}
