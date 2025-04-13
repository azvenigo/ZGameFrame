// Vertical Gaussian Blur Compute Shader
#define THREAD_GROUP_SIZE 32

// Input texture for reading
Texture2D<float4> InputTexture : register(t0);
// Output texture for writing
RWTexture2D<float4> OutputTexture : register(u0);

cbuffer BlurParams : register(b0)
{
    float2 texelSize;    // 1 / TextureWidth, 1 / TextureHeight
    float sigma;         // Gaussian blur falloff
    int radius;          // Blur radius
    int width;           // Texture width
    int height;          // Texture height
};

// Cache kernel weights in shared memory
groupshared float kernelWeights[64];  // Assuming max radius of 32

// Precompute kernel weights for the current sigma
void FillKernelWeights()
{
    float weightSum = 0.0;
    
    [unroll]
    for (int i = 0; i <= radius; i++)
    {
        float weight = exp(-(i * i) / (2.0 * sigma * sigma));
        kernelWeights[i] = weight;
        if (i > 0)
            weightSum += 2.0 * weight;
        else
            weightSum += weight;
    }
    
    // Normalize weights
    float factor = 1.0 / weightSum;
    [unroll]
    for (int j = 0; j <= radius; j++)
    {
        kernelWeights[j] *= factor;
    }
}

// Vertical blur
[numthreads(THREAD_GROUP_SIZE, THREAD_GROUP_SIZE, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID, uint GI : SV_GroupIndex)
{
    if (GI == 0)
    {
        FillKernelWeights();
    }
    
    GroupMemoryBarrierWithGroupSync();
    
    if (DTid.x >= width || DTid.y >= height)
        return;
    
    float4 sum = InputTexture[DTid.xy] * kernelWeights[0];
    
    [unroll]
    for (int i = 1; i <= radius; i++)
    {
        int2 offset1 = int2(DTid.x, min(DTid.y + i, height - 1));
        int2 offset2 = int2(DTid.x, max(DTid.y - i, 0));
        
        sum += InputTexture[offset1] * kernelWeights[i];
        sum += InputTexture[offset2] * kernelWeights[i];
    }
    
    OutputTexture[DTid.xy] = sum;
}