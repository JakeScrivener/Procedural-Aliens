// A constant buffer that stores the three basic column-major matrices for composing geometry.
cbuffer ModelViewProjectionConstantBuffer : register(b0)
{
	matrix model;
	matrix view;
	matrix projection;
};
cbuffer CameraConstantBuffer : register (b1)
{
	float4 eyePos;
	float4 backgroundColour;
	float nearPlane;
	float farPlane;
	float2 padding;
}
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
	float2 uv : TEXCOORD0;
};

static const float3 g_positions[4] =
{
float3(-1, 1, 0),
float3(-1, -1, 0),
float3(1, 1, 0),
float3(1, -1, 0),
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

float3x3 rotX(float angle)
{
	float s = sin(angle);
	float c = cos(angle);

	return float3x3(
		1, 0, 0,
		0, c, -s,
		0, s, c);;
}

[maxvertexcount(6)]
void main(point GeometryShaderInput input[1], inout TriangleStream<PixelShaderInput> OutputStream)
{
	PixelShaderInput output = (PixelShaderInput)0;


	float4 vPos = input[0].pos;
	vPos.y = (FractalNoise(vPos.xz / 50) * 10) - 9.7;
	vPos = mul(vPos, model);
	vPos = mul(vPos, view);

	float quadSize = 0.5;
	//vertex 1:
	output.pos = vPos + float4(mul(quadSize * g_positions[0], rotX(sin(time + FractalNoise(input[0].pos.xz)*3))),0);
	output.pos = mul(output.pos, projection);
	output.uv = ((g_positions[0].xy*-1) + float2(1, 1)) / 2;
	OutputStream.Append(output);

	//vertex 2:
	output.pos = vPos + float4(quadSize*g_positions[1], 0.0);
	output.pos = mul(output.pos, projection);
	output.uv = ((g_positions[1].xy*-1) + float2(1, 1)) / 2;
	OutputStream.Append(output);

	//vertex 3:
	output.pos = vPos + float4(mul(quadSize * g_positions[2], rotX(sin(time + FractalNoise(input[0].pos.xz)))), 0);
	output.pos = mul(output.pos, projection);
	output.uv = ((g_positions[2].xy*-1) + float2(1, 1)) / 2;
	OutputStream.Append(output);
	//OutputStream.RestartStrip();

	////vertex 2:
	//output.pos = vPos + float4(quadSize*g_positions[1], 0.0);
	//output.pos = mul(output.pos, projection);
	//output.uv = ((g_positions[1].xy*-1) + float2(1, 1)) / 2;
	//OutputStream.Append(output);

	////vertex 3:
	//output.pos = vPos + float4(mul(quadSize * g_positions[2], rotX(sin(time + FractalNoise(input[0].pos.xz)))), 0);
	//output.pos = mul(output.pos, projection);
	//output.uv = ((g_positions[2].xy*-1) + float2(1, 1)) / 2;
	//OutputStream.Append(output);

	//vertex 4:
	output.pos = vPos + float4(quadSize*g_positions[3], 0.0);
	output.pos = mul(output.pos, projection);
	output.uv = ((g_positions[3].xy*-1) + float2(1, 1)) / 2;
	OutputStream.Append(output);

	OutputStream.RestartStrip();

}