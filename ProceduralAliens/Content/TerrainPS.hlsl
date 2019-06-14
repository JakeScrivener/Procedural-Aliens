// Per-pixel color data passed through the pixel shader.
struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float4 posWorld : TEXCOORD0;
	float3 norm : NORMAL;
};

cbuffer ModelViewProjectionConstantBuffer : register(b0)
{
	matrix model;
	matrix view;
	matrix projection;
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

//Generate random number
float Hash(float2 grid) {
	float h = dot(grid, float2 (127.1, 311.7));
	return frac(sin(h)*43758.5453123);
}
//Smooth noise
float Noise(in float2 p)
{
	float2 grid = floor(p);
	float2 f = frac(p);
	float2 uv = f * f*(3.0 - 2.0*f);
	float n1, n2, n3, n4;
	n1 = Hash(grid + float2(0.0, 0.0)); n2 = Hash(grid + float2(1.0, 0.0));
	n3 = Hash(grid + float2(0.0, 1.0)); n4 = Hash(grid + float2(1.0, 1.0));
	n1 = lerp(n1, n2, uv.x); n2 = lerp(n3, n4, uv.x);
	n1 = lerp(n1, n2, uv.y);
	return n1;//2*(2.0*n1 -1.0);
}
//Layer noise
float FractalNoise(in float2 xy)
{
	float w = 0.7;
	float f = 0.0;
	for (int i = 0; i < 4; i++)
	{
		f += Noise(xy) * w;
		w *= 0.5;
		xy *= 2.7;
	}
	return f;
}

// A pass-through function for the (interpolated) color data.
float4 main(PixelShaderInput input) : SV_TARGET
{
	float4 matDiffuse = float4(0.9, 0.2, 0.1, 1.0);
	float4 matSpec = float4(0.0, 0.0, 0.0, 1.0);
	float4 ambient = float4(0.3, 0.1, 0.1, 1.0);

	float texColour = FractalNoise(input.posWorld.xz);

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
