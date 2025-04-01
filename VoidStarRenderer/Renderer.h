#pragma once
#include "ConstantsCache.h"
#include "VoidWorld.h"
#include "IVoidRenderContext.h"
#include "RenderResources.h"
#include "SyncCounter.h"


namespace star::renderer
{
	class Renderer
	{
	public:
		Renderer(IVoidRenderContext* context);

		void Init(VoidExtent2D screenExtent);
		void Render(game::World* world);

		graphics::RenderSettings renderSettings;
	private:
		struct RendererNodeState
		{
			bool doWireframe = false;
		};

		void InitResources(VoidExtent2D screenExtent);
		void ImGui();

		RenderResources _renderResources;
		IVoidRenderContext* _context;
		ShaderManager _shaderManager;
		VoidExtent2D _windowExtent;
		ConstantsCache _constantsCache;
		RendererNodeState _nodeState;
	};
}
