#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <filesystem>
#include <regex>
#include <numeric>
#include "utils.h"
#include "Model.h"

static const FLOAT SCALT_RATIO = 0.5f; /*0.8f;*/

Model::Model(const std::string &path)
{
	namespace fs = std::filesystem;

	if (!fs::exists(path))
	{
		LOG_ERROR("File %s does not exist!", path.c_str());
	}
	else
	{
		LOG_INFO("Loading the model %s...", path.c_str());
		if (this->load(path))
		{
			LOG_INFO("Successfully load the Model: %s", path.c_str());
			LOG_INFO("vertices: %zu, faces %zu.", vertices.size(), faces.size());
			this->calculateBBox();
		}
		else
		{
			LOG_INFO("Failed to load the Model: %s", path.c_str());
		}
	}
	return;
}

bool Model::load(const std::string &path)
{
	this->vertices.clear();
	this->normals.clear();
	this->texcoords.clear();
	this->faces.clear();

	std::ifstream file(path.c_str());
	if (!file.is_open())
	{
		LOG_ERROR("Cannot open the file: %s", path.c_str());
		return false;
	}

	std::string line;
	while (std::getline(file, line))
	{

		std::string type = line.substr(0, line.find(' '));

		if (type == "v")
		{
			Vertex v;
			sscanf(line.c_str(), "v %f %f %f", &v.position.x, &v.position.y, &v.position.z);
			vertices.push_back(v);
		}
		else if (type == "vn")
		{
			Vec3f vn;
			sscanf(line.c_str(), "vn %f %f %f", &vn.x, &vn.y, &vn.z);
			this->normals.push_back(vn);
		}
		else if (type == "vt")
		{
			Vec2f vt;
			sscanf(line.c_str(), "vt %f %f", &vt.x, &vt.y);
			this->texcoords.push_back(vt);
		}
		else if (type == "f")
		{
			Face face;
			if (!parseFace(line, face))
			{
				return false;
			}

			this->faces.push_back(face);
		}
		else
		{
			// comment or something
		}
	}
	return true;
}

bool Model::parseFace(const std::string &line, Face &face)
{
	std::vector<std::string> tokens = stringSplit(line.substr(2), ' ');

	ASSERT(tokens.size() == 3 || tokens.size() == 4, "Invalid file format!");

	INT vidx, vtidx, vnidx;

	for (const std::string &token : tokens)
	{

		if (
			std::regex_match(token, std::regex(R"(\d+/\d+/\d+)")) ||
			std::regex_match(token, std::regex(R"(\d+//\d+//\d+)")))
		{
			sscanf(token.c_str(), "%d/%d/%d", &vidx, &vtidx, &vnidx);
			face.vIdx.push_back(vidx - 1);
			face.vtIdx.push_back(vtidx - 1);
			face.vnIdx.push_back(vnidx - 1);
		}
		else if (
			std::regex_match(token, std::regex(R"(\d+/\d+)")) ||
			std::regex_match(token, std::regex(R"(\d+//\d+)")))
		{
			sscanf(token.c_str(), "%d/%d", &vidx, &vtidx);

			face.vIdx.push_back(vidx - 1);
			face.vtIdx.push_back(vtidx - 1);
		}
		else if (
			std::regex_match(token, std::regex("\\d+")))
		{
			sscanf(token.c_str(), "%d", &vidx);
			face.vIdx.push_back(vidx - 1);
		}
		else
		{
			LOG_ERROR("Parse face fails: unknown format. Content: %s.", line.c_str());
			return false;
		}
	}

	/* 通过叉乘计算法向量 */
	face.normal = calculateNormal(
		vertices[face.vIdx[0]].position,
		vertices[face.vIdx[1]].position,
		vertices[face.vIdx[2]].position);

	return true;
}

void Model::calculateBBox()
{

	this->bbox = {
		.minPoint = Point3f{
			std::numeric_limits<FLOAT>::max(),
			std::numeric_limits<FLOAT>::max(),
			std::numeric_limits<FLOAT>::max()},
		.maxPoint = Point3f{std::numeric_limits<FLOAT>::lowest(), std::numeric_limits<FLOAT>::lowest(), std::numeric_limits<FLOAT>::lowest()}};

	for (const Vertex &v : this->vertices)
	{
		this->bbox.minPoint = vec3SelectMin(this->bbox.minPoint, v.position);
		this->bbox.maxPoint = vec3SelectMax(this->bbox.maxPoint, v.position);
	}

	this->bboxFace.resize(this->faces.size());

	for (size_t i = 0; i < this->faces.size(); ++i)
	{
		this->bboxFace[i] = {
			.minPoint = Point3f{
				std::numeric_limits<FLOAT>::max(),
				std::numeric_limits<FLOAT>::max(),
				std::numeric_limits<FLOAT>::max()},
			.maxPoint = Point3f{
				std::numeric_limits<FLOAT>::lowest(), 
				std::numeric_limits<FLOAT>::lowest(), 
				std::numeric_limits<FLOAT>::lowest()}};
		for (const INT vIdx : this->faces[i].vIdx)
		{
			this->bboxFace[i].minPoint = vec3SelectMin(this->bboxFace[i].minPoint, this->vertices.at(vIdx).position);
			this->bboxFace[i].maxPoint = vec3SelectMax(this->bboxFace[i].maxPoint, this->vertices.at(vIdx).position);
		}
	}
	return;
}

void Model::project(const INT windowWidth, const INT windowHeight)
{
	const FLOAT scale = SCALT_RATIO *
						(std::min(windowWidth, windowHeight) - 1) /
						std::max(
							this->bbox.maxPoint.x - this->bbox.minPoint.x,
							this->bbox.maxPoint.y - this->bbox.minPoint.y);
	const Vec3f modelCenter = (this->bbox.minPoint + this->bbox.maxPoint) / 2.0f;

	const Vec3f translation = Vec3f{
								  (FLOAT)windowWidth / 2.0f,
								  (FLOAT)windowHeight / 2.0f,
								  0.0f} -
							  modelCenter * scale;
	this->transfrom(translation, scale);
	return;
}

