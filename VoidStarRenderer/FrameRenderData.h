#pragma once
#include <vector>

#include "RenderProxy.h"
#include "VoidImage.h"

namespace star::graphics
{
	constexpr uint32_t ShadowMapCascades = 4;

	struct RenderSettings
	{
		int normalMapEnable;
		int meshletDebug;
		int enableLight;
		float shadowBias;
	};

	struct SceneConstants
	{
		glm::mat4 viewProjection;
		glm::mat4 view;
		glm::mat4 proj;
		glm::vec3 cameraPosition;
		uint32_t lightCount;
		RenderSettings renderSettings;
	};

	struct ShadowMapInfo
	{
		float splits[ShadowMapCascades];
		glm::mat4 viewProj[ShadowMapCascades];
	};

	struct LightInfo
	{
		glm::vec3 worldPosition;
		glm::vec3 direction;
		glm::vec3 color;
		float intensity;
		uint32_t type; // 1 = directional
		ShadowMapInfo shadowMapInfo;
		uint32_t shadowIdx;
	};

	struct FrameRenderData
	{
		std::vector<RenderProxy> render_proxies;
		std::vector<LightInfo> lights;
		std::vector<std::shared_ptr<VoidImage>> shadowMaps;
	};
}
