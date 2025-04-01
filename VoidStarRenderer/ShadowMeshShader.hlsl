#include "Shared.hlsli"
#include "GlobalBindings.hlsli"

struct VertexOut
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

struct PrimOut
{
    uint layer : SV_RenderTargetArrayIndex;
};

// Mesh data constants
cbuffer MeshData : register(b0)
{
    float4x4 localToWorld;
    int hasNormal;
    int hasMetalness;
    int hasRoughness;
    int hasAlpha;
    uint meshletCount;
};

[numthreads(128, 1, 1)]
[outputtopology("triangle")]
void main(
	uint GroupThreadID : SV_GroupThreadID,
    uint GroupID : SV_GroupID,
	in payload ShadowAmpPayload payload,
    out vertices VertexOut outputVertices[64],
	out indices uint3 outputTriangles[124],
	out primitives PrimOut outputPrims[124]
)
{
    if (payload.meshletIdx >= meshletCount)
        return;

    SceneConstants sceneConstants = sceneConstantsBuffer[0];
    Meshlet meshlet = meshlets[payload.meshletIdx];

    SetMeshOutputCounts(meshlet.VertexCount, meshlet.PrimitiveCount);

    if (GroupThreadID < meshlet.VertexCount)
    {
        uint index = meshlet.VertexIndices[GroupThreadID];
        Vertex v = vertexBuffer[index];

        float4x4 mvp = mul(cascadeData[0].cascadeViewProjs[GroupID], localToWorld);

        outputVertices[GroupThreadID].position = mul(mvp, float4(v.position, 1.0));
        outputVertices[GroupThreadID].uv = v.uv;
    }

    if (GroupThreadID < meshlet.PrimitiveCount)
    {
        uint3 triIndices = uint3(meshlet.PrimitiveIndices[(GroupThreadID * 3) + 0],
								 meshlet.PrimitiveIndices[(GroupThreadID * 3) + 1],
								 meshlet.PrimitiveIndices[(GroupThreadID * 3) + 2]);

        outputTriangles[GroupThreadID] = triIndices;
        outputPrims[GroupThreadID].layer = GroupID;
    }
}