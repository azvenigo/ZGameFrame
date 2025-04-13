//struct PSInput {
//    float4 position : SV_POSITION;
//    float2 uv : TEXCOORD0;
//};



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







Texture2D grassTexture : register(t0);
SamplerState samplerState : register(s0);

cbuffer TimeBuffer : register(b1) {
    float time;
}

// Simple Perlin noise function for texture-based randomness
float random(float2 st) {
    return frac(sin(dot(st.xy, float2(12.9898, 78.233))) * 43758.5453123);
}

float perlinNoise(float2 uv) {
    float2 i = floor(uv);
    float2 f = frac(uv);
    
    float a = random(i);
    float b = random(i + float2(1.0, 0.0));
    float c = random(i + float2(0.0, 1.0));
    float d = random(i + float2(1.0, 1.0));
    
    float2 u = f * f * (3.0 - 2.0 * f);
    
    return lerp(a, b, u.x) + (c - a) * u.y * (1.0 - u.x) + (d - b) * u.x * u.y;
}

float4 PSMain(PSInput input) : SV_Target {

		    	  float3 lightDirNorm = normalize(-input.nor);
		float2 worldUV = lightDirNorm.xy * 0.1;
//		float noise = perlinNoise(worldUV + time * 0.1) * 0.1;

    // Animate the grass sway using a sin function and noise
    float windEffect = sin(input.uv.y * 20.0 * time) * 0.05;
    float noise = perlinNoise(input.uv * 5.0 + time * 0.1) * 0.1;
    
    float2 displacedUV = input.uv + float2(windEffect + noise, 0.0);
    float4 grassColor = grassTexture.Sample(samplerState, displacedUV);
    
    // Darken grass at the bottom to simulate depth
    grassColor.rgb *= lerp(0.6, 1.0, input.uv.y);
    //grassColor.rgb = float3(0, 0, 1);
    
    //grassColor.rgb = lightDirNorm;
    
    return grassColor;
}
