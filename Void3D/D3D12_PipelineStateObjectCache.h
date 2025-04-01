#pragma once

#include <d3d12.h>
#include <mutex>
#include <unordered_map>
#include <wrl/client.h>

#include "D3D12_VoidShader.h"

class D3D12_VoidImage;

struct D3D12_PipelineCacheDesc
{
	D3D12_VoidShader* vs = nullptr;
	D3D12_VoidShader* gs = nullptr;
	D3D12_VoidShader* ps = nullptr;
	D3D12_VoidShader* ms = nullptr;
	D3D12_VoidShader* as = nullptr;
	ID3D12RootSignature* rootSig = nullptr;
	D3D12_CULL_MODE cullMode = D3D12_CULL_MODE_NONE;
	BOOL depthEnable = FALSE;
	BOOL stencilEnable = FALSE;
	D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	std::vector<D3D12_VoidImage*> renderTargets;
	uint32_t numRenderTargets = 0;
	DXGI_FORMAT depthFormat;
	D3D12_FILL_MODE fillMode = D3D12_FILL_MODE_SOLID;
	BOOL depthClipEnable = TRUE;
};

class D3D12_PipelineStateObjectCache
{
public:
	D3D12_PipelineStateObjectCache(ID3D12Device10* device);

	ID3D12PipelineState* get_state(D3D12_PipelineCacheDesc& desc);

private:
	void hash_pointer_combine(size_t& seed, const void* p);
	void hash_cull_combine(size_t& seed, const D3D12_CULL_MODE mode);
	void hash_int_combine(size_t& seed, const int i);
	void hash_top_combine(size_t& seed, const D3D12_PRIMITIVE_TOPOLOGY_TYPE top);
	void hash_uint_combine(size_t& seed, const uint32_t i);
	void hash_fill_combine(size_t& seed, const D3D12_FILL_MODE mode);
	void hash_format_combine(size_t& seed, const DXGI_FORMAT format);


	size_t hash_state(D3D12_PipelineCacheDesc& desc);
	Microsoft::WRL::ComPtr<ID3D12PipelineState> create_state(D3D12_PipelineCacheDesc& desc) const;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> create_mesh_state(D3D12_PipelineCacheDesc& desc) const;

	ID3D12Device10* _device;

	std::unordered_map<size_t, Microsoft::WRL::ComPtr<ID3D12PipelineState>> _stateCache;

	std::hash<const void*> _pointerHasher;
	std::hash<D3D12_CULL_MODE> _cullModeHasher;
	std::hash<int> _intHasher;
	std::hash<D3D12_PRIMITIVE_TOPOLOGY_TYPE> _topologyHasher;
	std::hash<uint32_t> _uintHasher;
	std::hash<D3D12_FILL_MODE> _fillHasher;
	std::hash<DXGI_FORMAT> _formatHasher;

	std::mutex _lock;
};
