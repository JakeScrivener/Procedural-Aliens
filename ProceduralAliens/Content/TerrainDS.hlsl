cbuffer ModelViewProjectionConstantBuffer : register(b0)
{
	matrix model;
	matrix view;
	matrix projection;
};

static float3 QuadPos[4] = {
float3(-1, 0, 1),
float3(-1, 0, -1),
float3(1, 0, 1),
float3(1, 0, -1)
};

// Per-pixel color data passed through the pixel shader.
struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float4 posWorld : TEXCOORD0;
	float3 norm : NORMAL;
};

struct DSInput
{
	float4 pos : SV_POSITION;
};

struct HS_Quad_Tess_Param {
	float Edges[4] : SV_TessFactor;
	float Inside[2] : SV_InsideTessFactor;
};

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

[domain("quad")]
PixelShaderInput main(HS_Quad_Tess_Param input,
	float2 UV : SV_DomainLocation,
	const OutputPatch<DSInput, 4> OutPatch)	
{
	PixelShaderInput Output;
	float3 vPos1 = (1.0 - UV.y)*QuadPos[0].xyz
		+ UV.y* QuadPos[1].xyz;
	float3 vPos2 = (1.0 - UV.y)*QuadPos[2].xyz
		+ UV.y* QuadPos[3].xyz;
	float3 uvPos = (1.0 - UV.x)*vPos1 + UV.x* vPos2;
		
	uvPos.y = FractalNoise(uvPos.xz);
	float dYx = FractalNoise(uvPos.xz + float2(0.1, 0.0));
	float dYz = FractalNoise(uvPos.xz + float2(0.0, 0.1));

	Output.norm = normalize(float3(uvPos.y - dYx,  0.2, uvPos.y - dYz));
	uvPos.xz *= 50;
	uvPos.y *= 10;
	uvPos.y -= 10;
	
	Output.posWorld = mul(float4(uvPos,1), model);
	Output.pos = mul(Output.posWorld, view);
	Output.pos = mul(Output.pos, projection);
	return Output;
}