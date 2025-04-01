#include "pch.h"
#include "PrepareRenderNode.h"

#include "../imgui/imgui.h"

namespace star::renderer
{
	void PrepareRenderNode::Execute(RenderContext& context)
	{
		graphics::SceneConstants constants;

		auto viewMatrix = context.world->active_camera->GetViewMatrix();
		auto projMatrix = context.world->active_camera->GetProjectionMatrix();

		static float shadowBias = 0.001f;

		ImGui::Begin("Shadow bias");
		ImGui::DragFloat("Value", &shadowBias, 0.0001f, 0.f, 1.f);
		ImGui::End();

		constants.viewProjection = projMatrix * viewMatrix;
		constants.view = viewMatrix;
		constants.proj = projMatrix;
		constants.cameraPosition = context.world->active_camera->position;
		constants.renderSettings = context.renderSettings;
		constants.renderSettings.shadowBias = shadowBias;
		constants.lightCount = static_cast<uint32_t>(context.frameData.lights.size());

		//std::shared_ptr<VoidStructuredBuffer> constantsBuffer = context.resources->GetBuffer("sceneConstants");
		auto constantsBuffer = context.resources->ToBuffer(constants, false, true);

		context.resources->Set("sceneConstants", constantsBuffer);

		auto lightsBuffer = context.resources->ToBuffer(context.frameData.lights, false, true);

		context.resources->Set("lightsBuffer", lightsBuffer);
	}
}
