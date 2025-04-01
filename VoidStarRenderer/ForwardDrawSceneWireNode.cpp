#include "pch.h"
#include "ForwardDrawSceneWireNode.h"

#include "IVoidGraphicsCommandList.h"
#include "ResourceRegistry.h"

namespace star::renderer
{
	void ForwardDrawSceneWireNode::Execute(RenderContext& context)
	{
		return;
		auto drawBuffer = context.resources->Get<VoidImage>("mainDrawBuffer");
		auto depthBuffer = context.resources->Get<VoidImage>("mainDepthBuffer");
		auto vertShader = context.shaders->GetShader("VertexShader.cso", Vertex);
		auto meshShader = context.shaders->GetShader("MeshShader.cso", Mesh);
		auto pixelShader = context.shaders->GetShader("PixelWire.cso", Pixel);


		//context.context->set_vertex_shader(vertShader);

		context.context->set_mesh_shader(meshShader);
		context.context->set_pixel_shader(pixelShader);

		context.context->set_input_primitive(TriangleList);
		context.context->set_primitive(Triangle);
		context.context->set_draw_extent(context.drawExtent);
		context.context->set_fill_mode(Wireframe);
		context.context->set_cull_mode(CullBack);
		context.context->set_depth_test_enabled(true);

		context.context->bind_render_target(0, drawBuffer);
		context.context->set_num_render_targets(1);
		context.context->bind_depth_target(depthBuffer);

		for (auto& r : context.frameData.render_proxies)
		{
			auto mesh = resource::GResourceRegistry->TryGetMeshData(r.meshHandle);
			if (mesh == nullptr)
				continue;

			auto material = resource::GResourceRegistry->TryGetMaterialData(mesh->material);
			if (material == nullptr)
				continue;

			auto texture = resource::GResourceRegistry->TryGetTextureData(material->albedo);
			if (texture == nullptr)
				continue;

			std::shared_ptr<graphics::Texture> normal = nullptr;

			if (material->normal.has_value())
			{
				normal = resource::GResourceRegistry->TryGetTextureData(material->normal.value());
			}

			{
				auto drawConstants = graphics::DrawConstants();
				drawConstants.localToWorld = r.worldMatrix;
				drawConstants.hasNormal = normal != nullptr ? 1 : 0;
				drawConstants.meshletCount = mesh->numMeshlets;
				auto pConstants = r.constants->map();
				memcpy(pConstants, &drawConstants, sizeof(graphics::DrawConstants));
				r.constants->unmap();
			}

			if (normal != nullptr)
			{
				context.context->bind_resource(3, normal->imageResource);
			}

			context.context->bind_resource(2, texture->imageResource);
			context.context->bind_resource(4, mesh->meshletBuffer);
			context.context->bind_resource(0, mesh->vertexBuffer);
			context.context->bind_constant(0, r.constants);
			context.context->dispatch_mesh(mesh->numMeshlets, 1, 1);
			//context.context->draw_indexed(mesh->numIndices, 0);
		}
	}

}