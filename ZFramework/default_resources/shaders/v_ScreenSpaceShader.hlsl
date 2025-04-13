struct VSInput 
{
    float3 pos : POSITION;  // Already in NDC space
    float3 nor : NORMAL;  // Already in NDC space
    float2 uv  : TEXCOORD0; // Texture coordinates
};

struct PSInput 
{
    float4 pos : SV_POSITION;
    float3 nor : NORMAL;  // Already in NDC space
    float2 uv  : TEXCOORD0;
};

PSInput VSMain(VSInput input) 
{
    PSInput output;
    output.pos = float4(input.pos, 1.0); // Directly pass as clip-space
    output.nor = input.nor;
    output.uv = input.uv;
    
    // Check if the normal is valid (not zero-length)
    if (length(input.nor) > 0.001) 
    {
        output.nor = normalize(input.nor);
    } 
    else 
    {
        output.nor = float3(0, 0, 0); // No valid normal
    }
    
    return output;
}