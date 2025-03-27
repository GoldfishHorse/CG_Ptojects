#pragma once

#include "ZBuffer.h"
#include "Model.h"

class NaiveZBuffer : public ZBuffer
{
public:
	NaiveZBuffer() = delete;
	NaiveZBuffer(const int windowsWidth, const int windowHeight)
		: ZBuffer(windowsWidth, windowHeight) {};
	~NaiveZBuffer() = default;
	void init();
	void renderModel(const Model &model) override;

// private:
// 	std::vector<FLOAT> zBuffer;
};