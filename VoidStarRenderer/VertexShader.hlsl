#include "Shared.hlsli"

struct VertexInput
{
	uint vertexId : SV_VertexID;
};

struct VertexOutput
{
    float4 position : SV_Position;
    float3 normal : TEXCOORD0;
    float3 tangent : TEXCOORD1;
    float3 biTangent : TEXCOORD2;
    float2 uv : TEXCOORD3;
    nointerpolation int hasNormal : TEXCOORD4;
};

StructuredBuffer<Vertex> vertexBuffer : register(t0);
StructuredBuffer<SceneConstants> sceneConstantsBuffer : register(t1);

cbuffer MeshData : register(b0)
{
    float4x4 localToWorld;
    int hasNormal;
};

VertexOutput main(VertexInput input)
{
    VertexOutput output;

    SceneConstants sceneConstants = sceneConstantsBuffer[0];

    Vertex v = vertexBuffer[input.vertexId];

    float4x4 mvp = mul(sceneConstants.viewProjection, localToWorld);

    output.position = mul(mvp, v.position);
    output.normal = mul(localToWorld, v.normal);
    output.uv = v.uv;
    output.hasNormal = hasNormal;
    output.tangent = v.tangent;
    output.biTangent = v.biTangent;

    return output;
}