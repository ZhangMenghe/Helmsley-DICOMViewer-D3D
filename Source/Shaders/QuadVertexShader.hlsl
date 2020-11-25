// A constant buffer that stores the three basic column-major matrices for composing geometry.
cbuffer ModelViewProjectionConstantBuffer : register(b0)
{
	matrix model;
	matrix uViewProjMat;
};

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
PixelShaderInput main(VertexShaderInput input){
	PixelShaderInput output;
	float4 pos = mul(float4(input.pos.xy, .0f, 1.0f), model);
	//debug only! xmmatrix trans not working
	pos.x += 0.8;
	pos.y -= 0.8;
	pos = mul(pos, uViewProjMat);
	output.pos = pos;

	output.tex = input.tex.xy;

	return output;
}
