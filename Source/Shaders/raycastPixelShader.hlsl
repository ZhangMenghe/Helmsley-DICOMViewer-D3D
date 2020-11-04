//Texture2D shaderTexture;
SamplerState SampleType;

// Per-pixel color data passed through the pixel shader.
struct v2f {
	float4 pos : SV_POSITION;
	float3 raydir: TEXCOORD0;
	float3 screenPos : TEXCOORD1;
	float3 FragPos: TEXCOORD2;
	float3 tex :TEXCOORD3;
};

// A pass-through function for the (interpolated) color data.
float4 main(v2f input) : SV_TARGET{
	return float4(input.tex, 1.0f);
	// Sample the pixel color from the texture using the sampler at this texture coordinate location.
	//return shaderTexture.Sample(SampleType, input.tex.xy);
}
