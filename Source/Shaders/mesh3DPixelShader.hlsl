Texture3D shaderTexture;
SamplerState uSampler;

struct v2f {
	float4 pos : SV_POSITION;
};

float4 main(v2f input) : SV_TARGET{
	return 1.0;
}
