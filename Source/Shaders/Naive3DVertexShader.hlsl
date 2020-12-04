cbuffer raycastConstantBuffer : register(b0) {
	matrix model;
	matrix uViewProjMat;
};
// Per-vertex data used as input to the vertex shader.
struct Pos3DInput
{
	float3 pos : POSITION;
};

// Per-pixel color data passed through the pixel shader.
struct PixelShaderInput{
	float4 pos : SV_POSITION;
};

// Simple shader to do vertex processing on the GPU.
PixelShaderInput main(Pos3DInput input) {
	PixelShaderInput output;
	output.pos = mul(float4(input.pos, 1.0f), model);
	output.pos = mul(output.pos, uViewProjMat);
	return output;
}
