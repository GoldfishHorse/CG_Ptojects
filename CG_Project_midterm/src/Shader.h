#pragma once
#include "Model.h"

/* 着色器 */
class Shader
{
public:
	Shader()
	{
		light.Type = D3DLIGHT_POINT; /* 点光源 */
		light.Position = LIGHT_POSITION;
		// light.Type = D3DLIGHT_DIRECTIONAL; /* 方向光 */
		// Vec3f lightDir = LIGHT_DIRECTION;
		// D3DXVec3Normalize(&lightDir, &lightDir);
		// light.Direction = lightDir;
		light.Diffuse = LIGHT_DIFFUSE_COLOR;
		light.Ambient = LIGHT_AMBIENT_COLOR;
	}
	Light light;
	static const Vec3f LIGHT_DIRECTION;
	static const Point3f LIGHT_POSITION;
	static const Color LIGHT_DIFFUSE_COLOR; 		/* 漫反射颜色 */
	static const Color LIGHT_AMBIENT_COLOR;			/* 环境光颜色 */

	static constexpr FLOAT kd = 0.8f; /* 漫反射系数 */

	/** @brief  渲染模型 (为 face.color 赋值) */
	void shadeModel(Model &model) const;
};