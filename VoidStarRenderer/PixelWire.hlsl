#include "Shared.hlsli"

struct PixelInput
{
    float4 position : SV_Position;
    float3 normal : TEXCOORD0;
    float3 tangent : TEXCOORD1;
    float3 biTangent : TEXCOORD2;
    float2 uv : TEXCOORD3;
    nointerpolation int hasNormal : TEXCOORD4;
    nointerpolation uint groupId : TEXCOORD5;
};

Texture2D normalMap : register(t3);
Texture2D albedo : register(t2);
SamplerState samplerState : register(s0);
StructuredBuffer<SceneConstants> sceneConstantsBuffer : register(t1);

float4 main(PixelInput input) : SV_TARGET
{
    SceneConstants sceneConstants = sceneConstantsBuffer[0];

    float4 texColor = albedo.Sample(samplerState, input.uv);

    return texColor;
}

