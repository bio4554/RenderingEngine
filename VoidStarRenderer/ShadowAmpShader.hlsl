#include "Shared.hlsli"
#include "GlobalBindings.hlsli"

struct VertexOut
{
    float4 position : SV_Position;
    float3 normal : TEXCOORD0;
    float3 tangent : TEXCOORD1;
    float3 biTangent : TEXCOORD2;
    float2 uv : TEXCOORD3;
    nointerpolation int hasNormal : TEXCOORD4;
    nointerpolation uint groupId : TEXCOORD5;
};





// Mesh data constants
cbuffer MeshData : register(b0)
{
    float4x4 localToWorld;
    int hasNormal;
    uint meshletCount;
};



groupshared ShadowAmpPayload payload;

[numthreads(1, 1, 1)]
void main(uint gtid : SV_GroupThreadID, uint dtid : SV_DispatchThreadID, uint gid : SV_GroupID)
{
    payload.meshletIdx = gid;

    DispatchMesh(4, 1, 1, payload);
}