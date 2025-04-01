struct PixelInput
{
    float2 texCoord : TEXCOORD0;
    float4 position : SV_Position;
};

Texture2D drawData : register(t0);
SamplerState samplerState : register(s0);

float4 main(PixelInput input) : SV_TARGET
{
    return drawData.Sample(samplerState, input.texCoord);
}