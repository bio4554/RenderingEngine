#include "Shared.hlsli"
#include "GlobalBindings.hlsli"

static const float PI = 3.141592;
static const float Epsilon = 0.00001;

// Constant normal incidence Fresnel factor for all dielectrics.
static const float3 Fdielectric = 0.04;

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

// taken from https://github.com/Nadrin/PBR/blob/master/data/shaders/hlsl/pbr.hlsl

// GGX/Towbridge-Reitz normal distribution function.
// Uses Disney's reparametrization of alpha = roughness^2.
float ndfGGX(float cosLh, float roughness)
{
    float alpha = roughness * roughness;
    float alphaSq = alpha * alpha;

    float denom = (cosLh * cosLh) * (alphaSq - 1.0) + 1.0;
    return alphaSq / (PI * denom * denom);
}

// Single term for separable Schlick-GGX below.
float gaSchlickG1(float cosTheta, float k)
{
    return cosTheta / (cosTheta * (1.0 - k) + k);
}

// Schlick-GGX approximation of geometric attenuation function using Smith's method.
float gaSchlickGGX(float cosLi, float cosLo, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0; // Epic suggests using this roughness remapping for analytic lights.
    return gaSchlickG1(cosLi, k) * gaSchlickG1(cosLo, k);
}

// Shlick's approximation of the Fresnel factor.
float3 fresnelSchlick(float3 F0, float cosTheta)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float4 main(PixelInput input) : SV_TARGET
{
	SceneConstants sceneConstants = sceneConstantsBuffer[0];

	float4 texColor = albedo.Sample(linearSampler, input.uv);

	if (texColor.w < 0.99f)
	{
		discard;
	}

	float3 normal = input.normal;

    if (hasNormal == 1)
    {
        float4 normalSample = normalMap.Sample(linearSampler, input.uv);
        normalSample = (2.0 * normalSample) - 1.0;

        float3 T = normalize(
		normalize(input.tangent) - normalize(input.normal) * dot(normalize(input.normal),
		                                                         normalize(input.tangent.xyz)));

        float3x3 TBN = float3x3(T, input.biTangent, normalize(input.normal));

        normal = normalize(mul(normalSample, TBN));
    }
    //normal = input.normal;

	float roughness = roughnessMap.Sample(linearSampler, input.uv).x;
	float metalness = metalnessMap.Sample(linearSampler, input.uv).x;

	//float3 lightDir = normalize(float3(0.f, -1.f, 1.f));

	//float lightDiff = max(dot(normal, -lightDir), 0.0);

	float3 lightContribAcc = float3(0, 0, 0);
	bool needAmbient = true;
	for (uint i = 0; i < sceneConstants.lightCount; i++)
	{
		LightInfo light = lightInfoBuffer[i];

		if (light.type == 1) // directional
		{
			//float lightContrib = max(dot(normal, -light.direction), 0.0);

			uint cascadeIndex = 0;
			for (uint j = 0; j < SHADOW_CASCADES; j++)
			{
				if (input.projPosition.z > light.shadowMapInfo.splits[j])
				{
					cascadeIndex = j + 1;
				}
			}

			if (cascadeIndex >= 4)
			{
				cascadeIndex = 3;
			}

			//float3 lightViewDir = normalize(mul(sceneConstants.view, float4(normalize(light.direction.xyz), 0.0)).xyz);
			float4 lightSpacePos = mul(light.shadowMapInfo.viewProj[cascadeIndex], float4(input.worldPosition, 1.0));
			lightSpacePos = lightSpacePos / lightSpacePos.w;

			float3 shadowCoords = float3((lightSpacePos.x + 1.0) * 0.5, 1.0 - (lightSpacePos.y + 1.0) * 0.5,
			                             lightSpacePos.z);
			
			float4 shadowSample = shadowMaps[light.shadowIdx].Sample(pointSampler,
			                                                         float3(shadowCoords.xy, cascadeIndex));

			if (shadowSample.x < shadowCoords.z - sceneConstants.renderSettings.shadowBias)
			{
				// in shadow?
				//finalColor = finalColor * 0.1;
			}
			else
			{
                float3 F0 = lerp(float3(0.04, 0.04, 0.04), texColor.rgb, metalness);

                float3 N = normalize(normal);

                float3 V = normalize(sceneConstants.cameraPosition - input.worldPosition); // View direction
                float3 L = normalize(-light.direction); // Light direction
                float3 H = normalize(V + L); // Halfway vector

                float3 F = F0 + (1.0 - F0) * pow(1.0 - max(dot(H, V), 0.0), 5.0);

                float NdotV = max(dot(N, V), 0.0);
                float NdotL = max(dot(N, L), 0.0);
                float k = pow(roughness + 1.0, 2.0) / 8.0;
                float G_V = NdotV / (NdotV * (1.0 - k) + k);
                float G_L = NdotL / (NdotL * (1.0 - k) + k);
                float G = G_V * G_L;

                float NdotH = max(dot(N, H), 0.0);
                float alpha = roughness * roughness;
                float alpha2 = alpha * alpha;
                float NDF = alpha2 / (PI * pow(NdotH * NdotH * (alpha2 - 1.0) + 1.0, 2.0));

                float3 diffuse = (1.0 - metalness) * texColor.rgb / PI;

                float3 specular = (F * G * NDF) / (4.0 * NdotL * NdotV + 0.001); // Prevent division by zero

                float3 kD = (1.0 - F) * (1.0 - metalness); // Diffuse factor
                float3 radiance = light.color * light.intensity * 5;

                float3 color = (kD * diffuse + specular) * radiance * NdotL;

				needAmbient = false;
				lightContribAcc += color;
			}
		}
	}

	if (needAmbient)
	{
		lightContribAcc = float3(0.05, 0.05, 0.05) * texColor.xyz;
	}

	float4 finalColor = float4(lightContribAcc, texColor.w);

	if (sceneConstants.renderSettings.meshletDebug != 0)
	{
		return float4(RandomColor(input.groupId), 1.0);
	}

	if (sceneConstants.renderSettings.enableLight == 0)
	{
		return texColor;
	}

	return finalColor;
}
