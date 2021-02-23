Texture3D shaderTexture;
SamplerState uSampler;

struct v2f {
	float4 pos : SV_POSITION;
	float3 tex :TEXCOORD0;
	float3 ro : TEXCOORD1;
	//float3 FragPos: TEXCOORD2;
	float3 raydir: TEXCOORD2;
};

cbuffer raypixConstantBuffer : register(b0) {
	bool u_cut : packoffset(c0);
	bool u_cutplane_realsample : packoffset(c1);
	float4 u_pp: packoffset(c2);
	float4 u_pn: packoffset(c3);
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
	float usample_step_inverse = 0.005f;
	float high_bound = 0.01;
	float step_size = usample_step_inverse;
	bool last_succeeded = true;

	for (float t = head; t < tail && steps<128; steps++ ) {
		if (sum.a >= 0.98f) break;
		float3 p = ro + rd * t;
		float4 val_color = Sample(p);
		if (val_color.a > 0.01) {
			pd = (1.0 - sum.a) * val_color.a;
			if (pd > high_bound) {
				step_size /= 2.0; high_bound = min(high_bound * 2.0, 1.0); last_succeeded = false;
			}
			else {
				sum.rgb += pd * val_color.rgb;
				sum.a += pd;
				t += step_size;
				if (last_succeeded) { step_size *= 2.0; high_bound /= 2.0; }
				else last_succeeded = true;
			}
		}
		else {
			t += 4.0 * usample_step_inverse;
		}
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
	bool blocked_by_plane = false;
	if (u_cut) {
		float t;
		if (dot(u_pn.xyz, -ro) > .0f) {
			t = RayPlane(ro, rd, u_pp.xyz, u_pn.xyz);
			blocked_by_plane = (t <= intersect.x);
			intersect.x = max(intersect.x, t);
		}else {
			t = RayPlane(ro, rd, u_pp.xyz, -u_pn.xyz); intersect.y = min(intersect.y, t);
		}
		if (blocked_by_plane && intersect.x <= intersect.y) {
			float4 traced_color = Volume(ro + 0.5, rd, intersect.x, intersect.y);
			return lerp(traced_color, float4(.0f, .0f, .0f, 1.0f), 1.0 - traced_color.a);
		}
		if (u_cutplane_realsample) {
			clip(blocked_by_plane ? -1.0f : 1.0f);
			clip(intersect.y - intersect.x);
			return Sample(ro + 0.5 + rd * t);
		}
	}
	clip(blocked_by_plane ? -1.0f : 1.0f);
	clip(intersect.y - intersect.x);
	return Volume(ro + 0.5, rd, intersect.x, intersect.y);
}
