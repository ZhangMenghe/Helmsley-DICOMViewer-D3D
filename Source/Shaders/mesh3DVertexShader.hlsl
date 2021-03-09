

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
	float3 norm : NORMAL;
	float3 col : COLOR;
};

// Simple shader to do vertex processing on the GPU.
v2f main(VertexShaderInput input){
	v2f output;
	output.col = input.pos.xyz * 2.0f;
	output.pos = mul(float4(input.pos.x, -input.pos.y, input.pos.z, 1.0f), model);
	//output.pos = mul(float4(input.pos, 1.0f), model);
	output.pos = mul(output.pos, uViewProjMat);
	output.norm = input.norm;
	return output;
}
