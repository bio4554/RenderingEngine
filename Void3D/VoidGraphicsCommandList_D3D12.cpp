#include "pch.h"
#include "VoidGraphicsCommandList_D3D12.h"

#include "D3D12_Translator.h"

void VoidGraphicsCommandList_D3D12::draw_indexed(uint32_t indexCount, uint32_t firstIndex)
{
	pre_draw();

	_commandList.Get()->DrawIndexedInstanced(indexCount, 1, firstIndex, 0, 0);
}

void VoidGraphicsCommandList_D3D12::draw_instanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation)
{
	pre_draw();
	_commandList.Get()->DrawInstanced(vertexCount, instanceCount, startVertexLocation, startInstanceLocation);
}

void VoidGraphicsCommandList_D3D12::set_vertex_shader(std::shared_ptr<VoidShader> shader)
{
	auto vs = reinterpret_cast<D3D12_VoidShader*>(shader.get());
	_state->bind_vertex_shader(vs);
}

void VoidGraphicsCommandList_D3D12::set_geometry_shader(std::shared_ptr<VoidShader> shader)
{
	auto gs = reinterpret_cast<D3D12_VoidShader*>(shader.get());
	_state->bind_geo_shader(gs);
}

void VoidGraphicsCommandList_D3D12::set_pixel_shader(std::shared_ptr<VoidShader> shader)
{
	auto ps = reinterpret_cast<D3D12_VoidShader*>(shader.get());
	_state->bind_pixel_shader(ps);
}

void VoidGraphicsCommandList_D3D12::set_depth_clip(bool enabled)
{
	_state->set_depth_clip(enabled);
}

void VoidGraphicsCommandList_D3D12::set_depth_test_enabled(bool enabled)
{
	_state->set_enable_depth(enabled);
}

void VoidGraphicsCommandList_D3D12::set_cull_mode(VoidCullMode mode)
{
	_state->set_cull_mode(D3D12_Translator::cull_mode(mode));
}

void VoidGraphicsCommandList_D3D12::set_draw_extent(VoidExtent2D extent)
{
	_state->set_draw_extent(extent);
}

void VoidGraphicsCommandList_D3D12::set_fill_mode(VoidFillMode mode)
{
	_state->set_fill_mode(D3D12_Translator::fill_mode(mode));
}

void VoidGraphicsCommandList_D3D12::set_input_primitive(VoidInputTopologyType type)
{
	_state->set_input_topology(D3D12_Translator::input_topology(type));
}

void VoidGraphicsCommandList_D3D12::set_num_render_targets(uint32_t count)
{
	_state->set_num_rt(count);
}

void VoidGraphicsCommandList_D3D12::set_primitive(VoidTopologyType type)
{
	_state->set_topology(D3D12_Translator::topology(type));
}

void VoidGraphicsCommandList_D3D12::set_stencil_test_enabled(bool enabled)
{
	_state->set_enable_stencil(enabled);
}

void VoidGraphicsCommandList_D3D12::clear_depth(float value, std::shared_ptr<VoidImage> depth)
{
	auto ds = reinterpret_cast<D3D12_VoidImage*>(depth.get());

	VOID_ASSERT(ds->_dsvDescriptor.has_value(), "can't clear, no dsv");
	VOID_ASSERT_STATE_SET(ds);

	if (ds->_currentState != D3D12_RESOURCE_STATE_DEPTH_WRITE)
	{
		transition_resource(_commandList.Get(), ds->get_d3d12_resource(), VOID_GET_RESOURCE_STATE(ds), D3D12_RESOURCE_STATE_DEPTH_WRITE);
		ds->_currentState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	}

	_commandList->ClearDepthStencilView(ds->get_d3d12_dsv(), D3D12_CLEAR_FLAG_DEPTH, value, 0, 0, nullptr);
}

void VoidGraphicsCommandList_D3D12::clear_image(float rgba[4], VoidImage* image)
{
	auto rtv = reinterpret_cast<D3D12_VoidImage*>(image);

	VOID_ASSERT(rtv->_rtvDescriptor.has_value(), "can't clear, no rtv");
	VOID_ASSERT_STATE_SET(rtv);

	if (rtv->_currentState != D3D12_RESOURCE_STATE_RENDER_TARGET)
	{
		transition_resource(_commandList.Get(), rtv->get_d3d12_resource(), VOID_GET_RESOURCE_STATE(rtv), D3D12_RESOURCE_STATE_RENDER_TARGET);
		rtv->_currentState = D3D12_RESOURCE_STATE_RENDER_TARGET;
	}

	_commandList->ClearRenderTargetView(rtv->get_d3d12_rtv(), rgba, 0, nullptr);
}

