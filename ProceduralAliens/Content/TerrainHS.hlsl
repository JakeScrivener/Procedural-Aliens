struct HS_Quad_Tess_Factors {
	float Edges[4] : SV_TessFactor;
	float Inside[2] : SV_InsideTessFactor;
};

// Per-pixel color data passed through the pixel shader.
struct PixelShaderInput
{
	float4 pos : SV_POSITION;
};

HS_Quad_Tess_Factors ConstantHS()
{
	HS_Quad_Tess_Factors Output;

	Output.Edges[0] = 100;
	Output.Edges[1] = 100;
	Output.Edges[2] = 100;
	Output.Edges[3] = 100;
	Output.Inside[0] = 100;
	Output.Inside[1] = 100;

	return Output;
}

[domain("quad")]
[partitioning("fractional_even")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(4)]
[patchconstantfunc("ConstantHS")]
PixelShaderInput main(InputPatch<PixelShaderInput, 4> patch,
	uint i : SV_OutputControlPointID)
{
	PixelShaderInput Output;
	Output.pos = patch[i].pos;
	return Output;
}
