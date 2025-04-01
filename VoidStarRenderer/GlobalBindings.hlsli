#define _IN_HLSL
#ifndef GLOBAL_BINDINGS_HLSI
#define GLOBAL_BINDINGS_HLSI
#include "Shared.hlsli"

StructuredBuffer<Meshlet> meshlets : register(t0);
// Vertex buffer
StructuredBuffer<Vertex> vertexBuffer : register(t1);
StructuredBuffer<SceneConstants> sceneConstantsBuffer : register(t2);
StructuredBuffer<LightInfo> lightInfoBuffer : register(t3);
StructuredBuffer<CascadeData> cascadeData : register(t4);
Texture2D albedo : register(t10);
Texture2D normalMap : register(t11);
Texture2D metalnessMap : register(t12);
Texture2D roughnessMap : register(t13);
Texture2D alphaMap : register(t14);
// Meshlet buffer (array of meshlets)




Texture2DArray shadowMaps[] : register(t32);

SamplerState linearSampler : register(s0);
SamplerState pcfSampler : register(s1);
SamplerState pointSampler : register(s2);





#endif