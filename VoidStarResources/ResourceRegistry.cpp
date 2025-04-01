#include "pch.h"
#include "ResourceRegistry.h"

#include <tracy/Tracy.hpp>

#include "JobSystem.h"

namespace star::resource
{
	ResourceRegistry* GResourceRegistry = nullptr;

	ResourceRegistry::ResourceRegistry(IVoidRenderContext* context)
	{
		_context = context;
	}

	MeshHandle ResourceRegistry::RegisterMesh(std::shared_ptr<graphics::Mesh> mesh)
	{
		std::unique_lock lock{ _meshRegisterLock };

		_meshes.push_back(mesh);
		_meshLoads.emplace_back();
		_meshLoaded.emplace_back();

		const auto index = _meshes.size();

		auto handle = MeshHandle(static_cast<uint32_t>(index) - 1);

		return handle;
	}

	TextureHandle ResourceRegistry::RegisterTexture(std::shared_ptr<graphics::Texture> texture)
	{
		std::unique_lock lock{ _textureRegisterLock };

		_textures.push_back(texture);
		_textureLoads.emplace_back();
		_textureLoaded.emplace_back();

		const auto index = _textures.size();

		auto handle = TextureHandle(static_cast<uint32_t>(index) - 1);

		return handle;
	}

	ModelHandle ResourceRegistry::RegisterModel(std::shared_ptr<graphics::Model> model)
	{
		std::unique_lock lock{ _modelRegisterLock };

		_models.push_back(model);
		_modelLoads.emplace_back();
		_modelLoaded.emplace_back();

		const auto index = _models.size();

		auto handle = ModelHandle(static_cast<uint32_t>(index) - 1);

		return handle;
	}

	MaterialHandle ResourceRegistry::RegisterMaterial(std::shared_ptr<graphics::Material> material)
	{
		std::unique_lock lock{ _materialRegisterLock };

		_materials.push_back(material);
		_materialLoads.emplace_back();
		_materialLoaded.emplace_back();

		const auto index = _materials.size();

		_materialLoaded[index - 1].store(true);

		auto handle = MaterialHandle(static_cast<uint32_t>(index) - 1);

		return handle;
	}


	std::shared_ptr<graphics::Texture> ResourceRegistry::TryGetTextureData(resource::TextureHandle handle)
	{
		if (_textureLoaded[handle.handle].get())
			return _textures[handle.handle];

		if (_textureLoads[handle.handle].get())
			return nullptr;

		// need to initiate a load
		{
			std::unique_lock lock{ _textureLoadLock };
			if (_textureLoads[handle.handle].get())
				return nullptr;

			_textureLoads[handle.handle].store(true);

			auto builder = core::JobBuilder();

			auto texture = _textures[handle.handle];

			builder.Dispatch("Texture load", [this, handle, texture]
				{
					ZoneScopedN("Texture load");
					auto image = LoadImage(texture, _context);
					texture->imageResource = image;
					_textureLoaded[handle.handle].store(true);
				});
		}

		return nullptr;
	}

	std::shared_ptr<graphics::Mesh> ResourceRegistry::TryGetMeshData(resource::MeshHandle handle)
	{
		if (_meshLoaded[handle.handle].get())
			return _meshes[handle.handle];

		if (_meshLoads[handle.handle].get())
			return nullptr;

		{
			std::unique_lock lock{ _meshLoadLock };
			if (_meshLoads[handle.handle].get())
				return nullptr;

			_meshLoads[handle.handle].store(true);

			auto builder = core::JobBuilder();

			auto mesh = _meshes[handle.handle];

			builder.Dispatch("Mesh load", [this, handle, mesh]
				{
					ZoneScopedN("Mesh load");
					auto packet = LoadMesh(mesh, _context);
					mesh->vertexBuffer = packet.vertexBuffer;
					mesh->meshletBuffer = packet.meshletBuffer;
					_meshLoaded[handle.handle].store(true);
				});
		}

		return nullptr;
	}

	std::shared_ptr<graphics::Model> ResourceRegistry::TryGetModelData(resource::ModelHandle handle)
	{
		return _models[handle.handle];
	}

	std::shared_ptr<graphics::Material> ResourceRegistry::TryGetMaterialData(resource::MaterialHandle handle)
	{
		return _materials[handle.handle];
	}

	std::shared_ptr<VoidImage> ResourceRegistry::LoadImage(std::shared_ptr<graphics::Texture> texture, IVoidRenderContext* context)
	{
		auto resource = context->create_image({ texture->width, texture->height, 1 }, VoidImageFormat::R8G8B8A8_UNORM);

		context->copy_image(texture->imageData.data(), resource.get());

		context->register_image_view(resource.get());

		return resource;
	}

	graphics::MeshRenderPacket ResourceRegistry::LoadMesh(std::shared_ptr<graphics::Mesh> mesh, IVoidRenderContext* context)
	{
		const size_t vertexBufferSize = mesh->vertices.size() * sizeof(graphics::Vertex);
		const size_t meshletBufferSize = mesh->meshlets.size() * sizeof(graphics::Meshlet);

		graphics::MeshRenderPacket packet = {};

		auto vertexBuffer = context->create_structured(vertexBufferSize);
		auto vertexStaging = context->create_structured(vertexBufferSize, true);
		memcpy(vertexStaging->map(), mesh->vertices.data(), vertexBufferSize);
		vertexStaging->unmap();
		context->copy_buffer(vertexStaging.get(), vertexBuffer.get());
		vertexStaging.reset();

		context->register_resource_view(vertexBuffer.get(), mesh->vertices.size(), sizeof(graphics::Vertex));

		auto meshletBuffer = context->create_structured(meshletBufferSize);
		auto meshletBufferStaging = context->create_structured(meshletBufferSize, true);
		memcpy(meshletBufferStaging->map(), mesh->meshlets.data(), meshletBufferSize);
		meshletBufferStaging->unmap();
		context->copy_buffer(meshletBufferStaging.get(), meshletBuffer.get());
		meshletBufferStaging.reset();

		context->register_resource_view(meshletBuffer.get(), mesh->meshlets.size(), sizeof(graphics::Meshlet));

		packet.vertexBuffer = vertexBuffer;
		packet.meshletBuffer = meshletBuffer;
		packet.numMeshlets = static_cast<uint32_t>(mesh->meshlets.size());

		return packet;
	}
}
