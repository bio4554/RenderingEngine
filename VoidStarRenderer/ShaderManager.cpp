#include "pch.h"
#include "ShaderManager.h"

namespace star::renderer
{
	ShaderManager::ShaderManager(IVoidRenderContext* context)
	{
		_context = context;
	}

	std::shared_ptr<VoidShader> ShaderManager::GetShader(const std::string& name, VoidShaderStage stage)
	{
		std::unique_lock lock{ _lock };

		auto shader = _shaders.find(name);
		if (shader != _shaders.end())
		{
			return shader->second;
		}

		auto created = _context->create_shader(name, stage);

		_shaders[name] = created;

		return created;
	}

}
