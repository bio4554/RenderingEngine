#pragma once
#include <unordered_map>

#include "VoidGraphicsPrimitives.h"

namespace star::graphics
{
	struct Meshlet;
}

namespace asset_baker
{
	struct ImportedTextureData
	{
		std::vector<uint8_t> data;
		size_t size;
		uint32_t width;
		uint32_t height;
	};

	struct ImportedMaterialData
	{
		ImportedTextureData albedo = {};
		ImportedTextureData normal = {};
		ImportedTextureData metalness = {};
		ImportedTextureData roughness = {};
		ImportedTextureData alpha = {};
		bool hasNormal = false;
		bool hasMetalness = false;
		bool hasRoughness = false;
		bool hasAlpha = false;

		/*template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar& data;
			ar& size;
			ar& width;
			ar& height;
		}*/
	};

	struct ImportedMeshletData
	{
		uint32_t vertexCount;
		uint32_t triangleCount;
		std::vector<uint32_t> vertexIndices;
		std::vector<uint32_t> primitiveIndices;
	};

	struct ImportedMeshData
	{
		std::vector<star::graphics::Vertex> vertices;
		std::vector<uint32_t> indices;
		std::string materialKey;
		bool hasMaterial;
		std::vector<star::graphics::Vertex> boundingBox;
		std::vector<star::graphics::Meshlet> meshlets;

		/*template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar& vertices;
			ar& indices;
			ar& materialKey;
			ar& hasMaterial;
			ar& boundingBox;
		}*/
	};

	struct ImportedModelData
	{
		std::vector<ImportedMeshData> meshes;
		std::unordered_map<std::string, ImportedMaterialData> materials;

		/*template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar& meshes;
			ar& materials;
		}*/
	};

	class ModelConverter
	{
	public:
		static ImportedModelData ImportModel(std::string path);
		static ImportedModelData ImportScene(std::string path);
	};
}
