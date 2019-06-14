cbuffer ModelViewProjectionConstantBuffer : register(b0)
{
	matrix model;
	matrix view;
	matrix projection;
};

cbuffer TimeConstantBuffer : register(b2)
{
	float time;
	float3 padding2;
}

struct GeometryShaderInput
{
	float4 pos : SV_Position;
};

struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float4 posWorld : TEXCOORD0;
	float3 norm : NORMAL;
	float2 uv : TEXCOORD1;
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

[maxvertexcount(40)]
void main(line GeometryShaderInput input[2], inout TriangleStream<PixelShaderInput> OutputStream)
{
	PixelShaderInput output = (PixelShaderInput)0;

	float theta = 0;
	float3 circlePos = float3(sin(theta), 0, cos(theta));

	float3 linePos[2];

	//base positions
	linePos[0] = input[0].pos;
	linePos[0].z -= time;
	linePos[0].x += sin(linePos[0].z);
	float3 basePos = linePos[0];

	linePos[0].y = (FractalNoise(linePos[0].xz / 50) * 10) - 9.8;

	linePos[1] = input[1].pos;
	linePos[1].z -= time;
	linePos[1].x += sin(linePos[1].z);
	float3 basePos2 = linePos[1];

	linePos[1].y = (FractalNoise(linePos[1].xz / 50) * 10) - 9.8;

	//circle positions
	for (int i = 0; i < 20; i++)
	{
		theta = 6.28 * ((float)i / 18);
		circlePos = float3(sin(theta), cos(theta), 0) * 0.2;

		//circle1
		linePos[0] += circlePos;

		output.pos = float4(linePos[0], 1);
		output.uv = float2(((float)i / 18), 0);
		output.norm = normalize(output.pos.xyz - basePos);
		output.posWorld = mul(output.pos, model);
		output.pos = mul(output.posWorld, view);
		output.pos = mul(output.pos, projection);
		OutputStream.Append(output);

		//circle2
		linePos[1] += circlePos;

		output.pos = float4(linePos[1], 1);
		output.uv = float2(((float)i / 18), 1);
		output.norm = normalize(output.pos.xyz - basePos2);
		output.posWorld = mul(output.pos, model);
		output.pos = mul(output.posWorld, view);
		output.pos = mul(output.pos, projection);
		OutputStream.Append(output);
	}
	OutputStream.RestartStrip();
}