#include "pch.h"
#include "D3D12_PipelineState.h"

#include <stdexcept>

#include "VoidAssertUtil.h"

std::unique_ptr<D3D12_PipelineState::PipelineAllocators> D3D12_PipelineState::_allocators;
ComPtr<ID3D12RootSignature> D3D12_PipelineState::_rootSignature;
D3D12_PipelineStateObjectCache* D3D12_PipelineState::_stateCache;
D3D12_CPU_DESCRIPTOR_HANDLE D3D12_PipelineState::_defaultSampler;
D3D12_CPU_DESCRIPTOR_HANDLE D3D12_PipelineState::_defaultSrv;

void D3D12_PipelineState::init(ID3D12Device10* device)
{
	VOID_ASSERT(_allocators == nullptr, "allocators already made!");
	_allocators = std::make_unique<PipelineAllocators>(device);

	D3D12_DESCRIPTOR_RANGE1 srvRanges[_maxSrvSections];

	for (int i = 0; i < _maxSrvSections; i++)
	{
		srvRanges[i] = {};
		srvRanges[i].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		srvRanges[i].NumDescriptors = 8;
		srvRanges[i].BaseShaderRegister = i * 8;
		srvRanges[i].RegisterSpace = 0;
		srvRanges[i].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;
		srvRanges[i].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	}

	D3D12_DESCRIPTOR_RANGE1 samplerRange;
	samplerRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
	samplerRange.NumDescriptors = _maxSamplers;
	samplerRange.BaseShaderRegister = 3;
	samplerRange.RegisterSpace = 0;
	samplerRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;
	samplerRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_DESCRIPTOR_RANGE1 constantRange;
	constantRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	constantRange.NumDescriptors = _maxCbvs;
	constantRange.BaseShaderRegister = 0;
	constantRange.RegisterSpace = 0;
	constantRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;
	constantRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER1 rootParameters[2 + _maxSrvSections];

	/*rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
	rootParameters[0].DescriptorTable.pDescriptorRanges = &srvRange;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;*/

	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
	rootParameters[0].DescriptorTable.pDescriptorRanges = &constantRange;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
	rootParameters[1].DescriptorTable.pDescriptorRanges = &samplerRange;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	for (int i = 0; i < _maxSrvSections; i++)
	{
		auto idx = i + 2;
		rootParameters[idx].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[idx].DescriptorTable.NumDescriptorRanges = 1;
		rootParameters[idx].DescriptorTable.pDescriptorRanges = &srvRanges[i];
		rootParameters[idx].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	}

	D3D12_STATIC_SAMPLER_DESC staticSampler = {};
	staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	staticSampler.MipLODBias = 0.0f;
	staticSampler.MaxAnisotropy = 1;
	staticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	staticSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	staticSampler.MinLOD = 0.0f;
	staticSampler.MaxLOD = D3D12_FLOAT32_MAX;
	staticSampler.ShaderRegister = 0; // s0
	staticSampler.RegisterSpace = 0;
	staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_STATIC_SAMPLER_DESC staticPcfSampler = {};
	staticPcfSampler.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT; // Use linear filtering with comparison
	staticPcfSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;               // Clamp to avoid wrapping
	staticPcfSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticPcfSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticPcfSampler.MipLODBias = 0.0f;
	staticPcfSampler.MaxAnisotropy = 1;
	staticPcfSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;         // Comparison function for shadow mapping
	staticPcfSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	staticPcfSampler.MinLOD = 0.0f;
	staticPcfSampler.MaxLOD = D3D12_FLOAT32_MAX;
	staticPcfSampler.ShaderRegister = 1; // s0
	staticPcfSampler.RegisterSpace = 0;
	staticPcfSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_STATIC_SAMPLER_DESC staticPointSampler = {};
	staticPointSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	staticPointSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticPointSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticPointSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticPointSampler.MipLODBias = 0.0f;
	staticPointSampler.MaxAnisotropy = 1;
	staticPointSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	staticPointSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	staticPointSampler.MinLOD = 0.0f;
	staticPointSampler.MaxLOD = D3D12_FLOAT32_MAX;
	staticPointSampler.ShaderRegister = 2; // s0
	staticPointSampler.RegisterSpace = 0;
	staticPointSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_STATIC_SAMPLER_DESC staticSamplers[3];
	staticSamplers[0] = staticSampler;
	staticSamplers[1] = staticPcfSampler;
	staticSamplers[2] = staticPointSampler;

	D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootDesc;
	rootDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
	rootDesc.Desc_1_1.NumParameters = _countof(rootParameters);
	rootDesc.Desc_1_1.pParameters = rootParameters;
	rootDesc.Desc_1_1.NumStaticSamplers = _countof(staticSamplers);
	rootDesc.Desc_1_1.pStaticSamplers = staticSamplers;
	rootDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	HRESULT hr = D3D12SerializeVersionedRootSignature(&rootDesc, &signature, &error);
	if (FAILED(hr))
	{
		if (error)
		{
			OutputDebugStringA((char*)error->GetBufferPointer());
		}

		throw std::runtime_error("failed to serialize root sig");
	}

	hr = device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&_rootSignature));

	if (FAILED(hr))
	{
		throw std::runtime_error("failed to create root sig");
	}

	// init cache
	_stateCache = new D3D12_PipelineStateObjectCache(device);

	{
		_defaultSrv = _allocators->cpuBufferDescHeap->allocate();
		_defaultSampler = _allocators->cpuSamplerDescHeap->allocate();
	}
}

