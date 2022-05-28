Texture3D shaderTexture;
SamplerState uSampler;

struct v2f {
	float4 pos : SV_POSITION;
	float3 norm : NORMAL;
	float3 col : COLOR;
	float4 light_dir: TANGENT;
};
cbuffer ColorConstantBuffer : register(b0) {
	float4 u_color;
};
float4 main(v2f input) : SV_TARGET{
	float3 norm = normalize(input.norm);
	norm.x = norm.x / 2.0f + 1.0f;
	norm.y = norm.y / 2.0f + 1.0f;
	norm.z = norm.z / 2.0f + 1.0f;
	
	float diff = max(dot(-input.light_dir.xyz, norm), 0.0);

	return diff * u_color;
}
