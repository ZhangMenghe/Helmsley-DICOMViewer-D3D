#include "StructAndConsts.hlsli"
Texture3D<uint> srcVolume : register(t0);
Texture3D<uint> srcInfo : register(t1);
RWTexture3D<float4> destVolume : register(u0);

float UpdateOpacityAlpha(int woffset, float alpha) {
	float2 lb = u_opacity[woffset].xy, rb = u_opacity[woffset + 1].zw;
	if (alpha < lb.x || alpha > rb.x) return .0;
	float2 lm = u_opacity[woffset].zw, lt = u_opacity[woffset + 1].xy;
	float2 rm = u_opacity[woffset + 2].xy, rt = u_opacity[woffset + 2].zw;
	float k = (lt.y - lm.y) / (lt.x - lm.x);
	if (alpha < lt.x) alpha *= k * (alpha - lm.x) + lm.y;
	else if (alpha < rt.x) alpha *= rt.y;
	else alpha *= -k * (alpha - rm.x) + rm.y;
	return alpha;
}
int getMaskBit(uint mask_value) {
	//check body
	if (mask_value == uint(0)) return ((u_maskbits & uint(1)) == uint(1)) ? 0 : -1;

	int CHECK_BIT = int(-1);
	//check if organ
	for (uint i = uint(0); i < u_organ_num; i++) {
		if (((u_maskbits >> uint(i + uint(1))) & uint(1)) == uint(0)) continue;
		uint cbit = (mask_value >> i) & uint(1);
		if (cbit == uint(1)) {
			CHECK_BIT = int(i) + 1;
			break;
		}
	}
	return CHECK_BIT;
}

float3 TransferColor(float intensity, int ORGAN_BIT) {
	float3 color = intensity;

	if (u_color_scheme > 0)
		color = hex2rgb(COLOR_SCHEME_HEX[u_color_scheme - 1][int(intensity * 255.0)]);
	if (u_show_organ && u_mask_recolor && ORGAN_BIT > int(0)) color = transfer_scheme(ORGAN_BIT, intensity);

	return AdjustContrastBrightness(color);
}


[numthreads(8, 8, 8)]
void main(uint3 threadID : SV_DispatchThreadID) {
	uint value = srcVolume[threadID].r;
	//organ mask
	uint u_mask = value >> uint(16);
	int ORGAN_BIT = -1;
	if (u_show_organ) {
		ORGAN_BIT = getMaskBit(u_mask);
		if (ORGAN_BIT < 0) { destVolume[threadID] = .0f; return; }
	}

	uint u_intensity = value & uint(0xffff);
	float intensity_01 = clamp(float(u_intensity) * 0.0002442002442002442 + u_base_value - 0.5, .0, 1.0);
	float alpha = .0f;
	for (int i = 0; i < u_widget_num; i++)
		if (((u_visible_bits >> i) & 1) == 1) alpha = max(alpha, UpdateOpacityAlpha(3 * i, intensity_01));
	destVolume[threadID] = float4(TransferColor(intensity_01, ORGAN_BIT), alpha);
}