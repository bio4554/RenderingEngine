#include "pch.h"
#include "VoidShader.h"

VoidShader::VoidShader(const char* code, VoidShaderStage stage) : VoidShader(std::string(code), stage)
{
}

VoidShader::VoidShader(std::vector<char> code, VoidShaderStage stage)
{
	_rawCode = code;
	_rawCodeSize = _rawCode.size() * sizeof(char);
	_stage = stage;
}

VoidShader::VoidShader(std::string code, VoidShaderStage stage)
{
	for (auto& c : code)
	{
		_rawCode.push_back(c);
	}

	_rawCodeSize = _rawCode.size() * sizeof(char);
	_stage = stage;
}

VoidShader::~VoidShader()
{

}


