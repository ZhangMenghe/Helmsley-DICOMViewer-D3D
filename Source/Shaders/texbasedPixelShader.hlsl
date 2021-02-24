Texture3D shaderTexture;
SamplerState uSampler;

struct v2f {
	float4 pos : SV_POSITION;
	float3 tex :TEXCOORD0;
};
cbuffer texPixConstantBuffer : register(b0){
	bool u_front : packoffset(c0);
	bool u_cut : packoffset(c1);
	float u_cut_texz : packoffset(c2.x);
};
float4 main(v2f input) : SV_TARGET{
	return float4(input.tex, 1.0);
	//if (u_cut) {
	//	if (u_front && input.tex.z > u_cut_texz) return .0f;
	//	else if (!u_front && input.tex.z < u_cut_texz) return .0f;
	//}
	//return shaderTexture.SampleLevel(uSampler, u_front? input.tex:float3(input.tex.xy, 1.0 - input.tex.z),0);
}
