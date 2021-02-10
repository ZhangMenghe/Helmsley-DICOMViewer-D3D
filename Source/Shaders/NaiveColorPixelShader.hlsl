struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD0;
};
// A pass-through function for the (interpolated) color data.
float4 main(PixelShaderInput input) : SV_TARGET{
	return float4(input.tex, .0f, 1.0f);
}
