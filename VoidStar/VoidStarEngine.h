#pragma once
#include "IVoidRenderContext.h"
#include "ModelConverter.h"
#include "Renderer.h"
#include "VoidWorld.h"

namespace star
{
	class VoidStarEngine
	{
	public:
		VoidStarEngine();

		void Init(VoidExtent2D windowExtent, SDL_Window* window);
		void Run();
		void Shutdown();

	private:
		void InitDebugWorld();
		void LoadImportedModel(asset_baker::ImportedModelData& model, float scale);
		void DrawImGui();

		IVoidRenderContext* _renderContext;
		std::unique_ptr<game::World> _world;
		star::renderer::Renderer* _renderer;
	};
}
