#include "pch.h"
#include "ForwardDrawSceneNode.h"

#include "IVoidGraphicsCommandList.h"
#include "ResourceRegistry.h"
#include "../imgui/imgui.h"

namespace star::renderer
{
	void ForwardDrawSceneNode::Execute(RenderContext& context)
	{
		auto drawBuffer = context.resources->Get<VoidImage>("mainDrawBuffer");
		auto depthBuffer = context.resources->Get<VoidImage>("mainDepthBuffer");
		auto vertShader = context.shaders->GetShader("VertexShader.cso", Vertex);
		auto meshShader = context.shaders->GetShader("MeshShader.cso", Mesh);
		auto pixelShader = context.shaders->GetShader("PixelShader.cso", Pixel);
		auto pbrPixelShader = context.shaders->GetShader("PBRPixel.cso", Pixel);


		static bool forceNormalShade = false;

		ImGui::Begin("Force normal shading");
		ImGui::Checkbox("Enabled", &forceNormalShade);
		ImGui::End();

		//context.context->set_vertex_shader(vertShader);

		context.context->set_amp_shader(nullptr);
		context.context->set_mesh_shader(meshShader);
		context.context->set_pixel_shader(pixelShader);

		context.context->set_input_primitive(TriangleList);
		context.context->set_primitive(Triangle);
		context.context->set_draw_extent(context.drawExtent);
		context.context->set_fill_mode(Solid);
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
			std::shared_ptr<graphics::Texture> roughness = nullptr;
			std::shared_ptr<graphics::Texture> metalness = nullptr;
			std::shared_ptr<graphics::Texture> alpha = nullptr;

			if (material->normal.has_value())
			{
				normal = resource::GResourceRegistry->TryGetTextureData(material->normal.value());
			}

			if(material->roughness.has_value())
			{
				roughness = resource::GResourceRegistry->TryGetTextureData(material->roughness.value());
			}

			if(material->metalness.has_value())
			{
				metalness = resource::GResourceRegistry->TryGetTextureData(material->metalness.value());
			}

			if(material->alpha.has_value())
			{
				alpha = resource::GResourceRegistry->TryGetTextureData(material->alpha.value());
			}

			{
				auto drawConstants = graphics::DrawConstants();
				drawConstants.localToWorld = r.worldMatrix;
				drawConstants.hasNormal = normal != nullptr ? 1 : 0;
				drawConstants.hasRoughness = roughness != nullptr ? 1 : 0;
				drawConstants.hasMetalness = metalness != nullptr ? 1 : 0;
				drawConstants.hasAlpha = alpha != nullptr ? 1 : 0;
				drawConstants.meshletCount = mesh->numMeshlets;
				auto pConstants = r.constants->map();
				memcpy(pConstants, &drawConstants, sizeof(graphics::DrawConstants));
				r.constants->unmap();
			}

			if (normal != nullptr)
			{
				context.context->bind_resource(11, normal->imageResource);
			}

			if (metalness != nullptr)
			{
				context.context->bind_resource(12, metalness->imageResource);
			}

			if(roughness != nullptr)
			{
				context.context->bind_resource(13, roughness->imageResource);
			}

			if(alpha != nullptr)
			{
				context.context->bind_resource(14, alpha->imageResource);
			}

			if(metalness != nullptr && roughness != nullptr && !forceNormalShade)
			{
				// we have the required stuff for PBR
				context.context->set_pixel_shader(pbrPixelShader);
			}
			else
			{
				//continue;
				context.context->set_pixel_shader(pixelShader);
			}

			context.context->bind_resource(10, texture->imageResource);
			context.context->bind_resource(0, mesh->meshletBuffer);
			context.context->bind_resource(1, mesh->vertexBuffer);
			context.context->bind_constant(0, r.constants);
			context.context->dispatch_mesh(mesh->numMeshlets, 1, 1);
			//context.context->draw_indexed(mesh->numIndices, 0);
		}
	}
}
