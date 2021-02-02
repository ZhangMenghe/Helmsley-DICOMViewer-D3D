#include "StructAndConsts.hlsli"
RWTexture2D<float4> destTex : register(u0);

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

float3 get_color(float xpos) {
    float3 color = xpos;

    if (u_color_scheme > 0)
        color = hex2rgb(COLOR_SCHEME_HEX[u_color_scheme - 1][int(xpos * 255.0)]);

    return AdjustContrastBrightness(color);
}

[numthreads(8, 1, 1)]
void main( uint3 threadID : SV_DispatchThreadID){
	float xpos = float(threadID.x) / colorbar_tex_width;
	uint unit_size = uint(colorbar_tex_height / 3.0);

    //intensity
    float gray = .0;
    for (int i = 0; i < u_widget_num; i++) {
        if (((u_visible_bits >> i) & 1) == 1) gray = max(gray, get_intensity(6 * i, xpos));
    }
    for (uint y = uint(0); y < unit_size; y++) {
        uint2 storePos = uint2(threadID.x, y);
		destTex[storePos] = gray;
    }

    //color scheme
    float4 color = float4(get_color(xpos), 1.0);
    for (uint y = uint(2) * unit_size; y < uint(u_tex_size.y); y++) {
        uint2 storePos = uint2(threadID.x, y);
        destTex[storePos] = color;
    }

    //mixture
    for (uint y = unit_size; y < uint(2) * unit_size; y++) {
        uint2 storePos = uint2(threadID.x, y);
        destTex[storePos] = float4(color.rgb, gray);
    }
}