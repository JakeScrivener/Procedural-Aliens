// A constant buffer that stores the three basic column-major matrices for composing geometry.
cbuffer ModelViewProjectionConstantBuffer : register(b0)
{
	matrix model;
	matrix view;
	matrix projection;
};

// Per-vertex data used as input to the vertex shader.
struct VertexShaderInput
{
	float3 pos : POSITION;
};

// Per-pixel color data passed through the pixel shader.
struct VS_Canvas
{
	float4 pos : SV_POSITION;
	float2 canvasXY : TEXCOORD0;
};

// Simple shader to do vertex processing on the GPU.
VS_Canvas main(VertexShaderInput input)
{
	VS_Canvas output;

	output.pos = float4(sign(input.pos.xy), 0, 1);

	float aspectRatio = projection._m11 / projection._m00;
	output.canvasXY = sign(input.pos.xy)*float2(aspectRatio, 1.0);

	return output;
}