void D3D12_PipelineState::shutdown_static()
{
	delete _stateCache;
	_allocators.reset();
	_rootSignature.Reset();
}

D3D12_PipelineState::D3D12_PipelineState(ID3D12Device10* device)
{
	_device = device;
	_dirtyFlags = PSODirtyFlags::Clean;

	for (auto& b : _samplerDirtyFlags)
	{
		b = false;
	}
}

D3D12_PipelineState::~D3D12_PipelineState()
{
	_allocators->cpuBufferDescHeap->free(_defaultSrv);
	_allocators->cpuSamplerDescHeap->free(_defaultSampler);
}

void D3D12_PipelineState::init(VoidExtent2D viewport)
{
	_viewport = viewport;

	/*D3D12_DESCRIPTOR_RANGE1 srvRange;
	srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	srvRange.NumDescriptors = _maxSrvs;
	srvRange.BaseShaderRegister = 0;
	srvRange.RegisterSpace = 0;
	srvRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;
	srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;*/

	

	// init descriptors
	

	_stateDesc.rootSig = _rootSignature.Get();
}

void D3D12_PipelineState::bind_vertex_shader(D3D12_VoidShader* shader)
{
	if (_stateDesc.vs == shader)
		return;

	_stateDesc.vs = shader;
	_dirtyFlags |= VertexShader;
}

void D3D12_PipelineState::bind_geo_shader(D3D12_VoidShader* shader)
{
	if (_stateDesc.gs == shader)
		return;

	_stateDesc.gs = shader;
	_dirtyFlags |= GeoShader;
}

void D3D12_PipelineState::bind_pixel_shader(D3D12_VoidShader* shader)
{
	if (_stateDesc.ps == shader)
		return;

	_stateDesc.ps = shader;
	_dirtyFlags |= PixelShader;
}

void D3D12_PipelineState::bind_mesh_shader(D3D12_VoidShader* shader)
{
	if (_stateDesc.ms == shader)
		return;

	_stateDesc.ms = shader;
	_dirtyFlags |= MeshShader;
}

void D3D12_PipelineState::bind_amp_shader(D3D12_VoidShader* shader)
{
	if (_stateDesc.as == shader)
		return;

	_stateDesc.as = shader;
	_dirtyFlags |= AmpShader;
}

void D3D12_PipelineState::bind_resource(uint16_t slot, std::shared_ptr<D3D12_VoidStructuredBuffer> buffer)
{
	VOID_ASSERT(slot < (8 * _maxSrvSections), "slot too big");

	auto section = slot / 8;

	VOID_ASSERT(section < _maxSrvSections, "invalid section");

	auto actualSlot = slot % 8;

	_boundSrvSections[section].resources[actualSlot] = buffer;
	_boundSrvSections[section].dirty = true;
	_dirtyFlags |= StructuredBuffers;
}

