Texture3D shaderTexture;
SamplerState uSampler;

struct v2f {
	float4 pos : SV_POSITION;
	float3 tex :TEXCOORD0;
	float3 ro : TEXCOORD1;
	//float3 FragPos: TEXCOORD2;
	float3 raydir: TEXCOORD2;
};

cbuffer raypixConstantBuffer : register(b0) {
	bool u_cut : packoffset(c0);
	bool u_cutplane_realsample : packoffset(c1);
	float4 u_pp: packoffset(c2);
	float4 u_pn: packoffset(c3);
};

// A pass-through function for the (interpolated) color data.
float4 main(v2f input) : SV_TARGET{
	return float4(input.tex, 1.0);
}