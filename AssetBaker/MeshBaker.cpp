#include "pch.h"
#include "MeshBaker.h"

#include <iostream>
#include <jemalloc/jemalloc.h>

#include "../VoidStarCore/JobSystem.h"

namespace asset_baker
{
	MeshBaker::MeshBaker(ImportedModelData* modelData)
	{
		_pModelData = modelData;
	}

	void MeshBaker::Bake()
	{
		meshopt_setAllocator(malloc, free);

		auto builder = star::core::JobBuilder();

		for(auto& mesh : _pModelData->meshes)
		{
			builder.DispatchNow("optimize mesh", [this, &mesh]()
				{
					OptimizeMesh(mesh);
				});
		}

		builder.DispatchWait();
		builder.WaitAll();
	}

	void MeshBaker::OptimizeMesh(ImportedMeshData& meshData)
	{
		// step 1: indexing
		size_t indexCount = meshData.indices.size();
		std::vector<uint32_t> remap(indexCount);
		auto vertexCount = meshopt_generateVertexRemap(remap.data(), meshData.indices.data(), indexCount, meshData.vertices.data(), meshData.vertices.size(), sizeof(star::graphics::Vertex));

		std::vector<uint32_t> indices(indexCount);
		std::vector<star::graphics::Vertex> vertices(vertexCount);
		
		meshopt_remapIndexBuffer(indices.data(), meshData.indices.data(), indexCount, remap.data());
		meshopt_remapVertexBuffer(vertices.data(), meshData.vertices.data(), meshData.vertices.size(), sizeof(star::graphics::Vertex), remap.data());

		meshopt_optimizeVertexCache(indices.data(), indices.data(), indexCount, vertexCount);

		meshopt_optimizeOverdraw(indices.data(), indices.data(), indexCount, &vertices[0].position.x, vertexCount, sizeof(star::graphics::Vertex), 1.05f);

		meshopt_optimizeVertexFetch(vertices.data(), indices.data(), indexCount, vertices.data(), vertexCount, sizeof(star::graphics::Vertex));

		const size_t maxVertices = 64;
		const size_t maxTriangles = 124;
		const float coneWeight = 0.0f;

		auto maxMeshlets = meshopt_buildMeshletsBound(indices.size(), maxVertices, maxTriangles);
		std::vector<meshopt_Meshlet> meshlets(maxMeshlets);
		std::vector<uint32_t> meshletVertices(maxMeshlets * maxVertices);
		std::vector<uint8_t> meshletTriangles(maxMeshlets * maxTriangles * 3);

		auto meshletCount = meshopt_buildMeshlets(meshlets.data(), meshletVertices.data(), meshletTriangles.data(), indices.data(), indices.size(), &vertices[0].position.x, vertices.size(), sizeof(star::graphics::Vertex), maxVertices, maxTriangles, coneWeight);
		//auto meshletCount = meshopt_buildMeshlets(meshlets.data(), meshletVertices.data(), meshletTriangles.data(), meshData.indices.data(), meshData.indices.size(), &meshData.vertices[0].position.x, meshData.vertices.size(), sizeof(star::graphics::Vertex), maxVertices, maxTriangles, coneWeight);

		meshData.meshlets = std::vector<star::graphics::Meshlet>(meshletCount);

		for(uint32_t i = 0; i < meshletCount; i++)
		{
			auto& meshlet = meshlets[i];
			auto& target = meshData.meshlets[i];

			target.vertexCount = meshlet.vertex_count;
			target.triangleCount = meshlet.triangle_count;

			auto lastVertex = meshlet.vertex_count + meshlet.vertex_offset;

			uint32_t idx = 0;
			for(uint32_t vi = meshlet.vertex_offset; vi < lastVertex; vi++)
			{
				target.vertexIndices[idx++] = meshletVertices[vi];
			}

			auto lastTriangle = (meshlet.triangle_count * 3) + meshlet.triangle_offset;

			idx = 0;
			for(uint32_t ti = meshlet.triangle_offset; ti < lastTriangle; ti+=3)
			{
				if (idx >= 371)
					auto b = 2;
				target.triangleIndices[idx++] = (meshletTriangles[ti]);
				target.triangleIndices[idx++] = (meshletTriangles[ti+1]);
				target.triangleIndices[idx++] = (meshletTriangles[ti+2]);
			}

			auto b = 2;
		}

		std::cout << "Built meshlets\n";

		meshData.vertices = vertices;
		meshData.indices = indices;
	}

}
