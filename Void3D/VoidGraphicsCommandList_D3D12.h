#pragma once
#include <queue>
#include <wrl/client.h>

#include <utility>

#include "D3D12_PipelineState.h"
#include "IVoidGraphicsCommandList.h"

class VoidGraphicsCommandList_D3D12 : public IVoidGraphicsCommandList
{
public:
	friend class VoidRenderContext_D3D12;

	VoidGraphicsCommandList_D3D12(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, ComPtr<ID3D12CommandAllocator> allocator, ID3D12Device10* device, VoidExtent2D* viewport)
	{
		_commandList = commandList;
		_state = std::make_unique<D3D12_PipelineState>(device);
		_state->init(*viewport);
		_state->frame_ended(_commandList.Get());
		_commandAllocator = allocator;
		_viewport = viewport;
	}

	void draw_indexed(uint32_t indexCount, uint32_t firstIndex) override;
	void draw_instanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation) override;

	void set_vertex_shader(std::shared_ptr<VoidShader> shader) override;
	void set_geometry_shader(std::shared_ptr<VoidShader> shader) override;
	void set_pixel_shader(std::shared_ptr<VoidShader> shader) override;

	void set_depth_clip(bool enabled) override;
	void set_depth_test_enabled(bool enabled) override;
	void set_cull_mode(VoidCullMode mode) override;
	void set_draw_extent(VoidExtent2D extent) override;
	void set_fill_mode(VoidFillMode mode) override;
	void set_input_primitive(VoidInputTopologyType type) override;
	void set_num_render_targets(uint32_t count) override;
	void set_primitive(VoidTopologyType type) override;
	void set_stencil_test_enabled(bool enabled) override;

	void clear_depth(float value, std::shared_ptr<VoidImage> depth) override;
	void clear_image(float rgba[4], VoidImage* image) override;

	void bind_constant(uint16_t slot, std::shared_ptr<VoidStructuredBuffer> buffer) override;
	void bind_depth_target(std::shared_ptr<VoidImage> target) override;
	void bind_index_buffer(std::shared_ptr<VoidIndexBuffer> buffer) override;
	void bind_render_target(uint16_t slot, std::shared_ptr<VoidImage> target) override;
	void bind_resource(uint16_t slot, std::shared_ptr<VoidImage> image) override;
	void bind_resource(uint16_t slot, std::shared_ptr<VoidStructuredBuffer> buffer) override;
	void bind_sampler(uint16_t slot, std::shared_ptr<VoidSampler> sampler) override;

	void transition_resource(std::shared_ptr<VoidImage> resource, VoidResourceState oldState, VoidResourceState newState) override;
	void transition_resource(std::shared_ptr<VoidStructuredBuffer> resource, VoidResourceState oldState, VoidResourceState newState) override;

	void reset_list();

	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> _commandList;
private:
	void pre_draw();
	void transition_resource(ID3D12GraphicsCommandList* cmd, ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);

	
	ComPtr<ID3D12CommandAllocator> _commandAllocator;
	std::unique_ptr<D3D12_PipelineState> _state;
	std::queue<std::shared_ptr<void>> _resourceLocks;
	VoidExtent2D* _viewport;
};