void D3D12_PipelineState::bind_resource(uint16_t slot, std::shared_ptr<D3D12_VoidImage> texture)
{
	VOID_ASSERT(slot < (8 * _maxSrvSections), "slot too big");

	auto section = slot / 8;

	VOID_ASSERT(section < _maxSrvSections, "invalid section");

	auto actualSlot = slot % 8;

	_boundSrvSections[section].resources[actualSlot] = texture;
	_boundSrvSections[section].dirty = true;
	_dirtyFlags |= StructuredBuffers;
}

void D3D12_PipelineState::bind_resource(uint16_t slot, std::shared_ptr<D3D12_VoidSampler> sampler)
{
	VOID_ASSERT(slot < _maxSamplers, "slot too big");

	_samplerDirtyFlags[slot] = true;
	_boundSamplers[slot] = sampler;
	_dirtyFlags |= SamplerBuffers;
}

void D3D12_PipelineState::bind_constant(uint16_t slot, std::shared_ptr<D3D12_VoidStructuredBuffer> buffer)
{
	VOID_ASSERT(slot < _maxCbvs, "slot too big");

	_boundCbvs[slot] = buffer;
	_dirtyFlags |= ConstantBuffers;
}

void D3D12_PipelineState::bind_render_target(uint16_t slot, std::shared_ptr<D3D12_VoidImage> target)
{
	VOID_ASSERT(slot < _maxRtvs, "slot too big");

	_dirtyFlags |= PSODirtyFlags::RenderTargets;
	_boundRtvs[slot] = target;
}

void D3D12_PipelineState::bind_depth_target(std::shared_ptr<D3D12_VoidImage> target)
{
	_dirtyFlags |= PSODirtyFlags::DepthBuffers;

	_boundDsv = target;
	_stateDesc.depthFormat = target->get_d3d12_format();
}

//void D3D12_PipelineState::bind_depth_target(uint16_t slot, bool array, uint32_t layer)
//{
//	auto target = _boundRtvs[slot];
//
//	VOID_ASSERT(target != nullptr, "buffer was null");
//
//	_stateDesc.depthFormat = target->get_d3d12_format();
//	_stateDesc.depthArray = array;
//	_stateDesc.depthLayer = layer;
//	_stateDesc.depthImage = target;
//}

void D3D12_PipelineState::bind_index_buffer(std::shared_ptr<D3D12_VoidIndexBuffer> buffer)
{
	_boundIndexBuffer = buffer;

	_dirtyFlags |= PSODirtyFlags::IndexBuffer;
}

void D3D12_PipelineState::force_bind_state(ID3D12GraphicsCommandList* cmd)
{
	ID3D12DescriptorHeap* ppHeaps[] = { _allocators->gpuBufferDescHeap->get_heap(), _allocators->gpuSamplerDescHeap->get_heap() };
	cmd->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	bind_view_scissor(cmd, _viewport, _viewport);

	_dirtyFlags = ANY_STATE_DIRTY | ANY_RESOURCE_DIRTY;
}

std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> D3D12_PipelineState::allocate_buffer_desc()
{
	return _allocators->gpuBufferDescHeap->allocate();
}

void D3D12_PipelineState::ensure_resource_states(ID3D12GraphicsCommandList* cmd)
{
	for(uint32_t i = 0; i < _stateDesc.numRenderTargets; i++)
	{
		auto& rtv = _boundRtvs[i];

		if(rtv != nullptr)
		{
			if(rtv->_currentState != D3D12_RESOURCE_STATE_RENDER_TARGET)
			{
				transition_resource(cmd, rtv.get(), VOID_GET_RESOURCE_STATE(rtv), D3D12_RESOURCE_STATE_RENDER_TARGET);
			}
		}
	}

	if(_boundDsv != nullptr && _stateDesc.depthEnable == TRUE)
	{
		if(_boundDsv->_currentState != D3D12_RESOURCE_STATE_DEPTH_WRITE)
		{
			transition_resource(cmd, _boundDsv.get(), VOID_GET_RESOURCE_STATE(_boundDsv), D3D12_RESOURCE_STATE_DEPTH_WRITE);
		}
	}
}

void D3D12_PipelineState::set_enable_depth(bool enabled)
{
	_stateDesc.depthEnable = enabled == TRUE;

	_dirtyFlags |= PSODirtyFlags::DepthTest;
}

