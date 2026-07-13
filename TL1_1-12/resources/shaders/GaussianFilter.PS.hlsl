#include "Fullscreen.hlsli"

Texture2D<float4> gtexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

static const float32_t PI = 3.14159265f;

static const float32_t2 kIndex3x3[3][3] =
{
    { { -1.0f, -1.0f }, { 0.0f, -1.0f }, { 1.0f, -1.0f } },
    { { -1.0f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f } },
    { { -1.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f } },
};

float32_t gauss(float32_t x, float32_t y, float32_t sigma)
{
    float32_t exponent = -(x * x + y * y) / (2.0f * sigma * sigma);
    float32_t denominator = 2.0f * PI * sigma * sigma;

    return exp(exponent) / denominator;
}

PixelShaderOutput main(VertexShaderOutput input)
{
    uint32_t width, height;
    gtexture.GetDimensions(width, height);

    float32_t2 uvStepSize = float32_t2(rcp(width), rcp(height));

    PixelShaderOutput output;
    output.color.rgb = float32_t3(0.0f, 0.0f, 0.0f);
    output.color.a = 1.0f;

    float32_t weight = 0.0f;
    float32_t kernel3x3[3][3];

    for (int32_t x = 0; x < 3; ++x)
    {
        for (int32_t y = 0; y < 3; ++y)
        {
            kernel3x3[x][y] = gauss(kIndex3x3[x][y].x, kIndex3x3[x][y].y, 2.0f);
            weight += kernel3x3[x][y];
        }
    }

    for (int32_t x = 0; x < 3; ++x)
    {
        for (int32_t y = 0; y < 3; ++y)
        {
            float32_t2 texcoord = input.texcoord + kIndex3x3[x][y] * uvStepSize;

            float32_t3 fetchColor = gtexture.Sample(gSampler, texcoord).rgb;

            output.color.rgb += fetchColor * kernel3x3[x][y];
        }
    }

    output.color.rgb *= rcp(weight);

    return output;
}