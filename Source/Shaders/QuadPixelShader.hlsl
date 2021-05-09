Texture2D shaderTexture;
SamplerState SampleType;

// Per-pixel color data passed through the pixel shader.
struct PixelShaderInput{
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD0;
};
// A pass-through function for the (interpolated) color data.
float4 main(PixelShaderInput input) : SV_TARGET{
	//return float4(input.tex.xy,.0f, 1.0f);
	// Sample the pixel color from the texture using the sampler at this texture coordinate location.
	return shaderTexture.Sample(SampleType, input.tex);
}
