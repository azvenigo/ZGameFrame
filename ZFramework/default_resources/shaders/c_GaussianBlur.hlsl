#define THREAD_GROUP_SIZE 16

Texture2D<float4> InputTexture : register(t0);
RWTexture2D<float4> OutputTexture : register(u0);

cbuffer BlurParams : register(b0)
{
    float2 texelSize;  // 1 / TextureWidth, 1 / TextureHeight
    float sigma;        // Gaussian blur falloff
    int radius;         // Blur radius
    int width;          // Texture width
    int height;         // Texture height
};

// Gaussian function with normalization
float Gaussian(float x, float sigma)
{
    float factor = 1.0 / (sqrt(2.0 * 3.1415926535) * sigma);
    return factor * exp(-(x * x) / (2.0 * sigma * sigma));
}

[numthreads(THREAD_GROUP_SIZE, THREAD_GROUP_SIZE, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    int2 texSize = int2(width, height);

    // Ensure within bounds
    if (DTid.x >= texSize.x || DTid.y >= texSize.y)
        return;

    float4 sum = 0.0;
    float weightSum = 0.0;

    // Apply 2D Gaussian blur
    for (int y = -radius; y <= radius; y++)
    {
        for (int x = -radius; x <= radius; x++)
        {
            int2 samplePos = clamp(DTid.xy + int2(x, y), int2(0, 0), texSize - int2(1, 1));
            float weight = Gaussian(length(float2(x, y)), sigma);
            sum += InputTexture[samplePos] * weight;
            weightSum += weight;
        }
    }

    // Normalize and write output
    OutputTexture[DTid.xy] = sum / weightSum;
}