void D3D12_PipelineState::set_enable_stencil(bool enabled)
{
	_stateDesc.stencilEnable = enabled == TRUE;

	_dirtyFlags |= PSODirtyFlags::Stencil;
}

void D3D12_PipelineState::set_topology(D3D12_PRIMITIVE_TOPOLOGY_TYPE type)
{
	_stateDesc.topologyType = type;

	_dirtyFlags |= PSODirtyFlags::Topology;
}

void D3D12_PipelineState::set_input_topology(D3D_PRIMITIVE_TOPOLOGY type)
{
	_boundInputTopology = type;

	_dirtyFlags |= PSODirtyFlags::InputPrimitive;
}

void D3D12_PipelineState::set_num_rt(uint32_t num)
{
	_stateDesc.numRenderTargets = num;

	if(_stateDesc.renderTargets.size() < num)
	{
		_stateDesc.renderTargets.resize(num);
	}

	for(uint32_t i = 0; i < num; i++)
	{
		_stateDesc.renderTargets[i] = _boundRtvs[i].get();
	}

	_dirtyFlags |= PSODirtyFlags::RenderTargets;
}

void D3D12_PipelineState::set_cull_mode(D3D12_CULL_MODE mode)
{
	_stateDesc.cullMode = mode;

	_dirtyFlags |= PSODirtyFlags::CullMode;
}

void D3D12_PipelineState::set_draw_extent(VoidExtent2D extent)
{
	_viewport = extent;

	_dirtyFlags |= PSODirtyFlags::DrawExtent;
}

void D3D12_PipelineState::set_fill_mode(D3D12_FILL_MODE mode)
{
	_stateDesc.fillMode = mode;

	_dirtyFlags |= PSODirtyFlags::FillMode;
}

void D3D12_PipelineState::set_depth_clip(bool enabled)
{
	_stateDesc.depthClipEnable = enabled ? TRUE : FALSE;

	_dirtyFlags |= PSODirtyFlags::DepthTest;
}

void D3D12_PipelineState::frame_ended(ID3D12GraphicsCommandList* cmd)
{
	ID3D12DescriptorHeap* ppHeaps[] = { _allocators->gpuBufferDescHeap->get_heap(), _allocators->gpuSamplerDescHeap->get_heap() };
	cmd->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	auto usedDescriptors = _allocators->gpuBufferDescHeap->end_frame();
	auto usedSamplers = _allocators->gpuSamplerDescHeap->end_frame();

	bind_view_scissor(cmd, _viewport, _viewport);

	_dirtyFlags = ANY_STATE_DIRTY | ANY_RESOURCE_DIRTY;
}

