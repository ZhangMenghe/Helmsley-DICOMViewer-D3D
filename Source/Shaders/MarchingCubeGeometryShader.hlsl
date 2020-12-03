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
static int const_edge_table[256] = {
	0x0  , 0x109, 0x203, 0x30a, 0x406, 0x50f, 0x605, 0x70c,
	0x80c, 0x905, 0xa0f, 0xb06, 0xc0a, 0xd03, 0xe09, 0xf00,
	0x190, 0x99 , 0x393, 0x29a, 0x596, 0x49f, 0x795, 0x69c,
	0x99c, 0x895, 0xb9f, 0xa96, 0xd9a, 0xc93, 0xf99, 0xe90,
	0x230, 0x339, 0x33 , 0x13a, 0x636, 0x73f, 0x435, 0x53c,
	0xa3c, 0xb35, 0x83f, 0x936, 0xe3a, 0xf33, 0xc39, 0xd30,
	0x3a0, 0x2a9, 0x1a3, 0xaa , 0x7a6, 0x6af, 0x5a5, 0x4ac,
	0xbac, 0xaa5, 0x9af, 0x8a6, 0xfaa, 0xea3, 0xda9, 0xca0,
	0x460, 0x569, 0x663, 0x76a, 0x66 , 0x16f, 0x265, 0x36c,
	0xc6c, 0xd65, 0xe6f, 0xf66, 0x86a, 0x963, 0xa69, 0xb60,
	0x5f0, 0x4f9, 0x7f3, 0x6fa, 0x1f6, 0xff , 0x3f5, 0x2fc,
	0xdfc, 0xcf5, 0xfff, 0xef6, 0x9fa, 0x8f3, 0xbf9, 0xaf0,
	0x650, 0x759, 0x453, 0x55a, 0x256, 0x35f, 0x55 , 0x15c,
	0xe5c, 0xf55, 0xc5f, 0xd56, 0xa5a, 0xb53, 0x859, 0x950,
	0x7c0, 0x6c9, 0x5c3, 0x4ca, 0x3c6, 0x2cf, 0x1c5, 0xcc ,
	0xfcc, 0xec5, 0xdcf, 0xcc6, 0xbca, 0xac3, 0x9c9, 0x8c0,
	0x8c0, 0x9c9, 0xac3, 0xbca, 0xcc6, 0xdcf, 0xec5, 0xfcc,
	0xcc , 0x1c5, 0x2cf, 0x3c6, 0x4ca, 0x5c3, 0x6c9, 0x7c0,
	0x950, 0x859, 0xb53, 0xa5a, 0xd56, 0xc5f, 0xf55, 0xe5c,
	0x15c, 0x55 , 0x35f, 0x256, 0x55a, 0x453, 0x759, 0x650,
	0xaf0, 0xbf9, 0x8f3, 0x9fa, 0xef6, 0xfff, 0xcf5, 0xdfc,
	0x2fc, 0x3f5, 0xff , 0x1f6, 0x6fa, 0x7f3, 0x4f9, 0x5f0,
	0xb60, 0xa69, 0x963, 0x86a, 0xf66, 0xe6f, 0xd65, 0xc6c,
	0x36c, 0x265, 0x16f, 0x66 , 0x76a, 0x663, 0x569, 0x460,
	0xca0, 0xda9, 0xea3, 0xfaa, 0x8a6, 0x9af, 0xaa5, 0xbac,
	0x4ac, 0x5a5, 0x6af, 0x7a6, 0xaa , 0x1a3, 0x2a9, 0x3a0,
	0xd30, 0xc39, 0xf33, 0xe3a, 0x936, 0x83f, 0xb35, 0xa3c,
	0x53c, 0x435, 0x73f, 0x636, 0x13a, 0x33 , 0x339, 0x230,
	0xe90, 0xf99, 0xc93, 0xd9a, 0xa96, 0xb9f, 0x895, 0x99c,
	0x69c, 0x795, 0x49f, 0x596, 0x29a, 0x393, 0x99 , 0x190,
	0xf00, 0xe09, 0xd03, 0xc0a, 0xb06, 0xa0f, 0x905, 0x80c,
	0x70c, 0x605, 0x50f, 0x406, 0x30a, 0x203, 0x109, 0x0 };

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
	uint3 e = uint3(1, 0, 0);

	float3 n = float3(map(p + e.xyy) - map(p - e.xyy),  // Gradient x
		map(p + e.yxy) - map(p - e.yxy),  // Gradient y
		map(p + e.yyx) - map(p - e.yyx)); // Gradient z

	return normalize(n);
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

	float mu = (isolevel - value_1) / (value_2 - value_1);

	res.position = float3(p1) + mu * (float3(p2 - p1));
	res.normal = normalize(n1 + mu * (n2 - n1));
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
	if (const_edge_table[configuration] != 0)
	{
		for (int i = 0; i < 12; ++i)
		{
			if (int(const_edge_table[configuration] & (1 << i)) != 0)
			{
				uint2 edge = edge_table[i];
				vertex_list[i] = find_vertex(0.0, pmn[edge.x], pmn[edge.y], values[edge.x], values[edge.y]);
				vertex_list[i].position = clamp(vertex_list[i].position * inv_volume_size - 0.5, -0.5, 0.5);
			}
		}
	}

	// Construct triangles based on this cell's configuration and the vertices calculated above
	int cell_start_memory = int(float(threadID.x) +
		float(threadID.y) * grid_size.x +
		float(threadID.z) * grid_size.x * grid_size.y) * 15;
	int triangle_start_memory = configuration * 16; // 16 = the size of each "row" in the triangle table

	for (int i = 0; i < 5; ++i)
	{
		int idx = cell_start_memory + 3 * i;

		if (triangle_table[triangle_start_memory + 3 * i].r != -1){
			Vertex v0 = vertex_list[triangle_table[triangle_start_memory + 3 * i].r];

			output_vertices[idx].position = v0.position;
			output_vertices[idx].normal = v0.normal;

			Vertex v1 = vertex_list[triangle_table[triangle_start_memory + 3 * i + 1].r];
			output_vertices[idx + 1].position = v1.position;
			output_vertices[idx + 1].normal = v1.normal;

			Vertex v2 = vertex_list[triangle_table[triangle_start_memory + 3 * i + 2].r];
			output_vertices[idx + 2].position = v2.position;
			output_vertices[idx + 1].normal = v2.normal;
		}
		else{
			output_vertices[idx].position = .0f;
			output_vertices[idx].normal = .0f;
			output_vertices[idx+1].position = .0f;
			output_vertices[idx+1].normal = .0f;
			output_vertices[idx+2].position = .0f;
			output_vertices[idx+2].normal = .0f;
		}
	}
    //output_vertices[threadID.x].pos = .0f;// inputConstantParticleData[dispatchThreadID.x].position;
}