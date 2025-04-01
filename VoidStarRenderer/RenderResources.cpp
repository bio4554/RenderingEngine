#include "pch.h"
#include "RenderResources.h"

#include <utility>

namespace star::renderer
{
	template <>
	void RenderResources::Set(const std::string& name, std::shared_ptr<VoidImage> resource)
	{
		_images[name] = std::move(resource);
	}

	template<>
	void RenderResources::Set(const std::string& name, std::shared_ptr<VoidStructuredBuffer> resource)
	{
		_buffers[name] = std::move(resource);
	}

	template<>
	std::shared_ptr<VoidImage> RenderResources::Get(const std::string& name)
	{
		return _images[name];
	}

	template<>
	std::shared_ptr<VoidStructuredBuffer> RenderResources::Get(const std::string& name)
	{
		return _buffers[name];
	}

	std::shared_ptr<VoidImage> RenderResources::Get(const ImageDescriptor& desc)
	{
		auto hash = HashDescriptor(desc);

		{
			std::unique_lock lock{ _cacheLock };
			if(_imageCacheFreeList.contains(hash))
			{
				auto& queue = _imageCacheFreeList[hash];
				if(!queue.empty())
				{
					auto resource = queue.front();
					queue.pop();
					return _imageCache[hash][resource];
				}

				auto newResource = CreateImage(desc);
				_imageCache[hash].push_back(newResource);
				return newResource;
			}

			_imageCacheFreeList[hash] = std::queue<size_t>();
			_imageCache[hash] = std::vector<std::shared_ptr<VoidImage>>();
			auto newResource = CreateImage(desc);
			_imageCache[hash].push_back(newResource);
			return newResource;
		}
	}

	RenderResources::RenderResources(IVoidRenderContext* context)
	{
		_renderContext = context;
	}

	void RenderResources::FreeCache()
	{
		for(auto& p : _imageCacheFreeList)
		{
			p.second = std::queue<size_t>();

			auto& v = _imageCache[p.first];
			for(size_t i = 0; i < v.size(); i++)
			{
				p.second.push(i);
			}
		}
	}

	std::shared_ptr<VoidImage> RenderResources::CreateImage(const ImageDescriptor& desc)
	{
		auto image = _renderContext->create_image(desc.size, desc.format, desc.flags);
		
		if(desc.renderTarget)
		{
			_renderContext->register_render_target_view(image.get());
		}

		if(desc.depthWrite)
		{
			_renderContext->register_depth_stencil_view(image.get());
		}

		if(desc.shaderRead)
		{
			_renderContext->register_image_view(image.get());
		}

		return image;
	}
}