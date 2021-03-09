struct Vertex{
    float3 position;
    float3 normal;
};

Texture3D<uint> srcVolume : register(t0);
//Buffer<int> quad_indices  : register(t1);

Buffer<int> triangle_table  : register(t1);
Buffer<int> medge_table   : register(t2);

RWStructuredBuffer<Vertex> output_vertices : register(u0);

cbuffer mcConstantBuffer : register(b0) {
	uint4 u_grid_size;
	uint4 u_volume_size;
	//mask
	uint u_maskbits;
	uint u_organ_num;
};

//static const float quad_vertices_2d[] = {
//		-0.5f, 0.5, .0,.0,
//		0.5,0.5, .0,.0,
//		0.5,-0.5, .0,1.0,
//		-0.5,-0.5, .0,1.0,
//};
//static const int quad_indices[] = {
//	0,1,3,1,2,3
//};

bool check_mask_bit(uint value) {
	for (uint i = uint(1); i < u_organ_num; i++) {
		if (((u_maskbits >> uint(i + uint(1))) & uint(1)) == uint(0)) continue;
		uint cbit = (value >> i) & uint(1);
		if (cbit == uint(1)) return true;
	}
	return false;
}
float map(uint3 p) {
	uint sc = srcVolume[p].r;
	sc = sc >> uint(16);
	return check_mask_bit(sc) ? -float(sc) : 1.0;
}

float3 CalculateGradient(uint3 p) {
	uint2 e = uint2(1, 0);

	float3 n = float3(map(p + e.xyy) - map(p - e.xyy),  // Gradient x
		map(p + e.yxy) - map(p - e.yxy),  // Gradient y
		map(p + e.yyx) - map(p - e.yyx)); // Gradient z

	return normalize(n);
}

float3 calcNormal(float3 v0, float3 v1, float3 v2)
{
	float3 edge0 = v1 - v0;
	float3 edge1 = v2 - v0;
	return normalize(cross(edge0, edge1));
}

Vertex find_vertex(float isolevel, uint3 p1, uint3 p2, float value_1, float value_2) {
	float3 n1 = CalculateGradient(p1);
	float3 n2 = CalculateGradient(p2);

	const float eps = 0.00001f;
	Vertex res;

	if (abs(isolevel - value_1) < eps)
	{
		res.position = float3(p1);
		res.normal = n1;
		return res;
	}
	if (abs(isolevel - value_2) < eps)
	{
		res.position = float3(p2);
		res.normal = n2;
		return res;
	}
	if (abs(value_1 - value_2) < eps)
	{
		res.position = float3(p1);
		res.normal = n1;
		return res;
	}

	//res.position = float3(p1);
	//res.normal = n1;
	//return res;

	float mu = (isolevel - value_1) / (value_2 - value_1);

	res.position = lerp(float3(p1), float3(p2), mu);//float3(p1) + mu * (float3(p2 - p1));

	res.normal = normalize(n1); //normalize(lerp(float3(n1), float3(n2), mu));//normalize(n1 + mu * (n2 - n1));
	return res;
}


//void main_debug(uint3 threadID : SV_DispatchThreadID) {
//	int id = 4 * quad_indices[threadID.x + 6];
//	output_vertices[threadID.x].position = float3(
//		quad_vertices_2d[id],
//		quad_vertices_2d[id+1],
//		quad_vertices_2d[id+2]);
//	output_vertices[threadID.x].normal = .0f;
//}

