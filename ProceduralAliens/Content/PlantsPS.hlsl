// Per-pixel color data passed through the pixel shader.
struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD0;
};

Texture2D txDiffuse : register(t0);
SamplerState txSampler : register(s0);

// A pass-through function for the (interpolated) color data.
float4 main(PixelShaderInput input) : SV_TARGET
{
	return txDiffuse.Sample(txSampler, input.uv);
	//return float4(input.uv.x,input.uv.y,1,1);
}
