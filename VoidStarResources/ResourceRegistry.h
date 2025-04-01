#pragma once
#include <cstdint>
#include <mutex>

#include "AtomBool.h"
#include "IVoidRenderContext.h"
#include "JobSystem.h"
#include "Material.h"
#include "ResourceHandles.h"
#include "VoidMesh.h"
#include "VoidModel.h"

namespace star::resource
{
	class ResourceRegistry
	{
	public:
		ResourceRegistry(IVoidRenderContext* context);

		MeshHandle RegisterMesh(std::shared_ptr<graphics::Mesh> mesh);
		TextureHandle RegisterTexture(std::shared_ptr<graphics::Texture> texture);
		ModelHandle RegisterModel(std::shared_ptr<graphics::Model> model);
		MaterialHandle RegisterMaterial(std::shared_ptr<graphics::Material> material);

		std::shared_ptr<graphics::Texture> TryGetTextureData(resource::TextureHandle handle);
		std::shared_ptr<graphics::Mesh> TryGetMeshData(resource::MeshHandle handle);
		std::shared_ptr<graphics::Model> TryGetModelData(resource::ModelHandle handle);
		std::shared_ptr<graphics::Material> TryGetMaterialData(resource::MaterialHandle handle);

	private:
		static std::shared_ptr<VoidImage> LoadImage(std::shared_ptr<graphics::Texture> texture, IVoidRenderContext* context);
		static graphics::MeshRenderPacket LoadMesh(std::shared_ptr<graphics::Mesh> mesh, IVoidRenderContext* context);

		std::vector<std::shared_ptr<graphics::Texture>> _textures;
		std::vector<std::shared_ptr<graphics::Mesh>> _meshes;
		std::vector<std::shared_ptr<graphics::Model>> _models;
		std::vector<std::shared_ptr<graphics::Material>> _materials;

		std::vector<core::AtomBool> _textureLoads;
		std::vector<core::AtomBool> _meshLoads;
		std::vector<core::AtomBool> _modelLoads;
		std::vector<core::AtomBool> _materialLoads;

		std::vector<core::AtomBool> _textureLoaded;
		std::vector<core::AtomBool> _meshLoaded;
		std::vector<core::AtomBool> _modelLoaded;
		std::vector<core::AtomBool> _materialLoaded;

		std::mutex _textureLoadLock;
		std::mutex _meshLoadLock;
		std::mutex _modelLoadLock;
		std::mutex _materialLoadLock;

		std::mutex _textureRegisterLock;
		std::mutex _meshRegisterLock;
		std::mutex _modelRegisterLock;
		std::mutex _materialRegisterLock;

		IVoidRenderContext* _context;
	};

	extern ResourceRegistry* GResourceRegistry;
}
