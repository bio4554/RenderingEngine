#pragma once
#include "RenderResources.h"

namespace star::renderer
{
	class RenderShadowMapsNode
	{
	public:
		void Execute(RenderContext& context);

	private:
		static constexpr uint32_t _mapWidth = 4096;
		static constexpr uint32_t _mapHeight = 4096;

		std::shared_ptr<VoidImage> GetMapForLight(graphics::LightInfo& light, RenderContext& context);
		void RenderSceneForMap(std::shared_ptr<VoidImage> map, uint32_t idx, RenderContext& context);
		graphics::ShadowMapInfo GetViewProjForLight(graphics::LightInfo& light, RenderContext& context);
	};
}
