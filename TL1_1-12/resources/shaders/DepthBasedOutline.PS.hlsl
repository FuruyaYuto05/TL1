#include "Fullscreen.hlsli"

Texture2D<float4> gtexture : register(t0);
Texture2D<float32_t> gDepthTexture : register(t1);

SamplerState gSampler : register(s0);
SamplerState gSamplerPoint : register(s1);

struct Material
{
    float32_t4x4 projectionInverse;
};

ConstantBuffer<Material> gMaterial : register(b0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

// 横方向の輪郭を見るKernel
static const float32_t kPrewittHorizontalKernel[3][3] =
{
    { -1.0f, 0.0f, 1.0f },
    { -1.0f, 0.0f, 1.0f },
    { -1.0f, 0.0f, 1.0f },
};

// 縦方向の輪郭を見るKernel
static const float32_t kPrewittVerticalKernel[3][3] =
{
    { -1.0f, -1.0f, -1.0f },
    { 0.0f, 0.0f, 0.0f },
    { 1.0f, 1.0f, 1.0f },
};

// 周囲9マスを見るためのindex
static const float32_t2 kIndex3x3[3][3] =
{
    { { -1.0f, -1.0f }, { 0.0f, -1.0f }, { 1.0f, -1.0f } },
    { { -1.0f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f } },
    { { -1.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f } },
};



PixelShaderOutput main(VertexShaderOutput input)
{
    uint32_t width, height;
    gtexture.GetDimensions(width, height);

    float32_t2 uvStepSize = float32_t2(rcp(width), rcp(height));

    float32_t2 difference = float32_t2(0.0f, 0.0f);

    for (int32_t x = 0; x < 3; ++x)
    {
        for (int32_t y = 0; y < 3; ++y)
        {
            float32_t2 texcoord = input.texcoord + kIndex3x3[x][y] * uvStepSize;

            float32_t ndcDepth = gDepthTexture.Sample(gSamplerPoint, texcoord);

            float32_t4 viewSpace = mul(
                float32_t4(0.0f, 0.0f, ndcDepth, 1.0f),
                gMaterial.projectionInverse
            );

            float32_t viewZ = viewSpace.z * rcp(viewSpace.w);

            difference.x += viewZ * kPrewittHorizontalKernel[x][y];
            difference.y += viewZ * kPrewittVerticalKernel[x][y];
        }
    }

    float32_t weight = length(difference);
    weight = saturate(weight * 0.6f);

    PixelShaderOutput output;
    output.color.rgb = (1.0f - weight) * gtexture.Sample(gSampler, input.texcoord).rgb;
    output.color.a = 1.0f;

    return output;
}