Texture2D tex : register(t0);
SamplerState samp : register(s0);

cbuffer LightBuffer : register(b0)
{
    float3 lightDir;    // Directional light direction
    float intensity;
    float3 lightColor;  // Light color
    float padding;			// alignment
};

struct PSInput
{
    float4 position : SV_POSITION;
    float3 nor : NORMAL; 
    float2 uv : TEXCOORD;
};




float4 PSMain(PSInput input) : SV_Target
{
//return float4(input.nor * 0.5 + 0.5, 1.0);
//return float4(lightColor, 1.0);

    float4 texColor = tex.Sample(samp, input.uv);
   	float3 finalColor = texColor.rgb;
    
    // Check if normal is valid
    if (length(input.nor) > 0.001) 
    {
    //return float4(lightColor, 1.0);
    
    
    	  float3 norm = normalize(input.nor);
    	  float3 lightDirNorm = normalize(-lightDir);
    
        // Simple Lambertian shading
        float diff = max(dot(norm, lightDirNorm), 0.0);
        finalColor *= (lightColor * diff * intensity);
    }
    
    return float4(finalColor, texColor.a);
}