#pragma once
#include <d3dcommon.h>
#include <wrl/client.h>

#include "VoidShader.h"

class D3D12_VoidShader : public VoidShader
{
public:
	D3D12_VoidShader(std::vector<char> code, Microsoft::WRL::ComPtr<ID3DBlob> blob, VoidShaderStage stage) : VoidShader(code, stage)
	{
		_blob = blob;
	}

	ID3DBlob* get_d3d12_blob() const
	{
		return _blob.Get();
	}

protected:
	Microsoft::WRL::ComPtr<ID3DBlob> _blob;
};