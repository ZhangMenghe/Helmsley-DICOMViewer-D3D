Texture3D shaderTexture;
SamplerState uSampler;

struct v2f {
	float4 pos : SV_POSITION;
	float3 tex :TEXCOORD0;
	float3 ro : TEXCOORD1;
	float3 FragPos: TEXCOORD2;
	float3 raydir: TEXCOORD3;
};
float2 RayCube(float3 ro, float3 rd, float3 extents) {
	float3 aabb[2] = { -extents, extents };
	float3 ird = 1.0 / rd;
	int3 sign = int3(rd.x < 0 ? 1 : 0, rd.y < 0 ? 1 : 0, rd.z < 0 ? 1 : 0);

	float tmin, tmax, tymin, tymax, tzmin, tzmax;
	tmin = (aabb[sign.x].x - ro.x) * ird.x;
	tmax = (aabb[1 - sign.x].x - ro.x) * ird.x;
	tymin = (aabb[sign.y].y - ro.y) * ird.y;
	tymax = (aabb[1 - sign.y].y - ro.y) * ird.y;
	tzmin = (aabb[sign.z].z - ro.z) * ird.z;
	tzmax = (aabb[1 - sign.z].z - ro.z) * ird.z;
	tmin = max(max(tmin, tymin), tzmin);
	tmax = min(min(tmax, tymax), tzmax);

	return float2(tmin, tmax);
}
float RayPlane(float3 ro, float3 rd, float3 planep, float3 planen) {
	float d = dot(planen, rd);
	float t = dot(planep - ro, planen);
	return d > 1e-5 ? (t / d) : (t > 0 ? 1e5 : -1e5);
}
float4 Sample(float3 p) {
	float3 cp = clamp(p, .01f, 0.99f);
	cp.y = 1.0 - cp.y;
	return shaderTexture.Sample(uSampler, cp);
}
float4 subDivide(float3 p, float3 ro, float3 rd, float t, float StepSize) {
	float t0 = t - StepSize * 4.0;
	float t1 = t;
	float tm;
#define BINARY_SUBDIV tm = (t0 + t1) * .5; p = ro + rd * tm; if (Sample(p).a > .01) t1 = tm; else t0 = tm;
	BINARY_SUBDIV
		BINARY_SUBDIV
		BINARY_SUBDIV
		BINARY_SUBDIV
#undef BINARY_SUBDIV
		t = tm;
	return Sample(p);
}
float4 Volume(float3 ro, float3 rd, float head, float tail) {
	float4 sum = .0;
	float pd = .0;
	uint steps = 0;
	float usample_step_inverse = 0.003f;

	
	for (float t = head; t < tail && steps<128; steps++ ) {
		if (sum.a >= 0.98f) break;
		float3 p = ro + rd * t;
		float4 val_color = Sample(p);
		if (val_color.a > 0.01) {
			if (pd < 0.01) {
			
				float t0 = t - usample_step_inverse * 3;
				float t1 = t;
				float tm;
#define BINARY_SUBDIV tm = (t0 + t1) * .5; p = ro + rd * tm; if (Sample(p).a) t1 = tm; else t0 = tm;
				BINARY_SUBDIV
					BINARY_SUBDIV
					BINARY_SUBDIV
#undef BINARY_SUBDIV
					t = tm;
				val_color = Sample(p);
			}
			/*sum.rgb += (1.0 - sum.a) * val_color.a * val_color.rgb;
			sum.a += (1.0 - sum.a) * val_color.a;*/
			
			val_color.rgb *= val_color.a;
			sum += val_color * (1 - sum.a);
		}
		t += val_color.a > 0.01 ? usample_step_inverse : usample_step_inverse * 4.0;
		pd = sum.a;
	}
	return float4(sum.rgb, saturate(sum.a));
}
// A pass-through function for the (interpolated) color data.
float4 main(v2f input) : SV_TARGET{
	//return float4(input.tex, 1.0);
	//input.tex = clamp(input.tex, .01f, 0.99f);
	//return shaderTexture.Sample(uSampler, input.tex);

	float3 ro = input.ro;
	float3 rd = normalize(input.raydir);
	
	float2 intersect = RayCube(ro, rd, .5);
	intersect.x = max(0, intersect.x);

	clip(intersect.y - intersect.x);
	return Volume(ro + 0.5, rd, intersect.x, intersect.y);
}
