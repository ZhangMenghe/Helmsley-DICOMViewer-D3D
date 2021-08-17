
// A constant buffer that stores the three basic column-major matrices for composing geometry.
struct MVPConstantBuffer {
	matrix mm;
};
cbuffer raycastConstantBuffer : register(b0) {
	MVPConstantBuffer uMVP;
	float1 u_volume_thickness;
};

// Per-vertex data used as input to the vertex shader.
struct VertexPos3d{
	float3 pos : POSITION;
};

// Per-pixel color data passed through the pixel shader.
struct v2f{
	float4 pos : SV_POSITION;
	float3 tex : TEXCOORD0;
};

// Simple shader to do vertex processing on the GPU.
v2f main(VertexPos3d input){
	v2f output;
	output.tex = input.pos + 0.5f;
	output.pos = mul(float4(input.pos.xy, input.pos.z * u_volume_thickness, 1.0f), uMVP.mm);
	return output;
}
