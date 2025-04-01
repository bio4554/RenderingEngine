#include "pch.h"
#include "CullWorldNode.h"
#include "RenderProxy.h"
#include <tracy/Tracy.hpp>

#include "VoidFigure.h"
#include "VoidLight.h"

namespace star::renderer
{
	void CullWorldNode::Execute(RenderContext& context)
	{
		for(auto& object : context.world->root_object->children)
		{
			CullObject(context, object);
		}
	}

	void CullWorldNode::CullObject(RenderContext& context, const std::shared_ptr<game::Object>& object)
	{
		ZoneScopedN("Cull object");
		if(object->IsType("Figure"))
		{
			if (const auto figure = reinterpret_cast<game::Figure*>(object.get()); figure->meshHandle.has_value())
			{
				auto proxy = graphics::RenderProxy();
				proxy.meshHandle = figure->meshHandle.value();
				proxy.worldMatrix = figure->transform.GetMatrix();
				proxy.constants = context.constantsCache->GetBuffer(sizeof(graphics::DrawConstants));
				context.frameData.render_proxies.push_back(proxy);
			}
		}

		if(object->IsType("Light"))
		{
			const auto light = reinterpret_cast<game::Light*>(object.get());

			auto proxy = graphics::LightInfo();
			proxy.color = light->color;
			proxy.direction = light->direction;
			proxy.intensity = light->intensity;
			proxy.type = 1;

			context.frameData.lights.push_back(proxy);
		}

		for(const auto& child : object->children)
		{
			CullObject(context, child);
		}
	}

}
