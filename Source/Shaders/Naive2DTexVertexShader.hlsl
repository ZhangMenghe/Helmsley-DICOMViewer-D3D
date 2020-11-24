// Per-vertex data used as input to the vertex shader.
struct VertexShaderInput
{
	float2 pos : POSITION;
	float2 tex : TEXCOORD0;
};

// Per-pixel color data passed through the pixel shader.
struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD0;
};

// Simple shader to do vertex processing on the GPU.
PixelShaderInput main(VertexShaderInput input) {
	PixelShaderInput output;
	output.pos = float4(input.pos, .0f, 1.0f);
	output.tex = input.tex;
	return output;
}
