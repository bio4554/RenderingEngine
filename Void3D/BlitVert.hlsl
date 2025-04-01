struct VertexOutput
{
    float2 texCoord : TEXCOORD0;
    float4 position : SV_Position;
};

VertexOutput main(uint vertexIndex : SV_VertexID)
{
    VertexOutput output;

    float2 uv = float2((vertexIndex << 1) & 2, vertexIndex & 2);
    output.position = float4(uv * float2(2, -2) + float2(-1, 1), 0, 1);
    output.texCoord = uv;
    return output;
}