void D3D12_PipelineState::bind_shader_resources(ID3D12GraphicsCommandList* cmd)
{
	commit_bound_to_cpu();

	if((_dirtyFlags & PSODirtyFlags::DrawExtent) == DrawExtent)
	{
		bind_view_scissor(cmd, _viewport, _viewport);

		_dirtyFlags ^= DrawExtent;
	}

	if((_needGpuCommit & PSODirtyFlags::StructuredBuffers) == StructuredBuffers)
	{
		/*auto gpuRange = _gpuBufferDescHeap->allocate(_maxSrvs);
		UINT size = _maxSrvs;

		for(uint32_t i = 0; i < _maxSrvs; i++)
		{
			auto srv = _boundSrvs[i];

			if (_boundSrvDescs[i].ptr == 0) 
			{
				_boundSrvDescs[i] = _defaultSrv;
				continue;
			}

			if (srv != nullptr) {
				VOID_ASSERT_STATE_SET(srv);

				if (srv->_currentState != D3D12_RESOURCE_STATE_GENERIC_READ)
				{
					transition_resource(cmd, srv, VOID_GET_RESOURCE_STATE(srv), D3D12_RESOURCE_STATE_GENERIC_READ);
				}
			}
		}

		_device->CopyDescriptors(1, &gpuRange.first, &size, size, _boundSrvDescs, nullptr, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		cmd->SetGraphicsRootDescriptorTable(0, gpuRange.second);*/

		for(int i = 0; i < _maxSrvSections; i++)
		{
			auto& section = _boundSrvSections[i];
			if(section.dirty == true)
			{
				auto gpuRange = _allocators->gpuBufferDescHeap->allocate(8);
				for(int j = 0; j < 8; j++)
				{
					auto srv = section.resources[j];

					if(section.handles[j].ptr == 0)
					{
						section.handles[j] = _defaultSrv;
						continue;
					}

					if(srv != nullptr)
					{
						VOID_ASSERT_STATE_SET(srv);

						if (srv->_currentState != D3D12_RESOURCE_STATE_GENERIC_READ)
						{
							transition_resource(cmd, srv.get(), VOID_GET_RESOURCE_STATE(srv), D3D12_RESOURCE_STATE_GENERIC_READ);
						}
					}
				}

				UINT size = 8;

				_device->CopyDescriptors(1, &gpuRange.first, &size, size, section.handles, nullptr, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				auto tableSlot = i + 2;
				cmd->SetGraphicsRootDescriptorTable(tableSlot, gpuRange.second);

				section.dirty = false;
			}
		}

		_needGpuCommit ^= PSODirtyFlags::StructuredBuffers;
	}

	if((_needGpuCommit & PSODirtyFlags::ConstantBuffers) == ConstantBuffers)
	{
		auto gpuRange = _allocators->gpuBufferDescHeap->allocate(_maxCbvs);
		UINT size = _maxCbvs;

		for(uint32_t i = 0; i < _maxCbvs; i++)
		{
			auto cbv = _boundCbvs[i];

			if(_boundCbvDescs[i].ptr == 0)
			{
				_boundCbvDescs[i] = _defaultSrv; // todo??
				continue;
			}

			// don't think these need barriers
		}

		_device->CopyDescriptors(1, &gpuRange.first, &size, size, _boundCbvDescs, nullptr, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		cmd->SetGraphicsRootDescriptorTable(0, gpuRange.second);

		_needGpuCommit ^= PSODirtyFlags::ConstantBuffers;
	}

	if((_needGpuCommit & PSODirtyFlags::SamplerBuffers) == SamplerBuffers)
	{
		auto gpuRange = _allocators->gpuSamplerDescHeap->allocate(_maxSamplers);
		UINT size = _maxSamplers;

		for (uint32_t i = 0; i < _maxSamplers; i++)
		{
			if (_boundSamplerDescs[i].ptr == 0)
				_boundSamplerDescs[i] = _defaultSampler;
		}

		_device->CopyDescriptors(1, &gpuRange.first, &size, 1, _boundSamplerDescs, &size, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
		cmd->SetGraphicsRootDescriptorTable(1, gpuRange.second);

		_needGpuCommit ^= PSODirtyFlags::SamplerBuffers;
	}

	if((_needGpuCommit & PSODirtyFlags::RenderTargets) == RenderTargets || (_needGpuCommit & PSODirtyFlags::DepthBuffers) == DepthBuffers)
	{
		for(uint32_t i = 0; i < _stateDesc.numRenderTargets; i++)
		{
			auto rtv = _boundRtvs[i];

			if (rtv == nullptr)
				continue;

			VOID_ASSERT_STATE_SET(rtv);

			if(rtv->_currentState != D3D12_RESOURCE_STATE_RENDER_TARGET)
			{
				transition_resource(cmd, rtv.get(), VOID_GET_RESOURCE_STATE(rtv), D3D12_RESOURCE_STATE_RENDER_TARGET);
			}
		}

		auto dsv = _boundDsv;

		if (dsv != nullptr)
		{
			VOID_ASSERT_STATE_SET(dsv);

			if (dsv->_currentState != D3D12_RESOURCE_STATE_DEPTH_WRITE)
			{
				transition_resource(cmd, dsv.get(), VOID_GET_RESOURCE_STATE(dsv), D3D12_RESOURCE_STATE_DEPTH_WRITE);
			}
		}

		cmd->OMSetRenderTargets(_stateDesc.numRenderTargets, _boundRtvDescs, FALSE, dsv != nullptr ? &_boundDsvDesc : nullptr);

		_needGpuCommit ^= PSODirtyFlags::RenderTargets | PSODirtyFlags::DepthBuffers;
	}

	if((_needGpuCommit & PSODirtyFlags::IndexBuffer) == IndexBuffer)
	{
		//VOID_ASSERT(_boundIndexBuffer != nullptr, "index buffer was nullptr");

		if(_boundIndexBuffer != nullptr)
			cmd->IASetIndexBuffer(_boundIndexBuffer->get_d3d12_view());

		_needGpuCommit ^= PSODirtyFlags::IndexBuffer;
	}

	if((_dirtyFlags & PSODirtyFlags::InputPrimitive) == InputPrimitive)
	{
		cmd->IASetPrimitiveTopology(_boundInputTopology);

		_dirtyFlags ^= PSODirtyFlags::InputPrimitive;
	}
}

void D3D12_PipelineState::bind_pso_if_invalid(ID3D12GraphicsCommandList* cmd, bool onlyRoot)
{
	if(is_pso_dirty())
	{
		if (!onlyRoot)
		{
			auto pso = get_pso();
			cmd->SetPipelineState(pso);
		}
		cmd->SetGraphicsRootSignature(_rootSignature.Get());

		clean_pso();
	}
}

bool D3D12_PipelineState::is_pso_dirty()
{
	if((_dirtyFlags & ANY_STATE_DIRTY) != 0)
	{
		return true;
	}

	return false;
}

ID3D12PipelineState* D3D12_PipelineState::get_pso()
{
	return _stateCache->get_state(_stateDesc);
}

void D3D12_PipelineState::clean_pso()
{
	auto rtvDirty = _dirtyFlags & PSODirtyFlags::RenderTargets;
	auto inputTopDirty = _dirtyFlags & PSODirtyFlags::InputPrimitive;
	auto viewportDirty = _dirtyFlags & PSODirtyFlags::DrawExtent;
	auto depthDirty = _dirtyFlags & PSODirtyFlags::DepthBuffers;

	_dirtyFlags ^= ANY_STATE_DIRTY;

	// OR with this so that the resource binding stage can know if RTVs need to be bound.
	_dirtyFlags |= rtvDirty;

	// same with input topology
	_dirtyFlags |= inputTopDirty;

	// also viewport
	_dirtyFlags |= viewportDirty;

	// also depth...
	_dirtyFlags |= depthDirty;
}

void D3D12_PipelineState::transition_resource(ID3D12GraphicsCommandList* cmd, D3D12_VoidResource* resource, D3D12_RESOURCE_STATES oldState, D3D12_RESOURCE_STATES newState)
{
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = resource->get_d3d12_resource();
	barrier.Transition.StateBefore = oldState;
	barrier.Transition.StateAfter = newState;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	cmd->ResourceBarrier(1, &barrier);

	resource->_currentState = newState;
}

void D3D12_PipelineState::bind_view_scissor(ID3D12GraphicsCommandList* cmd, VoidExtent2D viewport, VoidExtent2D scissor)
{
	D3D12_VIEWPORT vp = {};
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	vp.Width = static_cast<float>(viewport.width);
	vp.Height = static_cast<float>(viewport.height);
	vp.MinDepth = 0.f;
	vp.MaxDepth = 1.f;
	cmd->RSSetViewports(1, &vp);

	D3D12_RECT scissorRect = {};
	scissorRect.left = 0;
	scissorRect.top = 0;
	scissorRect.right = static_cast<LONG>(scissor.width);
	scissorRect.bottom = static_cast<LONG>(scissor.height);

	cmd->RSSetScissorRects(1, &scissorRect);
}

std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> D3D12_PipelineState::get_cpu_handles(std::vector<std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE>>& list)
{
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> result;

	for(auto& h : list)
	{
		result.push_back(h.first);
	}

	return result;
}

std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> D3D12_PipelineState::get_gpu_handles(std::vector<std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE>>& list)
{
	std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> result;

	for (auto& h : list)
	{
		result.push_back(h.second);
	}

	return result;
}

void D3D12_PipelineState::commit_bound_to_cpu()
{
	if((_dirtyFlags & PSODirtyFlags::StructuredBuffers) == PSODirtyFlags::StructuredBuffers)
	{
		// SRVs dirty, bind em all
		/*for(uint32_t i = 0; i < _maxSrvs; i++)
		{
			auto resource = _boundSrvs[i];
			if (resource == nullptr) continue;
			_boundSrvDescs[i] = resource->get_d3d12_srv();
		}*/

		for(int i = 0; i < _maxSrvSections; i++)
		{
			auto& section = _boundSrvSections[i];
			if(section.dirty)
			{
				for(int j = 0; j < 8; j++)
				{
					auto resource = section.resources[j];
					if (resource == nullptr) continue;
					section.handles[j] = resource->get_d3d12_srv();
				}
			}
		}

		_dirtyFlags ^= PSODirtyFlags::StructuredBuffers;
		_needGpuCommit |= PSODirtyFlags::StructuredBuffers;
	}

	if((_dirtyFlags & PSODirtyFlags::ConstantBuffers) == PSODirtyFlags::ConstantBuffers)
	{
		// bind all CBVs
		for(uint32_t i = 0; i < _maxCbvs; i++)
		{
			auto resource = _boundCbvs[i];
			if (resource == nullptr) continue;
			_boundCbvDescs[i] = resource->get_d3d12_cbv();
		}

		_dirtyFlags ^= PSODirtyFlags::ConstantBuffers;
		_needGpuCommit |= PSODirtyFlags::ConstantBuffers;
	}

	if((_dirtyFlags & PSODirtyFlags::SamplerBuffers) == PSODirtyFlags::SamplerBuffers)
	{
		// bind all samplers
		for(uint32_t i = 0; i < _maxSamplers; i++)
		{
			auto sampler = _boundSamplers[i];
			if (sampler == nullptr) continue;
			_boundSamplerDescs[i] = sampler->get_d3d12_handle();
		}

		_dirtyFlags ^= PSODirtyFlags::SamplerBuffers;
		_needGpuCommit |= PSODirtyFlags::SamplerBuffers;
	}

	if((_dirtyFlags & PSODirtyFlags::RenderTargets) == PSODirtyFlags::RenderTargets)
	{
		// bind all rtv
		for(uint32_t i = 0; i < _maxRtvs; i++)
		{
			auto resource = _boundRtvs[i];
			if (resource == nullptr) continue;

			_boundRtvDescs[i] = resource->get_d3d12_rtv();
		}

		_dirtyFlags ^= PSODirtyFlags::RenderTargets;
		_needGpuCommit |= PSODirtyFlags::RenderTargets;
	}

	if((_dirtyFlags & PSODirtyFlags::IndexBuffer) == PSODirtyFlags::IndexBuffer)
	{
		// only one index buffer

		_dirtyFlags ^= PSODirtyFlags::IndexBuffer;
		_needGpuCommit |= PSODirtyFlags::IndexBuffer;
	}

	if((_dirtyFlags & PSODirtyFlags::DepthBuffers) == PSODirtyFlags::DepthBuffers)
	{
		auto resource = _boundDsv;
		if(resource != nullptr)
		{
			_boundDsvDesc = resource->get_d3d12_dsv();
		}

		_dirtyFlags ^= PSODirtyFlags::DepthBuffers;
		_needGpuCommit |= PSODirtyFlags::DepthBuffers;
	}
}

D3D12_PipelineState::PipelineAllocators::PipelineAllocators(ID3D12Device2* device)
{
	cpuBufferDescHeap = std::make_shared<CpuDescriptorAllocator<maxCpuBufferDescriptors>>();
	cpuDsvDescHeap = std::make_shared<CpuDescriptorAllocator<maxCpuDsvDescriptors>>();
	cpuRtvDescHeap = std::make_shared<CpuDescriptorAllocator<maxCpuRtvDescriptors>>();
	cpuSamplerDescHeap = std::make_shared<CpuDescriptorAllocator<maxCpuSamplerDescriptors>>();
	gpuBufferDescHeap = std::make_shared<GpuDescriptorAllocator<maxGpuBufferDescriptors>>(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, L"PipelineStateBufferHeap");
	gpuSamplerDescHeap = std::make_shared<GpuDescriptorAllocator<maxGpuSamplerDescriptors>>(device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, L"PipelineStateSamplerHeap");

	cpuBufferDescHeap->init(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	cpuDsvDescHeap->init(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	cpuRtvDescHeap->init(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	cpuSamplerDescHeap->init(device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
}
