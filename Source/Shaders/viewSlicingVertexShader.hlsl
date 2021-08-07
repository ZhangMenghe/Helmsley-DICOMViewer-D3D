

// A constant buffer that stores the three basic column-major matrices for composing geometry.

cbuffer ModelViewProjectionConstantBuffer : register(b0)
{
	matrix model;
	matrix uViewProjMat;
};

// Per-vertex data used as input to the vertex shader.
struct VertexShaderInput{
	float3 pos : POSITION;
};

// Per-pixel color data passed through the pixel shader.
struct v2f{
	float4 pos : SV_POSITION;
	float3 tex :TEXCOORD0;
};

// Simple shader to do vertex processing on the GPU.
v2f main(VertexShaderInput input){
	v2f output;
	float3 texCoord = clamp(input.pos + 0.5, float3(0,0,0), float3(1,1,1));
	output.tex = float3(texCoord);
	output.pos = float4(input.pos.xyz * 1.05, 1.0f);
	output.pos = mul(output.pos, model);
	output.pos = mul(output.pos, uViewProjMat);
	return output;
}
