#include "Fullscreen.hlsli"

Texture2D<float4> gtexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    // RenderTextureの色を取得
    output.color = gtexture.Sample(gSampler, input.texcoord);

    // 周囲ほど暗く、中心ほど明るくするための計算
    float2 correct = input.texcoord * (1.0f - input.texcoord.yx);

    // xとyを掛け合わせる
    float vignette = correct.x * correct.y * 16.0f;

    // 0.8乗して少し調整し、0～1に収める
    vignette = saturate(pow(vignette, 0.8f));

    // 色に掛けることで、周囲を暗くする
    output.color.rgb *= vignette;

    return output;
}