#include "Shared.hlsli"
#include "GlobalBindings.hlsli"

struct PixelInput
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

float3 RandomColor(uint seed)
{
    // Constants for the hash function
    const uint prime1 = 1103515245;
    const uint prime2 = 12345;

    seed = seed * prime1 + prime2;
    float r = frac((seed / 65536.0) * 0.00390625); // Normalize to range [0, 1]

    seed = seed * prime1 + prime2;
    float g = frac((seed / 65536.0) * 0.00390625);

    seed = seed * prime1 + prime2;
    float b = frac((seed / 65536.0) * 0.00390625);

    // Return as a float3 color
    return float3(r, g, b);
}

float4 main(PixelInput input) : SV_TARGET
{
    SceneConstants sceneConstants = sceneConstantsBuffer[0];

    float4 texColor = albedo.Sample(linearSampler, input.uv);

    if(texColor.w < 0.5f)
    {
        discard;
    }

    if(hasAlpha == 1)
    {
        float4 alpha = alphaMap.Sample(linearSampler, input.uv);
        if(alpha.r < 0.5f)
            discard;
    }

    float3 normal = input.normal;

    if(hasNormal != 0 && sceneConstants.renderSettings.normalMapEnable != 0)
    {
        float4 normalSample = normalMap.Sample(pointSampler, input.uv);
        normalSample = (2.0 * normalSample) - 1.0;

        float3 T = normalize(normalize(input.tangent) - normalize(input.normal) * dot(normalize(input.normal), normalize(input.tangent.xyz)));

        float3x3 TBN = float3x3(T, input.biTangent, normalize(input.normal));

        normal = normalize(mul(normalSample, TBN));
    }

    //float3 lightDir = normalize(float3(0.f, -1.f, 1.f));

    //float lightDiff = max(dot(normal, -lightDir), 0.0);

	float4 lightContribAcc = float4(0,0,0,1);
    bool needAmbient = true;
    for (uint i = 0; i < sceneConstants.lightCount; i++)
    {
        LightInfo light = lightInfoBuffer[i];

        if(light.type == 1) // directional
        {
            float lightContrib = max(dot(normal, -light.direction), 0.0);

            uint cascadeIndex = 0;
            for (uint j = 0; j < SHADOW_CASCADES; j++)
            {
	            if(input.projPosition.z > light.shadowMapInfo.splits[j])
	            {
                    cascadeIndex = j + 1;
                }
            }

            if(cascadeIndex >= 4)
            {
                cascadeIndex = 3;
            }

            //float3 lightViewDir = normalize(mul(sceneConstants.view, float4(normalize(light.direction.xyz), 0.0)).xyz);
            float4 lightSpacePos = mul(light.shadowMapInfo.viewProj[cascadeIndex], float4(input.worldPosition, 1.0));
            lightSpacePos = lightSpacePos / lightSpacePos.w;

            float3 shadowCoords = float3((lightSpacePos.x + 1.0) * 0.5, 1.0 - (lightSpacePos.y + 1.0) * 0.5, lightSpacePos.z);

            float bias = 0.001;
            float4 shadowSample = shadowMaps[light.shadowIdx].Sample(pointSampler, float3(shadowCoords.xy, cascadeIndex));

            if(shadowSample.x < shadowCoords.z - bias)
            {
	            // in shadow?
                //finalColor = finalColor * 0.1;
            }
			else
			{
                needAmbient = false;
                lightContribAcc += lightContrib * float4(1, 1, 1, 0);
            }
        }
    }

    if(needAmbient)
    {
        lightContribAcc = float4(0.05, 0.05, 0.05, 1);
    }

    float4 finalColor = lightContribAcc * texColor;

    if(sceneConstants.renderSettings.meshletDebug != 0)
    {
        return float4(RandomColor(input.groupId), 1.0);
    }

    if(sceneConstants.renderSettings.enableLight == 0)
    {
        return texColor;
    }

    return finalColor;
}

