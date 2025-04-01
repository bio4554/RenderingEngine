#include "pch.h"
#include "Renderer.h"

#include <tracy/Tracy.hpp>

#include "BindShadowMapsNode.h"
#include "BindSharedResourcesNode.h"
#include "BlitToBackBufferNode.h"
#include "ClearDepthNode.h"
#include "ClearTargetNode.h"
#include "CullWorldNode.h"
#include "ForwardDrawSceneNode.h"
#include "ForwardDrawSceneWireNode.h"
#include "FrameRenderData.h"
#include "JobSystem.h"
#include "PrepareRenderNode.h"
#include "RenderShadowMapsNode.h"
#include "../imgui/imgui.h"

namespace star::renderer
{
	Renderer::Renderer(IVoidRenderContext* context) : renderSettings(), _renderResources(context), _context(context),
	                                                  _shaderManager(context), _constantsCache(context)
	{
	}

	void Renderer::Init(VoidExtent2D screenExtent)
	{
		_windowExtent = screenExtent;
		InitResources(screenExtent);
	}


	void Renderer::Render(game::World* world)
	{
		ImGui();

		_constantsCache.Reset();
		_renderResources.FreeCache();

		auto renderContext = RenderContext();
		renderContext.frameData = graphics::FrameRenderData();
		renderContext.resources = &_renderResources;
		renderContext.world = world;
		renderContext.shaders = &_shaderManager;
		renderContext.drawExtent = _windowExtent;
		renderContext.context = _context;
		renderContext.constantsCache = &_constantsCache;
		renderContext.renderSettings = renderSettings;

		{
			ZoneScopedN("Cull world");
			auto cullNode = CullWorldNode();
			cullNode.Execute(renderContext);
		}

		{
			ZoneScopedN("Prepare render");
			auto prepareNode = PrepareRenderNode();
			prepareNode.Execute(renderContext);
		}

		{
			ZoneScopedN("Shadow maps");
			auto shadowNode = RenderShadowMapsNode();
			shadowNode.Execute(renderContext);
		}

		{
			ZoneScopedN("Bind resources");
			auto bindResources = BindSharedResourcesNode();
			bindResources.Execute(renderContext);
		}

		{
			ZoneScopedN("Bind shadow maps");
			auto bindShadows = BindShadowMapsNode();
			bindShadows.Execute(renderContext);
		}

		{
			ZoneScopedN("Clear depth");
			auto clearDepth = ClearDepthNode();
			clearDepth.Execute(renderContext);
		}

		{
			ZoneScopedN("Clear target");
			auto clearTarget = ClearTargetNode();
			clearTarget.Execute(renderContext, "mainDrawBuffer");
		}

		if(_nodeState.doWireframe)
		{
			ZoneScopedN("Forward draw wireframe");
			auto forwardDraw = ForwardDrawSceneWireNode();
			forwardDraw.Execute(renderContext);
		}
		else
		{
			ZoneScopedN("Forward draw");
			auto forwardDraw = ForwardDrawSceneNode();
			forwardDraw.Execute(renderContext);
		}

		{
			ZoneScopedN("Blit to back buffer");
			auto blit = BlitToBackBufferNode();
			blit.Execute(renderContext);
		}

		_context->present();
	}

	void Renderer::InitResources(VoidExtent2D screenExtent)
	{
		VoidExtent3D extent = {
			.width = screenExtent.width,
			.height = screenExtent.height,
			.depth = 1
		};

		{
			auto depthBuffer = _context->create_image(extent, D32_FLOAT, DepthStencil);
			_context->register_depth_stencil_view(depthBuffer.get());
			_renderResources.Set<VoidImage>("mainDepthBuffer", depthBuffer);
		}

		{
			auto drawBuffer = _context->create_image(extent, R16G16B16A16_FLOAT, RenderTarget);
			_context->register_render_target_view(drawBuffer.get());
			_context->register_image_view(drawBuffer.get());
			_renderResources.Set<VoidImage>("mainDrawBuffer", drawBuffer);
		}

		{
			auto sceneConstants = _context->create_structured(sizeof(graphics::SceneConstants), true);
			_context->register_resource_view(sceneConstants.get());
			_renderResources.Set<VoidStructuredBuffer>("sceneConstants", sceneConstants);
		}
	}

	void Renderer::ImGui()
	{
		ImGui::Begin("Renderer Node States");
		ImGui::Checkbox("Wireframe", &_nodeState.doWireframe);
		ImGui::End();
	}
}
