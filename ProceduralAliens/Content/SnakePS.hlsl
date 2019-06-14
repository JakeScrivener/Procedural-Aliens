// Per-pixel color data passed through the pixel shader.
struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float4 posWorld : TEXCOORD0;
	float3 norm : NORMAL;
	float2 uv : TEXCOORD1;
};

cbuffer LightConstantBuffer : register(b1)
{
	float4 lightPos;
	float4 lightColour;
}

cbuffer CameraConstantBuffer : register (b2)
{
	float4 eyePos;
	float4 backgroundColour;
	float nearPlane;
	float farPlane;
	float2 padding;
}

Texture2D txDiffuse : register(t0);
SamplerState txSampler : register(s0);

// A pass-through function for the (interpolated) color data.
float4 main(PixelShaderInput input) : SV_TARGET
{
	float4 matDiffuse = float4(0.9, 0.2, 0.1, 1.0);
	float4 matSpec = float4(0.0, 0.0, 0.0, 1.0);
	float4 ambient = float4(0.3, 0.1, 0.1, 1.0);

	float texColour = txDiffuse.Sample(txSampler, input.uv);

	float3 n = normalize(input.norm);
	float3 viewDirection = normalize(eyePos - input.posWorld);
	float4 light = ambient;

	float3 lightDir = normalize(lightPos - input.posWorld);
	float diffuse = max(0.0, dot(lightDir, n));
	float3 R = normalize(reflect(-lightDir, n));
	float spec = pow(max(0.0, dot(viewDirection, R)), 0.8 * 128);
	light = saturate((ambient + (matDiffuse*diffuse) + (matSpec*spec)) * lightColour);

	float dist = length(input.posWorld - eyePos);
	float4 colour = texColour * light;
	colour = lerp(colour, float4(0.8, 0.8, 0.8, 1.0), 1.0 - exp(-0.0002*dist*dist));
	return colour;
}
