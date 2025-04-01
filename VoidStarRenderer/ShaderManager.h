#pragma once
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "IVoidRenderContext.h"
#include "VoidShader.h"

namespace star::renderer 
{
	class ShaderManager 
	{
	public:
		ShaderManager(IVoidRenderContext* context);

		std::shared_ptr<VoidShader> GetShader(const std::string& name, VoidShaderStage stage);
	private:
		std::mutex _lock;
		std::unordered_map<std::string, std::shared_ptr<VoidShader>> _shaders;
		IVoidRenderContext* _context;
	};
}
