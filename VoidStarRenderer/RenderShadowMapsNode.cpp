#include "pch.h"
#include "RenderShadowMapsNode.h"

#include <glm/ext/matrix_clip_space.hpp>

#include "ResourceRegistry.h"
#include "../imgui/imgui.h"

namespace star::renderer
{
	void RenderShadowMapsNode::Execute(RenderContext& context)
	{
		assert(context.world != nullptr && context.world->active_camera != nullptr);

		auto meshShader = context.shaders->GetShader("ShadowMeshShader.cso", Mesh);
		auto pixelShader = context.shaders->GetShader("ShadowPixel.cso", Pixel);
		auto ampShader = context.shaders->GetShader("ShadowAmpShader.cso", Amp);

		context.context->set_mesh_shader(meshShader);
		context.context->set_pixel_shader(pixelShader);
		context.context->set_amp_shader(ampShader);
		context.context->set_input_primitive(TriangleList);
		context.context->set_primitive(Triangle);
		context.context->set_draw_extent({_mapWidth, _mapHeight});
		context.context->set_fill_mode(Solid);
		context.context->set_cull_mode(CullNone);
		context.context->set_depth_test_enabled(true);
		context.context->set_depth_clip(false);

		context.context->bind_render_target(0, nullptr);
		context.context->set_num_render_targets(0);

		for (auto& light : context.frameData.lights)
		{
			if (light.type != 1)
				continue;

			auto map = GetMapForLight(light, context);

			light.shadowIdx = static_cast<uint32_t>(context.frameData.shadowMaps.size());
			context.frameData.shadowMaps.push_back(map);

			auto info = GetViewProjForLight(light, context);

			light.shadowMapInfo = info;

			auto infoBuffer = context.resources->ToBuffer(info, false, true);
			context.context->bind_resource(4, infoBuffer);

			context.context->bind_depth_target(map);

			context.context->clear_depth(1.f, map.get());

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

				if(material->roughness.has_value() == false || material->metalness.has_value() == false)
					continue;

				{
					auto drawConstants = graphics::DrawConstants();
					drawConstants.localToWorld = r.worldMatrix;
					drawConstants.hasNormal = 0;
					drawConstants.meshletCount = mesh->numMeshlets;
					auto pConstants = r.constants->map();
					memcpy(pConstants, &drawConstants, sizeof(graphics::DrawConstants));
					r.constants->unmap();
				}

				context.context->bind_resource(10, texture->imageResource);
				context.context->bind_resource(0, mesh->meshletBuffer);
				context.context->bind_resource(1, mesh->vertexBuffer);
				context.context->bind_constant(0, r.constants);
				context.context->dispatch_mesh(mesh->numMeshlets, 1, 1);
			}
		}
	}

	std::shared_ptr<VoidImage> RenderShadowMapsNode::GetMapForLight(graphics::LightInfo& light, RenderContext& context)
	{
		ImageDescriptor desc = {};
		desc.depthWrite = true;
		desc.flags = DepthStencil;
		desc.format = R32_TYPELESS;
		desc.shaderRead = true;
		desc.size = {_mapWidth, _mapHeight, graphics::ShadowMapCascades};

		auto map = context.resources->Get(desc);

		return map;
	}

	graphics::ShadowMapInfo RenderShadowMapsNode::GetViewProjForLight(graphics::LightInfo& light,
	                                                                  RenderContext& context)
	{
		static float lambda = 0.89f;

		ImGui::Begin("Shadow settings");
		ImGui::DragFloat("Lambda", &lambda, 0.001f, 0.f, 1.f);
		ImGui::End();

		graphics::ShadowMapInfo out;
		std::vector<float> cascadeSplits(graphics::ShadowMapCascades);

		auto cam = context.world->active_camera;

		float nearClip = cam->near;
		float farClip = cam->far;
		float clipRange = farClip - nearClip;

		float minZ = nearClip;
		float maxZ = nearClip + clipRange;

		float range = maxZ - minZ;
		float ratio = maxZ / minZ;

		// Calculate split depths based on view camera frustum
		// Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
		for (uint32_t i = 0; i < graphics::ShadowMapCascades; i++)
		{
			float p = (static_cast<float>(i) + 1.f) / static_cast<float>(graphics::ShadowMapCascades);
			float log = minZ * std::pow(ratio, p);
			float uniform = minZ + range * p;
			float d = lambda * (log - uniform) + uniform;
			cascadeSplits[i] = (d - nearClip) / clipRange;
		}

		constexpr float camDistance = 1000.f;

		// Calculate orthographic projection matrix for each cascade
		float lastSplitDist = 0.0;
		for (uint32_t i = 0; i < graphics::ShadowMapCascades; i++)
		{
			float splitDist = cascadeSplits[i];

			glm::vec3 frustumCorners[8] = {
				glm::vec3(-1.0f, 1.0f, 0.0f),
				glm::vec3(1.0f, 1.0f, 0.0f),
				glm::vec3(1.0f, -1.0f, 0.0f),
				glm::vec3(-1.0f, -1.0f, 0.0f),
				glm::vec3(-1.0f, 1.0f, 1.0f),
				glm::vec3(1.0f, 1.0f, 1.0f),
				glm::vec3(1.0f, -1.0f, 1.0f),
				glm::vec3(-1.0f, -1.0f, 1.0f),
			};

			// Project frustum corners into world space
			glm::mat4 invCam = glm::inverse(cam->GetProjectionMatrix() * cam->GetViewMatrix());
			for (uint32_t j = 0; j < 8; j++)
			{
				glm::vec4 invCorner = invCam * glm::vec4(frustumCorners[j], 1.0f);
				frustumCorners[j] = invCorner / invCorner.w;
			}

			for (uint32_t j = 0; j < 4; j++)
			{
				glm::vec3 dist = frustumCorners[j + 4] - frustumCorners[j];
				frustumCorners[j + 4] = frustumCorners[j] + (dist * splitDist);
				frustumCorners[j] = frustumCorners[j] + (dist * lastSplitDist);
			}

			// Get frustum center
			glm::vec3 frustumCenter = glm::vec3(0.0f);
			for (uint32_t j = 0; j < 8; j++)
			{
				frustumCenter += frustumCorners[j];
			}
			frustumCenter /= 8.0f;

			float radius = 0.0f;
			for (uint32_t j = 0; j < 8; j++)
			{
				float distance = glm::length(frustumCorners[j] - frustumCenter);
				radius = glm::max(radius, distance);
			}
			radius = std::ceil(radius * 16.0f) / 16.0f;

			glm::vec3 maxExtents = glm::vec3(radius);
			glm::vec3 minExtents = -maxExtents;


			glm::vec3 lightDirN = glm::normalize(light.direction);
			auto eye = frustumCenter - glm::normalize(light.direction) * -minExtents.z;

			glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

			// If the light direction is close to being parallel with the Y-axis, change the up vector
			if (glm::abs(glm::dot(light.direction, up)) > 0.999f) {
				up = glm::vec3(1.0f, 0.0f, 0.0f); // Use the X-axis as the up vector instead
			}

			glm::mat4 lightViewMatrix = glm::lookAt(frustumCenter - glm::normalize(light.direction) * -minExtents.z, frustumCenter,
			                                        up);

			auto zNear = 0.1f;
			auto zFar = maxExtents.z - minExtents.z;

			constexpr float zMult = 3.0f;

			if (zNear < 0)
			{
				zNear *= zMult;
			}
			else
			{
				zNear /= zMult;
			}
			if (zFar < 0)
			{
				zFar /= zMult;
			}
			else
			{
				zFar *= zMult;
			}


			glm::mat4 lightOrthoMatrix =
				glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, zNear, zFar);

			//lightOrthoMatrix[1][1] *= -1;

			// Store split distance and matrix in cascade
			//cascades[i].splitDepth = (camera->near + splitDist * clipRange) * -1.0f;
			out.splits[i] = (cam->near + splitDist * clipRange); // *-1.0f;
			out.viewProj[i] = lightOrthoMatrix * lightViewMatrix;

			lastSplitDist = cascadeSplits[i];
		}

		return out;
	}
}
