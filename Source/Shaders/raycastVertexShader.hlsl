
// A constant buffer that stores the three basic column-major matrices for composing geometry.

cbuffer raycastConstantBuffer : register(b0) {
	matrix uModelMat;
	matrix uViewProjMat;
	float4 uCamPosInObjSpace;
};
// Per-vertex data used as input to the vertex shader.
struct VertexShaderInput{
	float3 pos : POSITION;
	float3 tex : TEXCOORD0;
};

// Per-pixel color data passed through the pixel shader.
struct v2f{
	float4 pos : SV_POSITION;
	float3 tex :TEXCOORD0;
	float3 screenPos : TEXCOORD1;
	float3 FragPos: TEXCOORD2;
	float3 raydir: TEXCOORD3;
};

// Simple shader to do vertex processing on the GPU.
v2f main(VertexShaderInput input){
	v2f output;
	output.raydir = input.pos - uCamPosInObjSpace.xyz;
	output.FragPos = mul(float4(input.pos, 1.0f), uModelMat);
	output.pos = mul(float4(input.pos, 1.0f), uViewProjMat);
	output.screenPos = output.pos.xyw;
	output.tex = input.tex;
	return output;
}