void Model::moveToWindowsCenter(const INT windowWidth, const INT windowHeight)
{
	const Vec3f modelCenter = (this->bbox.minPoint + this->bbox.maxPoint) / 2.0f;
	const Vec3f translation = Vec3f{
								  (FLOAT)windowWidth / 2.0f,
								  (FLOAT)windowHeight / 2.0f,
								  0.0f} -
							  modelCenter;
	this->transfrom(translation, 1.0f);
	return;
}

void Model::transfrom(const Vec3f &translation, const FLOAT scale)
{
	Matrix4x4f matScale;
	D3DXMatrixScaling(&matScale, scale, scale, scale);
	Matrix4x4f matTranslation;
	D3DXMatrixTranslation(&matTranslation, translation.x, translation.y, translation.z);
	Matrix4x4f matTransform = matScale * matTranslation;
	auto applyTransfromCoord = [&](Point3f &p) -> void
	{
		D3DXVec3TransformCoord(&p, &p, &matTransform);
	};
	for (Vertex &vertex : this->vertices)
	{
		applyTransfromCoord(vertex.position);
	}
	this->calculateBBox();
	return;
}

void Model::transfrom(const Quaternion &rotation, const Vec3f &translation, const FLOAT scale)
{
	Matrix4x4f matScale;
	D3DXMatrixScaling(&matScale, scale, scale, scale);
	Matrix4x4f matRotation;
	D3DXMatrixRotationQuaternion(&matRotation, &rotation);
	Matrix4x4f matTranslation;
	D3DXMatrixTranslation(&matTranslation, translation.x, translation.y, translation.z);
	Matrix4x4f matTransform = matScale * matTranslation * matRotation;

	auto applyTransfromCoord = [&](Point3f &p) -> void
	{
		D3DXVec3TransformCoord(&p, &p, &matTransform);
	};
	auto applyTrandfromDirection = [&](Vec3f &v) -> void
	{
		D3DXVec3TransformNormal(&v, &v, &matRotation); /* rotation only. */
	};
	for (Vertex &vertex : this->vertices)
	{
		applyTransfromCoord(vertex.position);
		applyTrandfromDirection(vertex.normal);
	}
	std::for_each(this->normals.begin(), this->normals.end(), applyTrandfromDirection);
	for (Face &face : this->faces)
	{
		applyTrandfromDirection(face.normal);
	}

	this->calculateBBox();
	return;
}

void Model::transfrom(const Matrix4x4f &matrix)
{
	auto applyTransfromCoord = [&](Point3f &p) -> void
	{
		D3DXVec3TransformCoord(&p, &p, &matrix);
	};
	auto applyTrandfromDirection = [&](Vec3f &v) -> void
	{
		D3DXVec3TransformNormal(&v, &v, &matrix);
		D3DXVec3Normalize(&v, &v);
	};
	for (Vertex &vertex : this->vertices)
	{
		applyTransfromCoord(vertex.position);
		applyTrandfromDirection(vertex.normal);
	}
	std::for_each(this->normals.begin(), this->normals.end(), applyTrandfromDirection);
	for (Face &face : this->faces)
	{
		applyTrandfromDirection(face.normal);
	}
	this->calculateBBox();
	return;
}

FLOAT Model::calculateZ(const size_t faceIdx, const Vec2f &pixel) const
{
	if (faceIdx > this->faces.size())
	{
		LOG_ERROR("Invalid faceIdx %zu!", faceIdx);
	}

	const Face &face = this->faces.at(faceIdx);
	const Point3f &v0 = this->vertices.at(face.vIdx[0]).position;
	const Point3f &v1 = this->vertices.at(face.vIdx[1]).position;
	const Point3f &v2 = this->vertices.at(face.vIdx[2]).position;

	FLOAT z = std::numeric_limits<FLOAT>::lowest();
	switch (face.vIdx.size())
	{
	case 3:
		z = Model::calculateZTriangle(pixel, v0, v1, v2);
		break;
	case 4:
	{
		/* 对于四边形面片, 取三个三角形的大深度 */
		const Point3f &v3 = this->vertices.at(face.vIdx[3]).position;
		z = std::max({Model::calculateZTriangle(pixel, v0, v1, v2),
					  Model::calculateZTriangle(pixel, v1, v2, v3),
					  Model::calculateZTriangle(pixel, v0, v1, v3)});
		break;
	}
	default:
		LOG_ERROR("Invalid face!");
		break;
	}
	return z;
}

FLOAT Model::calculateZTriangle(const Vec2f &pixel, const Point3f &A, const Point3f &B, const Point3f &C)
{
	FLOAT z = std::numeric_limits<FLOAT>::lowest();
	/* 计算重心坐标 */
	const FLOAT alpha = (-(pixel.x - B.x) * (C.y - B.y) + (pixel.y - B.y) * (C.x - B.x)) / (-(A.x - B.x) * (C.y - B.y) + (A.y - B.y) * (C.x - B.x));
	const FLOAT beta = (-(pixel.x - C.x) * (A.y - C.y) + (pixel.y - C.y) * (A.x - C.x)) / (-(B.x - C.x) * (A.y - C.y) + (B.y - C.y) * (A.x - C.x));
	const FLOAT gamma = 1 - alpha - beta;
	if (alpha >= 0 && beta >= 0 && gamma >= 0)
	{
		z = A.z * alpha + B.z * beta + C.z * gamma;
	}
	return z;
}