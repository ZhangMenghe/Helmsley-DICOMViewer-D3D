Texture3D<uint> srcVolume : register(t0);
RWTexture3D<float4> destVolume : register(u0);

cbuffer volumeSetupConstBuffer : register(b0){
	uint4 u_tex_size;

	//opacity widget
	float4 u_opacity[30];
	int u_widget_num;
	int u_visible_bits;
	
	//contrast
	float u_contrast_low;
	float u_contrast_high;
	float u_brightness;

	//mask
	uint u_maskbits;
	uint u_organ_num;
	bool u_mask_recolor;
	
	//others
	bool u_show_organ;
	uint u_color_scheme;//COLOR_GRAYSCALE COLOR_HSV COLOR_BRIGHT
};
const static float3 ORGAN_COLORS[7] = {
	float3(0.24, 0.004, 0.64), float3(0.008, 0.278, 0.99), float3(0.75, 0.634, 0.996),
	float3(1, 0.87, 0.14), float3(0.98, 0.88, 1.0), float3(0.99, 0.106, 0.365), float3(.0, 0.314, 0.75) };

// All components are in the range [0?], including hue.
float3 hsv2rgb(float3 c) {
	float4 K = float4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
	float3 p = abs(frac(c.xxx + K.xyz) * 6.0 - K.www);
	return lerp(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y) * c.z;
}
float3 transfer_scheme(float gray) {
	return hsv2rgb(float3(gray, 1.0, 1.0));
}
float3 transfer_scheme(int cat, float gray) {
	return ORGAN_COLORS[cat - 1] * gray;
}
//hot to color. H(0~180)
float3 bright_scheme(float gray) {
	return hsv2rgb(float3((1.0 - gray) * 180.0 / 255.0, 1.0, 1.0));
}
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

//applied contrast, brightness, 12bit->8bit, return value 0-1
float TransferIntensityStepOne(float intensity) {
	intensity = (intensity - u_contrast_low) / (u_contrast_high - u_contrast_low);
	intensity = max(.0, min(1.0, intensity));
	intensity = clamp(intensity + u_brightness * 2.0 - 1.0, .0, 1.0);
	return intensity;
	////max value 4095
	//float intensity_01 = float(intensity) * 0.0002442002442002442;
	//if (intensity_01 > u_contrast_high || intensity_01 < u_contrast_low) intensity_01 = .0;
	//intensity_01 = smoothstep(u_contrast_low, u_contrast_high, intensity_01);
	////    intensity_01 = (intensity_01 - u_contrast_low) / (u_contrast_high - u_contrast_low) * u_contrast_level;
	//intensity_01 = clamp(u_brightness + intensity_01 - 0.5, .0, 1.0);
	//return intensity_01;
}

float3 TransferColor(float intensity, int ORGAN_BIT) {
	intensity = smoothstep(u_contrast_low, u_contrast_high, intensity);
	intensity = max(.0, min(1.0f, intensity));

	float3 color = intensity;
	
	if(u_color_scheme==1)color = transfer_scheme(intensity);
	else if(u_color_scheme==2) color = bright_scheme(intensity);
	if (u_show_organ && u_mask_recolor && ORGAN_BIT > int(0)) color = transfer_scheme(ORGAN_BIT, intensity);
	
	return color;
}

[numthreads(8,8,8)]
void main(uint3 threadID : SV_DispatchThreadID){
	uint value = srcVolume[threadID].r;
	//mask
	uint u_mask = value >> uint(16);
	int ORGAN_BIT = -1;
	if (u_show_organ) {
		ORGAN_BIT = getMaskBit(u_mask);
		if (ORGAN_BIT < 0) { destVolume[threadID] = .0f; return; }
	}

	uint u_intensity = value & uint(0xffff);
	float intensity_01 = float(value.x) * 0.0002442002442002442;

	float alpha = .0f;
	for (int i = 0; i < u_widget_num; i++)
		if (((u_visible_bits >> i) & 1) == 1) alpha = max(alpha, UpdateOpacityAlpha(3 * i, intensity_01));
	destVolume[threadID] = float4(TransferColor(TransferIntensityStepOne(intensity_01), ORGAN_BIT), alpha);
}