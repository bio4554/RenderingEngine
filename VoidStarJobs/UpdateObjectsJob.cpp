#include "pch.h"
#include "UpdateObjectsJob.h"
#include "VoidObject.h"

#include <tracy/Tracy.hpp>

namespace star::jobs
{
	std::shared_ptr<core::SyncCounter> UpdateObjectsJob::Execute(game::World* world, float delta)
	{
		auto builder = core::JobBuilder();
		{
			ZoneScopedN("Register object update jobs");

			for(auto& c : world->root_object->children)
			{
				UpdateObject(c, builder, delta);
			}

			builder.DispatchWait();
		}

		return builder.ExtractCounter();
	}

	void UpdateObjectsJob::UpdateObject(std::shared_ptr<game::Object> object, core::JobBuilder& builder, float delta)
	{
		if (object == nullptr)
			return;

		builder.DispatchNow("Update object", [object, &builder, delta]
			{
				ZoneScopedN("Update object");
				object->Update(delta);
			});

		for(auto o : object->children)
		{
			UpdateObject(o, builder, delta);
		}
	}
}
