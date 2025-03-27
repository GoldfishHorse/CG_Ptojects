#include "Shader.h"
#include "utils.h"

const Vec3f Shader::LIGHT_DIRECTION = Vec3f{0.0f, -1.0f, 1.0f};
const Point3f Shader::LIGHT_POSITION = Point3f{400.0f, 300.0f, 500.0f};
const Color Shader::LIGHT_DIFFUSE_COLOR = Color{0.6f, 0.6f, 0.6f, 1.0f /*alpha*/};
const Color Shader::LIGHT_AMBIENT_COLOR = Color{0.3f, 0.3f, 0.3f, 1.0f /*alpha*/};

void Shader::shadeModel(Model &model) const
{
	for (Face &face : model.faces)
	{
		
		face.color = Color{0.f, 0.f, 0.f, 1.0f};
		/* Lambert 光照模型 */
		for (size_t i = 0; i < face.vIdx.size(); i++)
		{
			Vec3f lightDir = (Vec3f)(this->light.Position) - model.vertices.at(face.vIdx.at(i)).position;
			D3DXVec3Normalize(&lightDir, &lightDir);
			const Vec3f normal = (!face.vnIdx.empty() && face.vnIdx.at(i) >= 0) ? model.normals.at(face.vnIdx.at(i))
																			   : face.normal;
			const FLOAT cosine = D3DXVec3Dot(&lightDir, &normal);
			if (cosine > 0.0) {
				face.color += kd * cosine * (Color)(this->light.Diffuse);
			}
			face.color += (Color)this->light.Ambient;
		}
		face.color /= face.vIdx.size();
		face.color = clampColor(face.color);
	}
	return;
}