#pragma once
#include "JobSystem.h"
#include "VoidWorld.h"

namespace star::jobs
{
	class UpdateObjectsJob
	{
	public:
		static std::shared_ptr<core::SyncCounter> Execute(game::World* world, float delta);
	private:
		static void UpdateObject(std::shared_ptr<game::Object> object, core::JobBuilder& builder, float delta);
	};
}
