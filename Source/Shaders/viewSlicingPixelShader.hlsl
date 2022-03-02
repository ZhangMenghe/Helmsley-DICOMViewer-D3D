Texture3D shaderTexture;
SamplerState uSampler;

struct v2f {
	float4 pos : SV_POSITION;
	float3 tex :TEXCOORD0;
};
cbuffer texPixConstantBuffer : register(b0){
	bool u_front : packoffset(c0);
	bool u_cut : packoffset(c1);
	float u_dist : packoffset(c2);
	float4 u_cut_point : packoffset(c3);
	float4 u_cut_normal : packoffset(c4);
	float4 u_plane_normal : packoffset(c5);
};

float4 getColor(float3 input) {
	if (u_cut) {
		// calculate plane to point
		float3 dir = u_cut_point.xyz - (input - 0.5);
		float dp = dot(dir, float3(u_cut_normal.xyz));
		//return float4(u_cut_point.xyz, 0.25);
		if (dp > 0) {
			// above the plane
			return float4(0, 0, 0, 0);
		}
	}

	//return float4(1,1,1, 0.25);
	//if (u_cut) {
	//	if (u_front && input.tex.z > u_cut_texz) return .0f;
	//	else if (!u_front && input.tex.z < u_cut_texz) return .0f;
	//}

	
	return shaderTexture.SampleLevel(uSampler, input * 1.05, 0);
}
float4 main(v2f input) : SV_TARGET{
	float dist = u_dist / 2.0;
	float3 sample_pos = float3(input.tex.x, 1.0 - input.tex.y, input.tex.z);

	//return getColor(sample_pos - dist * u_plane_normal)
	//	+ getColor(sample_pos)
	//	+ getColor(sample_pos + dist * u_plane_normal);
	return getColor(sample_pos);
}
