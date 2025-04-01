#include "pch.h"
#include "BlitToBackBufferNode.h"

#include "IVoidGraphicsCommandList.h"

namespace star::renderer
{
	void BlitToBackBufferNode::Execute(RenderContext& context)
	{
		auto backBuffer = context.context->get_back_buffer();
		auto drawImage = context.resources->Get<VoidImage>("mainDrawBuffer");
		auto vertShader = context.shaders->GetShader("BlitVertShader.cso", Vertex);
		auto pixelShader = context.shaders->GetShader("BlitPixelShader.cso", Pixel);

		context.context->set_pixel_shader(pixelShader);
		context.context->set_vertex_shader(vertShader);
		context.context->set_mesh_shader(nullptr);
		context.context->set_depth_test_enabled(false);
		context.context->set_cull_mode(CullNone);
		context.context->bind_render_target(0, backBuffer);
		context.context->set_num_render_targets(1);
		context.context->bind_resource(0, drawImage);
		context.context->draw_instanced(3, 1, 0, 0);
	}
}
