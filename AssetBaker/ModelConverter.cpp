#include "pch.h"
#include "ModelConverter.h"

#include <filesystem>
#include <iostream>
#include <glm/gtx/quaternion.hpp>

#include "stb_image.h"
#include "../VoidStarCore/JobSystem.h"

namespace asset_baker
{
	void ProcessSceneNode(aiNode* node, ImportedModelData& outModel, ImportedModelData& model, glm::mat4 parentTransform);

	ImportedModelData ModelConverter::ImportModel(std::string path)
	{
		auto importer = Assimp::Importer();

		ImportedModelData out;

		importer.SetPropertyBool("AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS", true);
		auto model = importer.ReadFile(path, aiProcess_FlipWindingOrder | aiProcess_FlipUVs | aiProcess_CalcTangentSpace | aiProcess_Triangulate);

		auto modelFolder = std::filesystem::path(path).remove_filename();

		for (unsigned int i = 0; i < model->mNumMaterials; i++)
		{
			auto mat = model->mMaterials[i];

			if (mat != nullptr)
			{
				aiString textureName, normalMapName;
				mat->Get(AI_MATKEY_TEXTURE_DIFFUSE(0), textureName);
				if (mat->Get(AI_MATKEY_TEXTURE_DISPLACEMENT(0), normalMapName) == aiReturn_SUCCESS)
				{
					auto b = 2;
				}

				auto texturePath = std::filesystem::path(modelFolder).append(textureName.C_Str()).string();

				int texWidth, texHeight, texChannels;
				auto pixels = stbi_load(texturePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

				size_t imageSize = texWidth * texHeight * 4;

				if (!pixels) {
					continue;
				}

				auto material = std::vector<uint8_t>();

				for (uint64_t j = 0; j < imageSize; j++)
				{
					material.push_back(pixels[j]);
				}

				ImportedMaterialData matData;
				matData.albedo.data = material;
				matData.albedo.size = imageSize;
				matData.albedo.width = (uint32_t)texWidth;
				matData.albedo.height = (uint32_t)texHeight;
				matData.hasNormal = false;

				auto normalTexturePath = std::filesystem::path(modelFolder).append(normalMapName.C_Str()).string();

				int nTexWidth, nTexHeight, nTexChannels;
				auto nPixels = stbi_load(normalTexturePath.c_str(), &nTexWidth, &nTexHeight, &nTexChannels, STBI_rgb_alpha);

				size_t nImageSize = nTexWidth * nTexHeight * 4;

				if(nPixels)
				{
					matData.hasNormal = true;

					auto nMaterial = std::vector<uint8_t>();

					for(uint64_t j = 0; j < nImageSize; j++)
					{
						nMaterial.push_back(nPixels[j]);
					}

					matData.normal.data = nMaterial;
					matData.normal.size = nImageSize;
					matData.normal.width = nTexWidth;
					matData.normal.height = nTexHeight;
				}

				out.materials[std::string(textureName.C_Str())] = matData;

				std::cout << "Imported material\n";
			}
		}

		if(out.materials.empty())
		{
			auto defaultTexturePath = "TODO";

			int texWidth, texHeight, texChannels;
			auto pixels = stbi_load(defaultTexturePath, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

			size_t imageSize = texWidth * texHeight * 4;

			if (!pixels) {
				throw std::runtime_error("failed to load default texture");
			}

			auto material = std::vector<uint8_t>();

			for (uint64_t j = 0; j < imageSize; j++)
			{
				material.push_back(pixels[j]);
			}

			ImportedMaterialData matData;
			matData.albedo.data = material;
			matData.albedo.size = imageSize;
			matData.albedo.width = (uint32_t)texWidth;
			matData.albedo.height = (uint32_t)texHeight;
			matData.hasNormal = false;

			out.materials[""] = matData;
		}

		for (unsigned int i = 0; i < model->mNumMeshes; i++)
		{
			out.meshes.emplace_back();

			auto mesh = model->mMeshes[i];

			bool flip = false;

			auto numColorChannels = mesh->GetNumColorChannels();
			auto numUVs = mesh->GetNumUVChannels();

			float minX = std::numeric_limits<float>::max();
			float maxX = std::numeric_limits<float>::min();
			float minY = std::numeric_limits<float>::max();
			float maxY = std::numeric_limits<float>::min();
			float minZ = std::numeric_limits<float>::max();
			float maxZ = std::numeric_limits<float>::min();

			// load verts/indices
			for (unsigned int v = 0; v < mesh->mNumVertices; v++)
			{
				const auto& mVertex = mesh->mVertices[v];
				star::graphics::Vertex vertex{};

				vertex.position.x = mVertex.x;
				vertex.position.y = mVertex.y;
				vertex.position.z = mVertex.z;

				if (vertex.position.x > maxX)
					maxX = vertex.position.x;
				if (vertex.position.x < minX)
					minX = vertex.position.x;
				if (vertex.position.y > maxY)
					maxY = vertex.position.y;
				if (vertex.position.y < minY)
					minY = vertex.position.y;
				if (vertex.position.z > maxZ)
					maxZ = vertex.position.z;
				if (vertex.position.z < minZ)
					minZ = vertex.position.z;

				if (numColorChannels > 0)
				{
					// todo handle multiple channels
					numColorChannels = 1;

					for (unsigned int ci = 0; ci < numColorChannels; ci++)
					{
						const auto& color = mesh->mColors[ci][v];
						vertex.color.r = color.r;
						vertex.color.g = color.g;
						vertex.color.b = color.b;
						vertex.color.a = color.a;
					}
				}
				else
				{
					vertex.color = flip ? glm::vec4{ 1.0f, 1.0f, 1.0f, 1.0f } : glm::vec4{ 0.0f, 0.0f, 0.0f, 0.0f };
					flip = !flip;
				}

				if (numUVs > 0)
				{
					// todo handle multiple channels
					numUVs = 1;

					for (unsigned int uvi = 0; uvi < numUVs; uvi++)
					{
						const auto& uv = mesh->mTextureCoords[uvi][v];
						vertex.uv.x = uv.x;
						vertex.uv.y = uv.y;
					}
				}

				if (mesh->HasNormals())
				{
					const auto& normal = mesh->mNormals[v];
					vertex.normal.x = normal.x;
					vertex.normal.y = normal.y;
					vertex.normal.z = normal.z;

					if(mesh->HasTangentsAndBitangents())
					{
						const auto& tangent = mesh->mTangents[v];
						const auto& biTangent = mesh->mBitangents[v];
						vertex.tangent = { tangent.x, tangent.y, tangent.z };
						vertex.biTangent = { biTangent.x, biTangent.y, biTangent.z };
					}
				}

				out.meshes[i].vertices.push_back(vertex);
			}

			// calc bounding box
			out.meshes[i].boundingBox = {
				star::graphics::Vertex{minX, minY, minZ, 1.f, 1.f, 1.f},
				star::graphics::Vertex{minX, minY, maxZ, 1.f, 1.f, 1.f},
				star::graphics::Vertex{minX, maxY, minZ, 1.f, 1.f, 1.f},
				star::graphics::Vertex{minX, maxY, maxZ, 1.f, 1.f, 1.f},
				star::graphics::Vertex{maxX, minY, minZ, 1.f, 1.f, 1.f},
				star::graphics::Vertex{maxX, minY, maxZ, 1.f, 1.f, 1.f},
				star::graphics::Vertex{maxX, maxY, minZ, 1.f, 1.f, 1.f},
				star::graphics::Vertex{maxX, maxY, maxZ, 1.f, 1.f, 1.f},
			};

			for (unsigned int f = 0; f < mesh->mNumFaces; f++)
			{
				const auto& face = mesh->mFaces[f];

				for (unsigned int ii = 0; ii < face.mNumIndices; ii++)
				{
					out.meshes[i].indices.push_back(face.mIndices[ii]);
				}
			}

			// load materials
			auto material = model->mMaterials[mesh->mMaterialIndex];

			if (material != nullptr)
			{
				aiString textureName;
				material->Get(AI_MATKEY_TEXTURE_DIFFUSE(0), textureName);

				if (out.materials.contains(std::string(textureName.C_Str())))
				{
					out.meshes[i].materialKey = std::string(textureName.C_Str());
					out.meshes[i].hasMaterial = true;
				}
				else
				{
					out.meshes[i].hasMaterial = false;
				}
			}
			else
			{
				out.meshes[i].hasMaterial = false;
			}

			assert(out.meshes[i].indices.size() % 3 == 0);

			std::cout << "imported mesh\n";
		}

		importer.FreeScene();

		return out;
	}

	bool LoadTexture(std::filesystem::path modelFolder, aiMaterial* mat, aiTextureType type, std::string& name, std::vector<uint8_t>& out, uint32_t& width, uint32_t& height)
	{
		aiString textureName;
		if (mat->GetTexture(type, 0, &textureName) != aiReturn_SUCCESS)
			return false;

		auto texturePath = std::filesystem::path(modelFolder).append(textureName.C_Str()).string();

		int texWidth, texHeight, texChannels;
		auto pixels = stbi_load(texturePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

		size_t imageSize = texWidth * texHeight * 4;

		if (!pixels) {
			return false;
		}

		out.clear();
		out.resize(imageSize);

		for (uint64_t j = 0; j < imageSize; j++)
		{
			out[j] = pixels[j];
		}

		width = static_cast<uint32_t>(texWidth);
		height = static_cast<uint32_t>(texHeight);

		name = std::string(textureName.C_Str());

		return true;
	}

	ImportedModelData ModelConverter::ImportScene(std::string path)
	{
		auto importer = Assimp::Importer();

		ImportedModelData out;

		importer.SetPropertyBool("AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS", true);
		auto model = importer.ReadFile(path, aiProcess_FlipWindingOrder | aiProcess_FlipUVs | aiProcess_CalcTangentSpace | aiProcess_Triangulate);

		auto modelFolder = std::filesystem::path(path).remove_filename();

		//{
		//	std::mutex mutex;
		//	auto jobBuilder = star::core::JobBuilder();
		//	for(uint32_t threadId = 0; threadId < model->mNumMaterials; threadId++)
		//	{
		//		auto mat = model->mMaterials[threadId];
		//		if(mat != nullptr)
		//		{
		//			jobBuilder.DispatchNow("texture load", [mat, modelFolder, &out, &mutex]()
		//				{
		//					aiString textureName, normalMapName;
		//					mat->Get(AI_MATKEY_TEXTURE_DIFFUSE(0), textureName);
		//					if (mat->Get(AI_MATKEY_TEXTURE_DISPLACEMENT(0), normalMapName) != aiReturn_SUCCESS)
		//					{
		//						normalMapName = aiString();
		//						mat->GetTexture(aiTextureType_NORMAL_CAMERA, 0, &normalMapName);
		//					}

		//					std::vector<aiMaterialProperty*> props;
		//					{
		//						for (int pi = 0; pi < mat->mNumProperties; pi++)
		//						{
		//							auto prop = mat->mProperties[pi];
		//							props.push_back(prop);
		//						}
		//					}

		//					auto texturePath = std::filesystem::path(modelFolder).append(textureName.C_Str()).string();

		//					int texWidth, texHeight, texChannels;
		//					auto pixels = stbi_load(texturePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

		//					size_t imageSize = texWidth * texHeight * 4;

		//					if (!pixels) {
		//						return;
		//					}

		//					auto material = std::vector<uint8_t>();

		//					for (uint64_t j = 0; j < imageSize; j++)
		//					{
		//						material.push_back(pixels[j]);
		//					}

		//					ImportedMaterialData matData;
		//					matData.texture.data = material;
		//					matData.texture.size = imageSize;
		//					matData.texture.width = (uint32_t)texWidth;
		//					matData.texture.height = (uint32_t)texHeight;
		//					matData.hasNormal = false;

		//					auto normalTexturePath = std::filesystem::path(modelFolder).append(normalMapName.C_Str()).string();

		//					int nTexWidth, nTexHeight, nTexChannels;
		//					auto nPixels = stbi_load(normalTexturePath.c_str(), &nTexWidth, &nTexHeight, &nTexChannels, STBI_rgb_alpha);

		//					size_t nImageSize = nTexWidth * nTexHeight * 4;

		//					if (nPixels)
		//					{
		//						matData.hasNormal = true;

		//						auto nMaterial = std::vector<uint8_t>();

		//						for (uint64_t j = 0; j < nImageSize; j++)
		//						{
		//							nMaterial.push_back(nPixels[j]);
		//						}

		//						matData.normal.data = nMaterial;
		//						matData.normal.size = nImageSize;
		//						matData.normal.width = nTexWidth;
		//						matData.normal.height = nTexHeight;
		//					}

		//					{
		//						std::unique_lock lock{ mutex };
		//						auto key = std::string(textureName.C_Str());
		//						if(out.materials.contains(key))
		//						{
		//							std::cout << std::format("Duplicate texture key: {}\n", key);
		//						}
		//						else 
		//						{
		//							out.materials[key] = matData;
		//						}
		//					}

		//					std::cout << "Imported material\n";
		//				});
		//		}
		//	}

		//	jobBuilder.DispatchWait();

		//	jobBuilder.WaitAll();
		//}

		for (unsigned int i = 0; i < model->mNumMaterials; i++)
		{
			//continue;
			auto mat = model->mMaterials[i];

			if (mat != nullptr)
			{
				std::string texName, dead;
				ImportedMaterialData matData;
				if(!LoadTexture(modelFolder, mat, aiTextureType_BASE_COLOR, texName, matData.albedo.data, matData.albedo.width, matData.albedo.height))
				{
					if(!LoadTexture(modelFolder, mat, aiTextureType_DIFFUSE, texName, matData.albedo.data, matData.albedo.width, matData.albedo.height))
						continue;
				}

				matData.hasNormal = LoadTexture(modelFolder, mat, aiTextureType_NORMAL_CAMERA, dead, matData.normal.data, matData.normal.width, matData.normal.height);
				if(!matData.hasNormal)
				{
					matData.hasNormal = LoadTexture(modelFolder, mat, aiTextureType_DISPLACEMENT, dead, matData.normal.data, matData.normal.width, matData.normal.height);
					if(!matData.hasNormal)
					{
						matData.hasNormal = LoadTexture(modelFolder, mat, aiTextureType_NORMALS, dead, matData.normal.data, matData.normal.width, matData.normal.height);
						if(!matData.hasNormal)
						{
							matData.hasNormal = LoadTexture(modelFolder, mat, aiTextureType_HEIGHT, dead, matData.normal.data, matData.normal.width, matData.normal.height);
						}
					}
				}
				matData.hasMetalness = LoadTexture(modelFolder, mat, aiTextureType_METALNESS, dead, matData.metalness.data, matData.metalness.width, matData.metalness.height);
				matData.hasRoughness = LoadTexture(modelFolder, mat, aiTextureType_DIFFUSE_ROUGHNESS, dead, matData.roughness.data, matData.roughness.width, matData.roughness.height);
				//matData.hasAlpha = LoadTexture(modelFolder, mat, aiTextureType_OPACITY, dead, matData.alpha.data, matData.alpha.width, matData.alpha.height);

				out.materials[texName] = matData;

				std::cout << "Imported material\n";
			}
		}

		if (out.materials.empty())
		{
			auto defaultTexturePath = "TODO";

			int texWidth, texHeight, texChannels;
			auto pixels = stbi_load(defaultTexturePath, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

			size_t imageSize = texWidth * texHeight * 4;

			if (!pixels) {
				throw std::runtime_error("failed to load default texture");
			}

			auto material = std::vector<uint8_t>();

			for (uint64_t j = 0; j < imageSize; j++)
			{
				material.push_back(pixels[j]);
			}

			ImportedMaterialData matData;
			matData.albedo.data = material;
			matData.albedo.size = imageSize;
			matData.albedo.width = (uint32_t)texWidth;
			matData.albedo.height = (uint32_t)texHeight;
			matData.hasNormal = false;

			out.materials[""] = matData;
		}

		for (unsigned int i = 0; i < model->mNumMeshes; i++)
		{
			out.meshes.emplace_back();

			auto mesh = model->mMeshes[i];

			bool flip = false;

			auto numColorChannels = mesh->GetNumColorChannels();
			auto numUVs = mesh->GetNumUVChannels();

			float minX = std::numeric_limits<float>::max();
			float maxX = std::numeric_limits<float>::min();
			float minY = std::numeric_limits<float>::max();
			float maxY = std::numeric_limits<float>::min();
			float minZ = std::numeric_limits<float>::max();
			float maxZ = std::numeric_limits<float>::min();

			// load verts/indices
			for (unsigned int v = 0; v < mesh->mNumVertices; v++)
			{
				const auto& mVertex = mesh->mVertices[v];
				star::graphics::Vertex vertex{};

				vertex.position.x = mVertex.x;
				vertex.position.y = mVertex.y;
				vertex.position.z = mVertex.z;

				if (vertex.position.x > maxX)
					maxX = vertex.position.x;
				if (vertex.position.x < minX)
					minX = vertex.position.x;
				if (vertex.position.y > maxY)
					maxY = vertex.position.y;
				if (vertex.position.y < minY)
					minY = vertex.position.y;
				if (vertex.position.z > maxZ)
					maxZ = vertex.position.z;
				if (vertex.position.z < minZ)
					minZ = vertex.position.z;

				if (numColorChannels > 0)
				{
					// todo handle multiple channels
					numColorChannels = 1;

					for (unsigned int ci = 0; ci < numColorChannels; ci++)
					{
						const auto& color = mesh->mColors[ci][v];
						vertex.color.r = color.r;
						vertex.color.g = color.g;
						vertex.color.b = color.b;
						vertex.color.a = color.a;
					}
				}
				else
				{
					vertex.color = flip ? glm::vec4{ 1.0f, 1.0f, 1.0f, 1.0f } : glm::vec4{ 0.0f, 0.0f, 0.0f, 0.0f };
					flip = !flip;
				}

				if (numUVs > 0)
				{
					// todo handle multiple channels
					numUVs = 1;

					for (unsigned int uvi = 0; uvi < numUVs; uvi++)
					{
						const auto& uv = mesh->mTextureCoords[uvi][v];
						vertex.uv.x = uv.x;
						vertex.uv.y = uv.y;
					}
				}

				if (mesh->HasNormals())
				{
					const auto& normal = mesh->mNormals[v];
					vertex.normal.x = normal.x;
					vertex.normal.y = normal.y;
					vertex.normal.z = normal.z;

					if (mesh->HasTangentsAndBitangents())
					{
						const auto& tangent = mesh->mTangents[v];
						const auto& biTangent = mesh->mBitangents[v];
						vertex.tangent = { tangent.x, tangent.y, tangent.z };
						vertex.biTangent = { biTangent.x, biTangent.y, biTangent.z };
					}
				}

				out.meshes[i].vertices.push_back(vertex);
			}

			// calc bounding box
			out.meshes[i].boundingBox = {
				star::graphics::Vertex{minX, minY, minZ, 1.f, 1.f, 1.f},
				star::graphics::Vertex{minX, minY, maxZ, 1.f, 1.f, 1.f},
				star::graphics::Vertex{minX, maxY, minZ, 1.f, 1.f, 1.f},
				star::graphics::Vertex{minX, maxY, maxZ, 1.f, 1.f, 1.f},
				star::graphics::Vertex{maxX, minY, minZ, 1.f, 1.f, 1.f},
				star::graphics::Vertex{maxX, minY, maxZ, 1.f, 1.f, 1.f},
				star::graphics::Vertex{maxX, maxY, minZ, 1.f, 1.f, 1.f},
				star::graphics::Vertex{maxX, maxY, maxZ, 1.f, 1.f, 1.f},
			};

			for (unsigned int f = 0; f < mesh->mNumFaces; f++)
			{
				const auto& face = mesh->mFaces[f];

				for (unsigned int ii = 0; ii < face.mNumIndices; ii++)
				{
					out.meshes[i].indices.push_back(face.mIndices[ii]);
				}
			}

			// load materials
			auto material = model->mMaterials[mesh->mMaterialIndex];

			if (material != nullptr)
			{
				aiString textureName;
				material->Get(AI_MATKEY_TEXTURE_DIFFUSE(0), textureName);

				if (out.materials.contains(std::string(textureName.C_Str())))
				{
					out.meshes[i].materialKey = std::string(textureName.C_Str());
					out.meshes[i].hasMaterial = true;
				}
				else
				{
					out.meshes[i].hasMaterial = false;
				}
			}
			else
			{
				out.meshes[i].hasMaterial = false;
			}

			assert(out.meshes[i].indices.size() % 3 == 0);

			std::cout << "imported mesh\n";
		}

		// process scene hierarchy
		auto root = model->mRootNode;
		auto sceneOut = ImportedModelData();
		sceneOut.materials = out.materials;

		ProcessSceneNode(root, sceneOut, out, glm::identity<glm::mat4>());

		importer.FreeScene();

		return sceneOut;
	}

	glm::mat4 ToGlm(aiMatrix4x4 aiMat)
	{
		return glm::mat4(
			aiMat.a1, aiMat.b1, aiMat.c1, aiMat.d1, // First column
			aiMat.a2, aiMat.b2, aiMat.c2, aiMat.d2, // Second column
			aiMat.a3, aiMat.b3, aiMat.c3, aiMat.d3, // Third column
			aiMat.a4, aiMat.b4, aiMat.c4, aiMat.d4  // Fourth column
		);
	}

	void ProcessSceneNode(aiNode* node, ImportedModelData& outModel, ImportedModelData& model, glm::mat4 parentTransform)
	{
		auto transform = ToGlm(node->mTransformation);
		auto finalTransform = parentTransform * transform;

		for(uint32_t i = 0; i < node->mNumMeshes; i++)
		{
			auto meshIdx = node->mMeshes[i];
			auto mesh = model.meshes[meshIdx];
			for(auto& v : mesh.vertices)
			{
				v.position = finalTransform * glm::vec4(v.position, 1.f);
				v.normal = glm::normalize(finalTransform * glm::vec4(v.normal, 0.f));
			}

			outModel.meshes.push_back(mesh);
		}

		for(uint32_t i = 0; i < node->mNumChildren; i++)
		{
			auto child = node->mChildren[i];
			ProcessSceneNode(child, outModel, model, finalTransform);
		}
	}
}
