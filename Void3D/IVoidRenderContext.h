#pragma once
#include <memory>
#include <SDL_events.h>
#include <SDL_video.h>

#include "VoidBuffers.h"
#include "VoidGraphicsStructs.h"
#include "VoidSampler.h"
#include "VoidShader.h"
#include "VoidImage.h"
#include "VoidResource.h"

class IVoidGraphicsCommandList;

enum VoidSamplerFilter
{
	Linear,
	Closest
};

enum VoidSamplerAddressMode
{
	Wrap
};

enum VoidImageFormat
{
	R8G8B8A8_UNORM,
	R32G32B32A32_FLOAT,
	R16G16B16A16_FLOAT,
	D32_FLOAT,
	R32_TYPELESS
};

enum VoidTopologyType
{
	Triangle
};

enum VoidInputTopologyType
{
	TriangleList
};

enum VoidCullMode
{
	CullBack,
	CullFront,
	CullNone
};

enum VoidFillMode
{
	Solid,
	Wireframe
};

enum class VoidResourceState
{
	Unknown,
	ShaderRead,
	DepthWrite,
	RenderTarget,
	CopySrc,
	CopyDst,
};

struct Void3DMemoryStats
{
	uint64_t available;
	uint64_t used;
};

class IVoidRenderContext
{
public:
	IVoidRenderContext() = default;
	virtual ~IVoidRenderContext() = default;

	static IVoidRenderContext* create();

	// init
	virtual void init(VoidExtent2D windowExtent, SDL_Window* windowHandle) = 0;
	virtual void shutdown() = 0;

	virtual void window_resized(VoidExtent2D newSize) = 0;

	// frame
	virtual void present() = 0;

	// drawing
	virtual void draw_indexed(uint32_t indexCount, uint32_t firstIndex) = 0;
	virtual void draw_instanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation) = 0;

	// compute dispatch
	virtual void dispatch_mesh(uint32_t x, uint32_t y, uint32_t z) = 0;

	// resource state management
	virtual void transition_resource(std::shared_ptr<VoidResource> resource, VoidResourceState oldState, VoidResourceState newState) = 0;

	// operations
	virtual void blit_to_render_target(std::shared_ptr<VoidImage> src, std::shared_ptr<VoidImage> dest) = 0;

	// shader binding
	virtual void set_vertex_shader(std::shared_ptr<VoidShader> shader) = 0;
	virtual void set_geometry_shader(std::shared_ptr<VoidShader> shader) = 0;
	virtual void set_pixel_shader(std::shared_ptr<VoidShader> shader) = 0;
	virtual void set_mesh_shader(std::shared_ptr<VoidShader> shader) = 0;
	virtual void set_amp_shader(std::shared_ptr<VoidShader> shader) = 0;
	virtual void set_shader_root_path(std::string path) = 0;

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

	// resource creation
	virtual std::shared_ptr<VoidStructuredBuffer> create_structured(size_t size, bool dynamic = false) = 0;
	virtual std::shared_ptr<VoidSampler> create_sampler(VoidSamplerFilter filter, VoidSamplerAddressMode uAddress, VoidSamplerAddressMode vAddress, VoidSamplerAddressMode wAddress, float minLOD, float maxLOD) = 0;
	virtual std::shared_ptr<VoidImage> create_image(VoidExtent3D size, VoidImageFormat format, VoidResourceFlags flags = VoidResourceFlags::None, VoidResourceState initialState = VoidResourceState::ShaderRead) = 0;
	virtual std::shared_ptr<VoidIndexBuffer> create_index_buffer(uint32_t* pIndices, size_t count) = 0;
	virtual std::shared_ptr<VoidShader> create_shader(std::string path, VoidShaderStage stage) = 0;

	// command lists
	virtual std::shared_ptr<IVoidGraphicsCommandList> allocate_graphics_list() = 0;
	virtual void submit_graphics_list(std::shared_ptr<IVoidGraphicsCommandList> list) = 0;

	// view creation
	virtual void register_resource_view(VoidStructuredBuffer* buffer) = 0;
	virtual void register_image_view(VoidImage* image) = 0;
	virtual void register_render_target_view(VoidImage* target) = 0;
	virtual void register_resource_view(VoidStructuredBuffer* buffer, size_t numElements, size_t elementSize) = 0;
	virtual void register_constant_view(VoidStructuredBuffer* buffer) = 0;
	virtual void register_depth_stencil_view(VoidImage* target) = 0;

	// image loading
	virtual void copy_image(void* data, VoidImage* image) = 0;
	virtual void clear_depth(float value, VoidImage* image) = 0;
	virtual void clear_image(float rgba[4], VoidImage* image) = 0;

	// resource release
	virtual void release_sampler(VoidSampler* sampler) = 0;
	virtual void release_shader(VoidShader* shader) = 0;
	virtual void release_resource(VoidResource* resource) = 0;

	// resource binding
	virtual void bind_resource(uint16_t slot, std::shared_ptr<VoidStructuredBuffer> buffer) = 0;
	virtual void bind_resource(uint16_t slot, std::shared_ptr<VoidImage> image) = 0;
	virtual void bind_sampler(uint16_t slot, std::shared_ptr<VoidSampler> sampler) = 0;
	virtual void bind_render_target(uint16_t slot, std::shared_ptr<VoidImage> target) = 0;
	virtual void bind_index_buffer(std::shared_ptr<VoidIndexBuffer> buffer) = 0;
	virtual void bind_depth_target(std::shared_ptr<VoidImage> target) = 0;
	virtual void bind_constant(uint16_t slot, std::shared_ptr<VoidStructuredBuffer> buffer) = 0;

	// util
	virtual std::shared_ptr<VoidImage> get_back_buffer() = 0;
	virtual VoidExtent2D get_current_draw_extent() = 0;
	/*virtual std::shared_ptr<VoidSampler> get_default_linear_sampler() = 0;
	virtual std::shared_ptr<VoidSampler> get_default_nearest_sampler() = 0;*/
	virtual void copy_buffer(VoidStructuredBuffer* src, VoidStructuredBuffer* dst) = 0;
	virtual uint8_t get_frame_index() = 0;
	virtual Void3DMemoryStats get_memory_usage_stats() = 0;

	static IVoidRenderContext* g;
protected:
	static constexpr uint8_t _numFrames = 2;
	VoidExtent2D _windowExtent;
	bool _vSyncEnabled = true;
	bool _tearingSupported = false;
	bool _fullscreen = false;
};