void VoidGraphicsCommandList_D3D12::bind_constant(uint16_t slot, std::shared_ptr<VoidStructuredBuffer> buffer)
{
	_resourceLocks.push(buffer);
	auto b = std::dynamic_pointer_cast<D3D12_VoidStructuredBuffer>(buffer);
	_state->bind_constant(slot, b);
}

void VoidGraphicsCommandList_D3D12::bind_depth_target(std::shared_ptr<VoidImage> target)
{
	_resourceLocks.push(target);
	auto rt = std::dynamic_pointer_cast<D3D12_VoidImage>(target);
	_state->bind_depth_target(rt);
}

void VoidGraphicsCommandList_D3D12::bind_index_buffer(std::shared_ptr<VoidIndexBuffer> buffer)
{
	_resourceLocks.push(buffer);
	auto b = std::dynamic_pointer_cast<D3D12_VoidIndexBuffer>(buffer);
	_state->bind_index_buffer(b);
}

void VoidGraphicsCommandList_D3D12::bind_render_target(uint16_t slot, std::shared_ptr<VoidImage> target)
{
	_resourceLocks.push(target);
	auto rt = std::dynamic_pointer_cast<D3D12_VoidImage>(target);
	_state->bind_render_target(slot, rt);
}

void VoidGraphicsCommandList_D3D12::bind_resource(uint16_t slot, std::shared_ptr<VoidImage> image)
{
	_resourceLocks.push(image);
	auto i = std::dynamic_pointer_cast<D3D12_VoidImage>(image);
	_state->bind_resource(slot, i);
}

void VoidGraphicsCommandList_D3D12::bind_resource(uint16_t slot, std::shared_ptr<VoidStructuredBuffer> buffer)
{
	_resourceLocks.push(buffer);
	auto b = std::dynamic_pointer_cast<D3D12_VoidStructuredBuffer>(buffer);
	_state->bind_resource(slot, b);
}

void VoidGraphicsCommandList_D3D12::bind_sampler(uint16_t slot, std::shared_ptr<VoidSampler> sampler)
{
	_resourceLocks.push(sampler);
	auto s = std::dynamic_pointer_cast<D3D12_VoidSampler>(sampler);
	_state->bind_resource(slot, s);
}

void VoidGraphicsCommandList_D3D12::transition_resource(std::shared_ptr<VoidImage> resource, VoidResourceState oldState, VoidResourceState newState)
{
	auto d3d12Resource = reinterpret_cast<D3D12_VoidImage*>(resource.get());

	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = d3d12Resource->_d3d12Resource;
	barrier.Transition.StateBefore = D3D12_Translator::resource_state(oldState);
	barrier.Transition.StateAfter = D3D12_Translator::resource_state(newState);
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	_commandList->ResourceBarrier(1, &barrier);
}

void VoidGraphicsCommandList_D3D12::transition_resource(std::shared_ptr<VoidStructuredBuffer> resource, VoidResourceState oldState, VoidResourceState newState)
{
	auto d3d12Resource = reinterpret_cast<D3D12_VoidStructuredBuffer*>(resource.get());

	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = d3d12Resource->_d3d12Resource;
	barrier.Transition.StateBefore = D3D12_Translator::resource_state(oldState);
	barrier.Transition.StateAfter = D3D12_Translator::resource_state(newState);
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	_commandList->ResourceBarrier(1, &barrier);
}


void VoidGraphicsCommandList_D3D12::reset_list()
{
	_commandAllocator->Reset();
	_commandList->Reset(_commandAllocator.Get(), nullptr);

	_state->frame_ended(_commandList.Get());
}

void VoidGraphicsCommandList_D3D12::pre_draw()
{
	_state->bind_pso_if_invalid(_commandList.Get());
	_state->bind_shader_resources(_commandList.Get());
	_state->ensure_resource_states(_commandList.Get());
}

void VoidGraphicsCommandList_D3D12::transition_resource(ID3D12GraphicsCommandList* cmd, ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
{
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = resource;
	barrier.Transition.StateBefore = before;
	barrier.Transition.StateAfter = after;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	cmd->ResourceBarrier(1, &barrier);
}
