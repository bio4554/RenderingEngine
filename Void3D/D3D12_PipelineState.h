#pragma once
#include <memory>
#include <wrl/client.h>

#include "D3D12_PipelineStateObjectCache.h"
#include "D3D12_VoidBuffers.h"
#include "D3D12_VoidImage.h"
#include "D3D12_VoidSampler.h"
#include "DescriptorAllocator.h"
#include "VoidBuffers.h"
#include "VoidGraphicsStructs.h"
#include "VoidSampler.h"
#include "VoidShader.h"

using Microsoft::WRL::ComPtr;

enum PSODirtyFlags
{
	Clean = 0,
	VertexShader = 1,
	PixelShader = 2,
	GeoShader = 4,
	StructuredBuffers = 8,
	SamplerBuffers = 16,
	RenderTargets = 32,
	DepthTest = 64,
	Stencil = 128,
	Topology = 256,
	CullMode = 512,
	IndexBuffer = 1024,
	InputPrimitive = 2048,
	DrawExtent = 4096,
	FillMode = 8192,
	ConstantBuffers = 16384,
	DepthBuffers = 32768,
	MeshShader = 65536,
	AmpShader = 131072
};

#define ANY_SHADER_DIRTY (VertexShader | GeoShader | PixelShader | MeshShader | AmpShader)

#define ANY_STATE_DIRTY (VertexShader | GeoShader | PixelShader | MeshShader | AmpShader | RenderTargets | DepthTest | Stencil | Topology | CullMode | InputPrimitive | FillMode | DepthBuffers)

#define ANY_RESOURCE_DIRTY (StructuredBuffers | SamplerBuffers | IndexBuffer | ConstantBuffers)

class D3D12_PipelineState
{
public:
	static void init(ID3D12Device10* device);
	static void shutdown_static();

	D3D12_PipelineState(ID3D12Device10* device);
	~D3D12_PipelineState();

	void init(VoidExtent2D viewport);
	void bind_vertex_shader(D3D12_VoidShader* shader);
	void bind_geo_shader(D3D12_VoidShader* shader);
	void bind_pixel_shader(D3D12_VoidShader* shader);
	void bind_mesh_shader(D3D12_VoidShader* shader);
	void bind_amp_shader(D3D12_VoidShader* shader);
	void bind_resource(uint16_t slot, std::shared_ptr<D3D12_VoidStructuredBuffer> buffer);
	void bind_resource(uint16_t slot, std::shared_ptr<D3D12_VoidImage> texture);
	void bind_resource(uint16_t slot, std::shared_ptr<D3D12_VoidSampler> sampler);
	void bind_constant(uint16_t slot, std::shared_ptr<D3D12_VoidStructuredBuffer> buffer);
	void bind_render_target(uint16_t slot, std::shared_ptr<D3D12_VoidImage> target);
	void bind_depth_target(std::shared_ptr<D3D12_VoidImage> target);
	void bind_index_buffer(std::shared_ptr<D3D12_VoidIndexBuffer> buffer);
	void force_bind_state(ID3D12GraphicsCommandList* cmd);
	std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> allocate_buffer_desc();
	void ensure_resource_states(ID3D12GraphicsCommandList* cmd);

	void set_enable_depth(bool enabled);
	void set_enable_stencil(bool enabled);
	void set_topology(D3D12_PRIMITIVE_TOPOLOGY_TYPE type);
	void set_input_topology(D3D_PRIMITIVE_TOPOLOGY type);
	void set_num_rt(uint32_t num);
	void set_cull_mode(D3D12_CULL_MODE mode);
	void set_draw_extent(VoidExtent2D extent);
	void set_fill_mode(D3D12_FILL_MODE mode);
	void set_depth_clip(bool enabled);

	void frame_ended(ID3D12GraphicsCommandList* cmd);

	void bind_shader_resources(ID3D12GraphicsCommandList* cmd);
	void bind_pso_if_invalid(ID3D12GraphicsCommandList* cmd, bool onlyRoot = false);
	ID3D12PipelineState* get_pso();
	const D3D12_PipelineCacheDesc& get_desc() const
	{
		return _stateDesc;
	}

private:
	struct DescriptorSection
	{
		bool dirty = false;
		D3D12_CPU_DESCRIPTOR_HANDLE handles[8];
		std::shared_ptr<D3D12_VoidResource> resources[8];
	};

