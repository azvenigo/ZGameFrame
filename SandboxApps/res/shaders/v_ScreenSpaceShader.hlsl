struct VSInput {
    float3 pos : POSITION;  // Already in NDC space
    float2 uv  : TEXCOORD0; // Texture coordinates
};

struct PSInput {
    float4 pos : SV_POSITION;
    float2 uv  : TEXCOORD0;
};

PSInput VSMain(VSInput input) 
{
    PSInput output;
    output.pos = float4(input.pos, 1.0); // Directly pass as clip-space
    output.uv = input.uv;
    return output;
}