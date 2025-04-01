#define _IN_HLSL
#ifndef SHARED_HLSI
#define SHARED_HLSI


#define MAX_VERTICES_PER_MESHLET 64
#define MAX_PRIMITIVES_PER_MESHLET 124
#define SHADOW_CASCADES 4

struct Vertex
{
	float3 position;
	float3 normal;
    float3 tangent;
    float3 biTangent;
	float4 color;
	float2 uv;
};

struct RenderSettings
{
    int normalMapEnable;
    int meshletDebug;
    int enableLight;
    float shadowBias;
};

struct SceneConstants
{
	float4x4 viewProjection;
    float4x4 view;
    float4x4 proj;
    float3 cameraPosition;
    uint lightCount;
    RenderSettings renderSettings;
};

struct MeshConstants
{
	float4x4 transformMatrix;
};

struct ShadowMapInfo
{
    float splits[SHADOW_CASCADES];
    float4x4 viewProj[SHADOW_CASCADES];
};

struct LightInfo
{
    float3 worldPosition;
    float3 direction;
    float3 color;
    float intensity;
    uint type; // 1 = directional
    ShadowMapInfo shadowMapInfo;
    uint shadowIdx;
};

struct Meshlet
{
    uint VertexCount;
    uint PrimitiveCount;
    uint VertexIndices[MAX_VERTICES_PER_MESHLET];
    uint PrimitiveIndices[MAX_PRIMITIVES_PER_MESHLET * 3]; // Each primitive has 3 indices
};

struct ShadowAmpPayload
{
    uint meshletIdx;
};

struct CascadeData
{
    float cascadeSplits[4];
    float4x4 cascadeViewProjs[4];
};

#endif