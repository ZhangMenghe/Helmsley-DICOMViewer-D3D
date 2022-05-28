cbuffer raycastConstantBuffer : register(b0) {
	matrix model;
	matrix uViewProjMat;
};

// Per-vertex data used as input to the vertex shader.
struct VertexPos3d{
	float3 pos : POSITION;
};

// Per-pixel color data passed through the pixel shader.
struct v2f {
	float4 pos : SV_POSITION;
	float3 norm : NORMAL;
	float3 col : COLOR;
	float4 light_dir: TANGENT;
};

// Simple shader to do vertex processing on the GPU.
v2f main(VertexPos3d input) {
	v2f output;
	output.col = input.pos.xyz * 2.0f;
	output.pos = mul(float4(input.pos, 1.0f), model);
	output.pos = mul(output.pos, uViewProjMat);
	output.norm = input.pos;
	output.light_dir = mul(float4(.0f, 1.0f, .0f, 1.0f), model);
	return output;
}