	static constexpr uint16_t _maxCbvs = 16;
	static constexpr uint16_t _maxUavs = 16;
	//static constexpr uint16_t _maxSrvs = 32;
	static constexpr uint16_t _maxSrvSections = 8;
	static constexpr uint16_t _maxSamplers = 16;
	static constexpr uint16_t _maxRtvs = 8;

	

	/*bool _cbvDirtyFlags[_maxCbvs];
	bool _uavDirtyFlags[_maxUavs];
	bool _srvDirtyFlags[_maxSrvs];*/
	bool _samplerDirtyFlags[_maxSamplers];
	uint32_t _dirtyFlags;
	uint32_t _needGpuCommit;
	uint32_t _needHeapBind = 1;
	VoidExtent2D _viewport;

	static D3D12_CPU_DESCRIPTOR_HANDLE _defaultSrv;
	static D3D12_CPU_DESCRIPTOR_HANDLE _defaultSampler;

	ID3D12Device10* _device;
	static ComPtr<ID3D12RootSignature> _rootSignature;

	struct PipelineAllocators {
		PipelineAllocators(ID3D12Device2* device);

		// descriptors
		static constexpr size_t maxCpuBufferDescriptors = 1000; // tune later
		static constexpr size_t maxCpuRtvDescriptors = 1000;
		static constexpr size_t maxCpuDsvDescriptors = 1000;
		static constexpr size_t maxCpuSamplerDescriptors = 1000;
		static constexpr size_t maxGpuBufferDescriptors = 1000000;
		static constexpr size_t maxGpuSamplerDescriptors = 2048;
		std::shared_ptr<CpuDescriptorAllocator<maxCpuBufferDescriptors>> cpuBufferDescHeap = nullptr;
		std::shared_ptr<CpuDescriptorAllocator<maxCpuRtvDescriptors>> cpuRtvDescHeap = nullptr;
		std::shared_ptr<CpuDescriptorAllocator<maxCpuDsvDescriptors>> cpuDsvDescHeap = nullptr;
		std::shared_ptr<CpuDescriptorAllocator<maxCpuSamplerDescriptors>> cpuSamplerDescHeap = nullptr;
		std::shared_ptr<GpuDescriptorAllocator<maxGpuBufferDescriptors>> gpuBufferDescHeap = nullptr;
		std::shared_ptr<GpuDescriptorAllocator<maxGpuSamplerDescriptors>> gpuSamplerDescHeap = nullptr;
	};

	static std::unique_ptr<PipelineAllocators> _allocators;

	std::shared_ptr<D3D12_VoidResource> _boundCbvs[_maxCbvs];
	DescriptorSection _boundSrvSections[_maxSrvSections];
	std::shared_ptr<D3D12_VoidSampler> _boundSamplers[_maxSamplers];
	std::shared_ptr<D3D12_VoidImage> _boundRtvs[_maxRtvs];
	std::shared_ptr<D3D12_VoidIndexBuffer> _boundIndexBuffer;
	std::shared_ptr<D3D12_VoidImage> _boundDsv;

	D3D_PRIMITIVE_TOPOLOGY _boundInputTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	D3D12_CPU_DESCRIPTOR_HANDLE _boundCbvDescs[_maxCbvs];
	//D3D12_CPU_DESCRIPTOR_HANDLE _boundSrvDescs[_maxSrvs];
	D3D12_CPU_DESCRIPTOR_HANDLE _boundSamplerDescs[_maxSamplers];
	D3D12_CPU_DESCRIPTOR_HANDLE _boundRtvDescs[_maxRtvs];
	D3D12_CPU_DESCRIPTOR_HANDLE _boundDsvDesc;

	D3D12_PipelineCacheDesc _stateDesc;
	static D3D12_PipelineStateObjectCache* _stateCache;

	// methods
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> get_cpu_handles(std::vector<std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE>>& list);
	std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> get_gpu_handles(std::vector<std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE>>& list);

	void commit_bound_to_cpu();
	bool is_pso_dirty();
	void clean_pso();
	void transition_resource(ID3D12GraphicsCommandList* cmd, D3D12_VoidResource* resource, D3D12_RESOURCE_STATES oldState, D3D12_RESOURCE_STATES newState);
	void bind_view_scissor(ID3D12GraphicsCommandList* cmd, VoidExtent2D viewport, VoidExtent2D scissor);
};