[numthreads(8,8,8)]
void main(uint3 threadID : SV_DispatchThreadID){
	float3 grid_size = float3(u_grid_size.xyz);
	float3 inv_grid_size = float3(1.0f / grid_size.x, 1.0f / grid_size.y, 1.0f / grid_size.z);

	float3 volume_size = float3(u_volume_size.xyz);
	float3 inv_volume_size = float3(1.0f / volume_size.x, 1.0f / volume_size.y, 1.0f / volume_size.z);

	uint3 cell_index = uint3(
		uint(float(threadID.x) * inv_grid_size.x * volume_size.x),
		uint(float(threadID.y) * inv_grid_size.y * volume_size.y),
		uint(float(threadID.z) * inv_grid_size.z * volume_size.z));

	// Avoid sampling outside of the volume bounds
	if (cell_index.x == 0 ||
		cell_index.y == (u_volume_size.y - 1) ||
		cell_index.z == (u_volume_size.z - 1)) return;

	uint3 neighbors[8] = {
		threadID,
		threadID + uint3(0, 0, 1),
		threadID + uint3(-1, 0, 1),
		threadID + uint3(-1, 0, 0),
	
		threadID + uint3(0, 1, 0),
		threadID + uint3(0, 1, 1),
		threadID + uint3(-1, 1, 1),
		threadID + uint3(-1, 1, 0)
	};

	uint2 edge_table[12] = {
		uint2(0, 1), uint2(1, 2), uint2(2, 3), uint2(3, 0), uint2(4, 5), uint2(5, 6),
		uint2(6, 7), uint2(7, 4), uint2(0, 4), uint2(1, 5), uint2(2, 6), uint2(3, 7) 
	};

	// Calculate which of the 256 configurations this cell is 
	float values[8];
	float3 pos[8];
	uint3 pmn[8];
	int configuration = 0;
	for (int i = 0; i < 8; ++i) {
		pos[i] = float3(
			float(neighbors[i].x) * inv_grid_size.x,
			float(neighbors[i].y) * inv_grid_size.y,
			float(neighbors[i].z) * inv_grid_size.z);
		pmn[i] = uint3(
			uint(pos[i].x * volume_size.x),
			uint(pos[i].y * volume_size.y),
			uint(pos[i].z * volume_size.z));
		values[i] = map(pmn[i]);

		if (values[i] < .0) configuration |= 1 << i;
	}

	Vertex vertex_list[12];
	if (medge_table[configuration] != 0)
	{
		for (int i = 0; i < 12; ++i)
		{
			if (int(medge_table[configuration] & (1 << i)) != 0)
			{
				uint2 edge = edge_table[i];
				vertex_list[i] = find_vertex(1.0f, pmn[edge.x], pmn[edge.y], values[edge.x], values[edge.y]);
				vertex_list[i].position = vertex_list[i].position * inv_volume_size - 0.5;//;vertex_list[i].position * inv_volume_size - 0.5;// *inv_volume_size - 0.5;
				
				//vertex_list[i].position = clamp(vertex_list[i].position * inv_volume_size - 0.5, -0.5, 0.5);
			}
		}
	}

	// Construct triangles based on this cell's configuration and the vertices calculated above
	int cell_start_memory = (threadID.x +
		threadID.y * u_grid_size.x +
		threadID.z * u_grid_size.x * u_grid_size.y) * 15;
	int triangle_start_memory = configuration * 16; // 16 = the size of each "row" in the triangle table

	for (int i = 0; i < 5; ++i)
	{
		int idx = cell_start_memory + 3 * i;

		if (triangle_table[triangle_start_memory + 3 * i].r != -1 && triangle_table[triangle_start_memory + 3 * i + 1].r != -1 && triangle_table[triangle_start_memory + 3 * i + 2].r != -1){

		  Vertex v0 = vertex_list[triangle_table[triangle_start_memory + 3 * i].r];
			Vertex v1 = vertex_list[triangle_table[triangle_start_memory + 3 * i + 1].r];
			Vertex v2 = vertex_list[triangle_table[triangle_start_memory + 3 * i + 2].r];

			output_vertices[idx].position = v0.position;
			//output_vertices[idx].normal = v0.normal;

			output_vertices[idx + 1].position = v1.position;
			//output_vertices[idx + 1].normal = v1.normal;

			output_vertices[idx + 2].position = v2.position;
			//output_vertices[idx + 2].normal = v2.normal;

			output_vertices[idx].normal = calcNormal(v0.position, v1.position, v2.position);
			output_vertices[idx + 1].normal = calcNormal(v1.position, v2.position, v0.position);
			output_vertices[idx + 2].normal = calcNormal(v2.position, v0.position, v1.position);
		}

	}
    //output_vertices[threadID.x].pos = .0f;// inputConstantParticleData[dispatchThreadID.x].position;
}