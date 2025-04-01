#pragma once
#include "ModelConverter.h"

namespace asset_baker
{
	class MeshBaker
	{
	public:
		MeshBaker(ImportedModelData* modelData);

		void Bake();

	private:
		void OptimizeMesh(ImportedMeshData& meshData);

		ImportedModelData* _pModelData;
	};
}
