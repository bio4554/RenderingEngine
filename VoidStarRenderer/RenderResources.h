#pragma once
#include <queue>
#include <unordered_map>

#include "Camera.h"
#include "ConstantsCache.h"
#include "FrameRenderData.h"
#include "IVoidRenderContext.h"
#include "ShaderManager.h"
#include "VoidWorld.h"

namespace star::renderer
{
	struct ImageDescriptor
	{
		VoidExtent3D size;
		VoidImageFormat format;
		VoidResourceFlags flags;
		bool renderTarget = false;
		bool depthWrite = false;
		bool shaderRead = false;
	};

	class RenderResources
	{
	public:
		RenderResources(IVoidRenderContext* context);

		template<typename RefType>
		void Set(const std::string& name, std::shared_ptr<RefType> resource);

		template<typename RefType>
		std::shared_ptr<RefType> Get(const std::string& name);

		std::shared_ptr<VoidImage> Get(const ImageDescriptor& desc);

		void FreeCache();
		std::shared_ptr<VoidImage> CreateImage(const ImageDescriptor& desc);

		template<typename T>
		std::shared_ptr<VoidStructuredBuffer> ToBuffer(const T& data, bool constant, bool dynamic)
		{
			return ToBuffer(&data, 1, constant, dynamic);
		}

		template<typename T>
		std::shared_ptr<VoidStructuredBuffer> ToBuffer(const std::vector<T>& data, bool constant, bool dynamic)
		{
			return ToBuffer(data.data(), data.size(), constant, dynamic);
		}

		template<typename T>
		std::shared_ptr<VoidStructuredBuffer> ToBuffer(T* data, size_t count, bool constant, bool dynamic)
		{
			auto totalSize = sizeof(T) * count;

			if(dynamic)
			{
				auto buffer = _renderContext->create_structured(totalSize, true);
				if(constant)
				{
					_renderContext->register_constant_view(buffer.get());
				}
				else
				{
					_renderContext->register_resource_view(buffer.get(), count, sizeof(T));
				}
				
				auto pData = buffer->map();
				memcpy(pData, data, totalSize);
				buffer->unmap();

				return buffer;
			}
			else
			{
				auto buffer = _renderContext->create_structured(totalSize, false);
				if(constant)
				{
					_renderContext->register_constant_view(buffer.get());
				}
				else
				{
					_renderContext->register_resource_view(buffer.get(), count, sizeof(T));
				}
				
				auto stagingBuffer = _renderContext->create_structured(totalSize, true);
				auto pData = stagingBuffer->map();
				memcpy(pData, data, totalSize);
				stagingBuffer->unmap();

				_renderContext->copy_buffer(stagingBuffer.get(), buffer.get());

				return buffer;
			}
		}
	private:
		std::unordered_map<std::string, std::shared_ptr<VoidImage>> _images;
		std::unordered_map<std::string, std::shared_ptr<VoidStructuredBuffer>> _buffers;

		

		inline size_t HashDescriptor(const ImageDescriptor& desc) const
		{
			size_t seed = 0;

			seed ^= _uint32Hasher(desc.size.width) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			seed ^= _uint32Hasher(desc.size.height) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			seed ^= _uint32Hasher(desc.size.depth) + 0x9e3779b9 + (seed << 6) + (seed >> 2);

			seed ^= _formatHasher(desc.format) + 0x9e3779b9 + (seed << 6) + (seed >> 2);

			seed ^= _flagsHasher(desc.flags) + 0x9e3779b9 + (seed << 6) + (seed >> 2);

			seed ^= _boolHasher(desc.depthWrite) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			seed ^= _boolHasher(desc.shaderRead) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			seed ^= _boolHasher(desc.renderTarget) + 0x9e3779b9 + (seed << 6) + (seed >> 2);

			return seed;
		}

		IVoidRenderContext* _renderContext;
		std::hash<uint32_t> _uint32Hasher;
		std::hash<VoidImageFormat> _formatHasher;
		std::hash<VoidResourceFlags> _flagsHasher;
		std::hash<bool> _boolHasher;
		std::unordered_map<size_t, std::queue<size_t>> _imageCacheFreeList;
		std::unordered_map<size_t, std::vector<std::shared_ptr<VoidImage>>> _imageCache;
		std::mutex _cacheLock;
	};

	struct RenderContext
	{
		graphics::FrameRenderData frameData;
		RenderResources* resources;
		game::World* world;
		ShaderManager* shaders;
		VoidExtent2D drawExtent;
		IVoidRenderContext* context;
		ConstantsCache* constantsCache;
		graphics::RenderSettings renderSettings;
	};
}
