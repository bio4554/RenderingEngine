#include "GlobalBindings.hlsli"

struct PixelIn
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

void main(PixelIn input)
{
    float4 texColor = albedo.Sample(linearSampler, input.uv);

    if (texColor.w < 1.0)
    {
        discard;
    }
}