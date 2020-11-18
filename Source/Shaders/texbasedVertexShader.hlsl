

// A constant buffer that stores the three basic column-major matrices for composing geometry.

cbuffer ModelViewProjectionConstantBuffer : register(b0)
{
	matrix model;
	matrix uViewProjMat;
};

// Per-vertex data used as input to the vertex shader.
struct VertexShaderInput{
	float2 pos : POSITION;
	float2 tex : TEXCOORD0;
	float2 zinfo: TEXCOORD1;
};

// Per-pixel color data passed through the pixel shader.
struct v2f{
	float4 pos : SV_POSITION;
	float3 tex :TEXCOORD0;
};

// Simple shader to do vertex processing on the GPU.
v2f main(VertexShaderInput input){
	v2f output;
	output.tex = float3(input.tex, input.zinfo.y);
	output.pos = float4(input.pos.xy, input.zinfo.x, 1.0f);
	output.pos = mul(output.pos, model);
	output.pos = mul(output.pos, uViewProjMat);
	return output;
}
