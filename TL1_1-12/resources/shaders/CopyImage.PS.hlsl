#include "CopyImage.hlsli"

Texture2D<float4> gtexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main( VertexShaderOutput input )
{
    PixelShaderOutput output;
    output.color = gtexture.Sample(gSampler, input.texcoord);
    return output;
}