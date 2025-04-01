#include "Shared.hlsli"
#include "GlobalBindings.hlsli"

struct VertexOut
{
    float4 position : SV_Position;
    float3 normal : TEXCOORD0;
    float3 tangent : TEXCOORD1;
    float3 biTangent : TEXCOORD2;
    float2 uv : TEXCOORD3;
    nointerpolation uint groupId : TEXCOORD7;
    float3 projPosition : TEXCOORD8;
    float3 worldPosition : TEXCOORD9;
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
    out vertices VertexOut outputVertices[64],
	out indices uint3 outputTriangles[124]
)
{
    if(GroupID >= meshletCount)
        return;

    SceneConstants sceneConstants = sceneConstantsBuffer[0];
    Meshlet meshlet = meshlets[GroupID];

    SetMeshOutputCounts(meshlet.VertexCount, meshlet.PrimitiveCount);

    if(GroupThreadID < meshlet.VertexCount)
    {
        uint index = meshlet.VertexIndices[GroupThreadID];
        Vertex v = vertexBuffer[index];

        float4x4 mvp = mul(sceneConstants.viewProjection, localToWorld);

        float4 transformed = mul(mvp, float4(v.position, 1.0));

        outputVertices[GroupThreadID].worldPosition = mul(localToWorld, float4(v.position, 1.0)).xyz;
        outputVertices[GroupThreadID].position = transformed;
        outputVertices[GroupThreadID].projPosition = transformed.xyz;
        outputVertices[GroupThreadID].normal = mul(localToWorld, float4(v.normal, 0.0)).xyz;
        outputVertices[GroupThreadID].uv = v.uv;
        outputVertices[GroupThreadID].tangent = v.tangent;
        outputVertices[GroupThreadID].biTangent = v.biTangent;
        outputVertices[GroupThreadID].groupId = GroupID;
    }

    if(GroupThreadID < meshlet.PrimitiveCount)
    {
        uint3 triIndices = uint3(meshlet.PrimitiveIndices[(GroupThreadID * 3) + 0],
								 meshlet.PrimitiveIndices[(GroupThreadID * 3) + 1],
								 meshlet.PrimitiveIndices[(GroupThreadID * 3) + 2]);

        outputTriangles[GroupThreadID] = triIndices;
    }
}