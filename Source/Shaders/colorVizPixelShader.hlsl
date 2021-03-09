struct PixelShaderInput{
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD0;
};
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
	float u_base_value;

	//mask
	uint u_maskbits;
	uint u_organ_num;
	bool u_mask_recolor;
	
	//others
	bool u_show_organ;
	uint u_color_scheme;//COLOR_GRAYSCALE COLOR_HSV COLOR_BRIGHT
};
float3 hsv2rgb(float3 c) {
	float4 K = float4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
	float3 p = abs(frac(c.xxx + K.xyz) * 6.0 - K.www);
	return lerp(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y) * c.z;
}
float3 transfer_scheme_hsv(float gray) {
	if (u_color_scheme == 1) return float3(gray, 1.0, 1.0);
	return float3((1.0 - gray) * 180.0 / 255.0, 1.0, 1.0);
}
float get_intensity(int woffset, float posx) {
	float2 lb = u_opacity[woffset].xy, rb = u_opacity[woffset + 1].zw;
	if (posx < lb.x || posx > rb.x) return .0;
	float2 lm = u_opacity[woffset].zw, lt = u_opacity[woffset + 1].xy;
	float2 rm = u_opacity[woffset + 2].xy, rt = u_opacity[woffset + 2].zw;

	float k = (lt.y - lm.y) / (lt.x - lm.x);
	if (posx < lt.x) return k * (posx - lm.x) + lm.y;
	if (posx < rt.x) return rt.y;
	return -k * (posx - rm.x) + rm.y;
}
float4 get_mixture(float posx, float3 gray) {
	if (u_color_scheme == 0) return gray.r;
	float3 color_hsv = transfer_scheme_hsv(posx);
	return float4(hsv2rgb(color_hsv), gray.r);
}

float4 main(PixelShaderInput input) : SV_TARGET{
	float intensity = input.tex.x; float ypos = 1.0f - input.tex.y;
	if (ypos > 0.66) {
		intensity = smoothstep(u_contrast_low, u_contrast_high, intensity);
		intensity = max(.0, min(1.0, intensity));

		if (u_color_scheme == 0) return float4(intensity, intensity, intensity, 1.0f);
		return float4(hsv2rgb(transfer_scheme_hsv((input.tex.x > u_contrast_high) ? 1.0 : intensity)), 1.0);
	}
	else {
		float gray = .0;
		for (int i = 0; i < u_widget_num; i++) {
			if (((u_visible_bits >> i) & 1) == 1) gray = max(gray, get_intensity(6 * i, intensity));
		}
		if (ypos < 0.33) return float4(gray, gray, gray, 1.0f);
		else {
			intensity = smoothstep(u_contrast_low, u_contrast_high, intensity);
			intensity = max(.0, min(1.0f, intensity));

			return get_mixture(intensity, gray);
		}
	}
}