Texture3D shaderTexture;
SamplerState uSampler;

struct v2f {
	float4 pos : SV_POSITION;
	float3 tex :TEXCOORD0;
};
cbuffer texPixConstantBuffer : register(b0){
	bool u_front;
};
// A pass-through function for the (interpolated) color data.
float4 main(v2f input) : SV_TARGET{
	//return float4(input.tex, 1.0);
	return shaderTexture.SampleLevel(uSampler, u_front? input.tex:float3(input.tex.xy, 1.0 - input.tex.z),0);
}