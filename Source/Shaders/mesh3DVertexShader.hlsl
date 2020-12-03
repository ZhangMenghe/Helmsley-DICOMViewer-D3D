

// A constant buffer that stores the three basic column-major matrices for composing geometry.

cbuffer raycastConstantBuffer : register(b0) {
	matrix model;
	matrix uViewProjMat;
};
// Per-vertex data used as input to the vertex shader.
struct VertexShaderInput{
	float3 pos : POSITION;
	float3 norm : NORMAL0;
};

// Per-pixel color data passed through the pixel shader.
struct v2f{
	float4 pos : SV_POSITION;
};

// Simple shader to do vertex processing on the GPU.
v2f main(VertexShaderInput input){
	v2f output;
	output.pos = mul(float4(-input.pos.y, input.pos.x, input.pos.z, 1.0f), model);
	output.pos = mul(output.pos, uViewProjMat);
	return output;
}
