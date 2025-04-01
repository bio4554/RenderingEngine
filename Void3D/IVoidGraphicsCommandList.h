#pragma once
#include "IVoidRenderContext.h"

class IVoidGraphicsCommandList
{
public:
	// drawing
	virtual void draw_indexed(uint32_t indexCount, uint32_t firstIndex) = 0;
	virtual void draw_instanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation) = 0;

	// shader binding
	virtual void set_vertex_shader(std::shared_ptr<VoidShader> shader) = 0;
	virtual void set_geometry_shader(std::shared_ptr<VoidShader> shader) = 0;
	virtual void set_pixel_shader(std::shared_ptr<VoidShader> shader) = 0;

	virtual void clear_depth(float value, std::shared_ptr<VoidImage> depth) = 0;
	virtual void clear_image(float rgba[4], VoidImage* image) = 0;

	virtual void transition_resource(std::shared_ptr<VoidImage> resource, VoidResourceState oldState, VoidResourceState newState) = 0;
	virtual void transition_resource(std::shared_ptr<VoidStructuredBuffer> resource, VoidResourceState oldState, VoidResourceState newState) = 0;

	// render state
	virtual void set_depth_test_enabled(bool enabled) = 0;
	virtual void set_stencil_test_enabled(bool enabled) = 0;
	virtual void set_primitive(VoidTopologyType type) = 0;
	virtual void set_input_primitive(VoidInputTopologyType type) = 0;
	virtual void set_num_render_targets(uint32_t count) = 0;
	virtual void set_cull_mode(VoidCullMode mode) = 0;
	virtual void set_draw_extent(VoidExtent2D extent) = 0;
	virtual void set_fill_mode(VoidFillMode mode) = 0;
	virtual void set_depth_clip(bool enabled) = 0;

	// resource binding
	virtual void bind_resource(uint16_t slot, std::shared_ptr<VoidStructuredBuffer> buffer) = 0;
	virtual void bind_resource(uint16_t slot, std::shared_ptr<VoidImage> image) = 0;
	virtual void bind_sampler(uint16_t slot, std::shared_ptr<VoidSampler> sampler) = 0;
	virtual void bind_render_target(uint16_t slot, std::shared_ptr<VoidImage> target) = 0;
	virtual void bind_index_buffer(std::shared_ptr<VoidIndexBuffer> buffer) = 0;
	virtual void bind_depth_target(std::shared_ptr<VoidImage> target) = 0;
	virtual void bind_constant(uint16_t slot, std::shared_ptr<VoidStructuredBuffer> buffer) = 0;
};
