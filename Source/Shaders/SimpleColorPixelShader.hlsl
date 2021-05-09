struct PixelShaderInput {
	float4 pos : SV_POSITION;
	float4 ori_pos: POSITION;
};
cbuffer ColorConstantBuffer : register(b0) {
	float4 u_color;
};
// A pass-through function for the (interpolated) color data.
float4 main(PixelShaderInput input) : SV_TARGET{
	return u_color;
}
