#include "pch.h"
#include "D3D12_PipelineStateObjectCache.h"

#include <stdexcept>

#include "D3D12_Translator.h"
#include "D3D12_VoidImage.h"
#include "d3dx12.h"

D3D12_PipelineStateObjectCache::D3D12_PipelineStateObjectCache(ID3D12Device10* device)
{
	_device = device;
	_stateCache = std::unordered_map<size_t, Microsoft::WRL::ComPtr<ID3D12PipelineState>>();
}

ID3D12PipelineState* D3D12_PipelineStateObjectCache::get_state(D3D12_PipelineCacheDesc& desc)
{
	auto stateHash = hash_state(desc);
	Microsoft::WRL::ComPtr<ID3D12PipelineState> newState;
	{
		std::unique_lock lock{ _lock };
		auto cached = _stateCache.find(stateHash);
		if (cached != _stateCache.end())
		{
			return cached->second.Get();
		}

		// need to build pso

		// if we have a mesh shader attached, we need to build a special pipeline
		if(desc.ms != nullptr)
		{
			newState = create_mesh_state(desc);
		}
		else 
		{
			newState = create_state(desc);
		}

		_stateCache[stateHash] = newState;
	}
	return newState.Get();
}

void D3D12_PipelineStateObjectCache::hash_pointer_combine(size_t& seed, const void* p)
{
	seed ^= _pointerHasher(p) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

void D3D12_PipelineStateObjectCache::hash_cull_combine(size_t& seed, const D3D12_CULL_MODE mode)
{
	seed ^= _cullModeHasher(mode) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

void D3D12_PipelineStateObjectCache::hash_int_combine(size_t& seed, const int i)
{
	seed ^= _intHasher(i) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

void D3D12_PipelineStateObjectCache::hash_top_combine(size_t& seed, const D3D12_PRIMITIVE_TOPOLOGY_TYPE top)
{
	seed ^= _topologyHasher(top) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

void D3D12_PipelineStateObjectCache::hash_uint_combine(size_t& seed, const uint32_t i)
{
	seed ^= _uintHasher(i) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

void D3D12_PipelineStateObjectCache::hash_fill_combine(size_t& seed, const D3D12_FILL_MODE mode)
{
	seed ^= _fillHasher(mode) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

void D3D12_PipelineStateObjectCache::hash_format_combine(size_t& seed, const DXGI_FORMAT format)
{
	seed ^= _formatHasher(format) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

size_t D3D12_PipelineStateObjectCache::hash_state(D3D12_PipelineCacheDesc& desc)
{
	size_t seed = 0;

	hash_pointer_combine(seed, desc.vs);
	hash_pointer_combine(seed, desc.gs);
	hash_pointer_combine(seed, desc.ps);
	hash_pointer_combine(seed, desc.ms);
	hash_pointer_combine(seed, desc.as);
	hash_pointer_combine(seed, desc.rootSig);

	hash_cull_combine(seed, desc.cullMode);
	hash_int_combine(seed, desc.depthEnable);
	hash_int_combine(seed, desc.depthClipEnable);
	hash_int_combine(seed, desc.stencilEnable);
	hash_top_combine(seed, desc.topologyType);

	for(const auto& rt : desc.renderTargets)
	{
		hash_pointer_combine(seed, rt);
	}

	hash_uint_combine(seed, desc.numRenderTargets);

	hash_format_combine(seed, desc.depthFormat);
	//hash_pointer_combine(seed, desc.depthImage);

	hash_fill_combine(seed, desc.fillMode);

	return seed;
}

Microsoft::WRL::ComPtr<ID3D12PipelineState> D3D12_PipelineStateObjectCache::create_state(D3D12_PipelineCacheDesc& desc) const
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = desc.rootSig;
	psoDesc.VS = desc.vs == nullptr ? D3D12_SHADER_BYTECODE() : D3D12_SHADER_BYTECODE{ desc.vs->get_d3d12_blob()->GetBufferPointer(), desc.vs->get_d3d12_blob()->GetBufferSize() };
	psoDesc.GS = desc.gs == nullptr ? D3D12_SHADER_BYTECODE() : D3D12_SHADER_BYTECODE{ desc.gs->get_d3d12_blob()->GetBufferPointer(), desc.gs->get_d3d12_blob()->GetBufferSize() };
	psoDesc.PS = desc.ps == nullptr ? D3D12_SHADER_BYTECODE() : D3D12_SHADER_BYTECODE{ desc.ps->get_d3d12_blob()->GetBufferPointer(), desc.ps->get_d3d12_blob()->GetBufferSize() };

	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.RasterizerState.CullMode = desc.cullMode;
	psoDesc.RasterizerState.FillMode = desc.fillMode;
	psoDesc.RasterizerState.DepthClipEnable = desc.depthClipEnable;

	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthEnable = desc.depthEnable;
	psoDesc.DepthStencilState.StencilEnable = desc.stencilEnable;
	psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

	if(desc.depthEnable)
	{
		psoDesc.DSVFormat = D3D12_Translator::dsv_type_map(desc.depthFormat);
	}

	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = desc.topologyType;
	psoDesc.NumRenderTargets = desc.numRenderTargets;

	for(uint32_t i = 0; i < desc.numRenderTargets; i++)
	{
		psoDesc.RTVFormats[i] = desc.renderTargets[i]->get_d3d12_format();
	}

	psoDesc.SampleDesc.Count = 1;

	Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
	HRESULT hr = _device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso));
	if(FAILED(hr))
	{
		throw std::runtime_error("failed to create pso");
	}

	return pso;
}

Microsoft::WRL::ComPtr<ID3D12PipelineState> D3D12_PipelineStateObjectCache::create_mesh_state(D3D12_PipelineCacheDesc& desc) const
{
	struct MeshPipelineStateStream
	{
		/*D3D12_PIPELINE_STATE_SUBOBJECT_TYPE RSType = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE;
		void* pRootSignature;*/
		CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE RS;

		CD3DX12_PIPELINE_STATE_STREAM_AS AS;

		CD3DX12_PIPELINE_STATE_STREAM_MS MS;

		CD3DX12_PIPELINE_STATE_STREAM_PS PS;

		CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER Rasterizer;

		CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC Blend;

		CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL DepthStencil;

		CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DepthFormat;

		CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTV;
	};

	D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = {};
	MeshPipelineStateStream stateStream;

	stateStream.RS = desc.rootSig;

	stateStream.MS = D3D12_SHADER_BYTECODE{ desc.ms->get_d3d12_blob()->GetBufferPointer(), desc.ms->get_d3d12_blob()->GetBufferSize() };

	stateStream.PS = D3D12_SHADER_BYTECODE{ desc.ps->get_d3d12_blob()->GetBufferPointer(), desc.ps->get_d3d12_blob()->GetBufferSize() };

	stateStream.AS = desc.as != nullptr ? D3D12_SHADER_BYTECODE{ desc.as->get_d3d12_blob()->GetBufferPointer(), desc.as->get_d3d12_blob()->GetBufferSize() } : D3D12_SHADER_BYTECODE();

	auto rasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	rasterizerState.CullMode = desc.cullMode;
	rasterizerState.FillMode = desc.fillMode;
	rasterizerState.DepthClipEnable = desc.depthClipEnable;
	stateStream.Rasterizer = rasterizerState;

	auto blendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	stateStream.Blend = blendState;

	auto depthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	depthStencilState.DepthEnable = desc.depthEnable;
	depthStencilState.StencilEnable = desc.stencilEnable;
	depthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	depthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	stateStream.DepthStencil = depthStencilState;

	DXGI_FORMAT dsvFormat;

	if (desc.depthEnable)
	{
		dsvFormat = D3D12_Translator::dsv_type_map(desc.depthFormat);
		stateStream.DepthFormat = dsvFormat;
	}

	D3D12_RT_FORMAT_ARRAY rtvFormats = {};
	rtvFormats.NumRenderTargets = desc.numRenderTargets;

	for (uint32_t i = 0; i < desc.numRenderTargets; i++)
	{
		rtvFormats.RTFormats[i] = desc.renderTargets[i]->get_d3d12_format();
	}

	stateStream.RTV = rtvFormats;

	streamDesc.pPipelineStateSubobjectStream = &stateStream;
	streamDesc.SizeInBytes = sizeof(MeshPipelineStateStream);

	Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
	HRESULT hr = _device->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&pso));
	if (FAILED(hr))
	{
		throw std::runtime_error("failed to create pso");
	}

	return pso;
}
