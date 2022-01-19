Texture3D shaderTexture;
SamplerState uSampler;

struct v2f {
	float4 pos : SV_POSITION;
	float3 norm : NORMAL;
	float3 col : COLOR;
};
cbuffer ColorConstantBuffer : register(b0) {
	float4 u_color;
};
float4 main(v2f input) : SV_TARGET{
	float3 norm = normalize(input.norm);
	norm.x = norm.x / 2.0f + 1.0f;
	norm.y = norm.y / 2.0f + 1.0f;
	norm.z = norm.z / 2.0f + 1.0f;
	float3 col = input.col;
	return float4(norm.xyz, 1.0);
}
