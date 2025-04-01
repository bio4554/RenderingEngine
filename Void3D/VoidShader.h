#pragma once
#include <string>
#include <vector>

enum VoidShaderStage
{
	Vertex,
	Geometry,
	Pixel,
	Mesh,
	Amp
};

class VoidShader
{
public:
	VoidShader(std::string code, VoidShaderStage stage);
	VoidShader(const char* code, VoidShaderStage stage);
	VoidShader(std::vector<char> code, VoidShaderStage stage);
	~VoidShader();

protected:
	std::vector<char> _rawCode;
	size_t _rawCodeSize;
	VoidShaderStage _stage;
};
