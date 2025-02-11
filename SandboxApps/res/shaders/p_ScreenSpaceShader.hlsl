Texture2D tex : register(t0);
SamplerState samp : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

float4 PSMain(PSInput input) : SV_Target
{
		//return float4(input.uv, 0.0, 1.0); // Maps UV coordinates to colors
    return tex.Sample(samp, input.uv);
    // return float4(1, 0, 0, 1); // Red color
}