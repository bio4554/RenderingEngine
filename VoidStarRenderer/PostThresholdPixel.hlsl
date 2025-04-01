struct PixelInput
{
    float2 texCoord : TEXCOORD0;
    float4 position : SV_Position;
};

Texture2D drawData : register(t0);
SamplerState samplerState : register(s0);

float4 main(PixelInput input) : SV_TARGET
{
    float4 sample = drawData.Sample(samplerState, input.texCoord);

    if(sample.r < 0.8)
        discard;

    return sample;
}