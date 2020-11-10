Texture3D<float4> srcVolume : register(t0);
RWTexture3D<float4> destVolume : register(u0);


[numthreads(8,8,8)]
void main(uint3 threadID : SV_DispatchThreadID)
{
	//float4 res = float4(0.392, 0.584, 0.929, 1.f);
	//float3 origin = float3(0, 0, 5);
	//float3 ray = origin;
	//float3 dir = rayDirection(90.f, float2(WIDTH, HEIGHT), float2(threadID.xy));
	//float3 target = normalize(float3(cos(mouse.y / 50.f) * cos(mouse.x / 50.f), sin(mouse.y / 50.f), cos(mouse.y / 50.f) * sin(mouse.x / 50.f)));
	//float4x4 viewToWorld = viewMatrix(origin, origin + target, float3(0, 1, 0));
	//dir = mul(float4(dir, 1.f),viewToWorld).xyz;
	//for (int i = 0; i < 1000; i++)
	//{
	//	float depth = map(ray);
	//	if (depth < EPSILON)
	//	{
	//		//res = float4(threadID / 600.f, 1.f);
	//		//res = float4(estimateNormal(origin), 1.f);
	//		float4 light = float4(calculateLighting(ray, origin, 1.f), 1.f);
	//		res = light;
	//		break;
	//	}
	//	//if (depth < 0.25 && depth > EPSILON)
	//	//	res += 0.015;

	//	ray += dir * depth;
	//}
	//threadID.y = HEIGHT - threadID.y;	//Flip y due to how d3d handles textures
	destVolume[threadID.xyz] = srcVolume[threadID.xyz].r;
		//float4(1.0f, 1.0f, .0f, 1.0f);//res;
	//sampleTexture[threadID.xy] = float4(threadID / 600.f, 1.f);
}