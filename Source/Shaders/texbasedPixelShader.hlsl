Texture3D shaderTexture;
SamplerState uSampler;

struct v2f {
	float4 pos : SV_POSITION;
	float3 tex : TEXCOORD0;
	float3 pos_model : TEXCOORD1;
};
cbuffer cutPlaneConstantBuffer : register(b0){
	float4 u_pp;
	float4 u_pn;
};
float4 main(v2f input) : SV_TARGET{
	//return float4(input.tex, 1.0);
	if (u_pn.w >= .0f) {
		if (dot(input.pos_model.xyz - u_pp.xyz, u_pn.xyz) < .0f) return .0f;
	}
	return shaderTexture.SampleLevel(uSampler, float3(input.tex.x, 1.0-input.tex.y, input.tex.z), 0);
}
