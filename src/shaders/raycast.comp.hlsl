#ifndef WORLD_W
#define WORLD_W 2
#endif
#ifndef WORLD_H
#define WORLD_H 2
#endif

#define TILE_SIZE 16
#define EPSILON (0.000001f)
#define INF (1e30f)

static const int3 WORLD_SIZE = int3(WORLD_W * 16, WORLD_H * 16, WORLD_W * 16);

struct Cam {
	float4 pos;
	float4 fwd;
	float4 right;
	float4 up;
	float2 A;
	float2 B;
};

struct ray_t {
	float dir_x;
	float dir_y;
	float dir_z;
	float x_percent;
	float y_percent;
	float dist;
	int has_hit;
	int side;
	uint id;
};

struct AABB_t {
	float3 min_co;
	float3 max_co;
};

// t = read only: space0
// u = write only: space1
// b: uniform: space2

// !TODO maybe add ifdef for the vks

[[vk::binding(0, 0)]]
Texture2D<float4> atlas : register(t0, space0);

[[vk::binding(1, 0)]]
StructuredBuffer<uint> blocks : register(t1, space0);

[[vk::binding(0, 1), vk::image_format("rgba8")]]
RWTexture2D<float4> outImage : register(u0, space1);

[[vk::binding(0, 2)]]
ConstantBuffer<Cam> cam : register(b0, space2);

uint get_block_at(int3 c) {
	int3 ch = c >> 4;
	if (ch.x >= WORLD_W || ch.y >= WORLD_H || ch.z >= WORLD_W || ch.x < 0 || ch.y < 0 || ch.z < 0)
		return 0;
	int3 l = c & 15;

	int chunkIdx = ch.y * WORLD_W * WORLD_W + ch.z * WORLD_W + ch.x;
	int local = (l.y << 8) | (l.z << 4) | l.x;
	return blocks[(chunkIdx << 12) + local];
}

static void get_full_hit(inout ray_t ray, int side, float dist, uint id, int map_x, int map_y, int map_z) {
	if (side == 0) {
		ray.x_percent = cam.pos.z + ray.dir_z * dist - map_z;
		ray.y_percent = cam.pos.y + ray.dir_y * dist - map_y;
		if (ray.dir_x < 0)
			ray.x_percent = 1 - ray.x_percent;
		// hit  west east
	}
	else if (side == 1) {
		ray.x_percent = cam.pos.x + ray.dir_x * dist - map_x;
		ray.y_percent = cam.pos.z + ray.dir_z * dist - map_z;
		if (ray.dir_y < 0) {
			ray.x_percent = 1 - ray.x_percent;
			ray.y_percent = 1 - ray.y_percent;
		}
		// hit up bot
	}
	else {
		ray.x_percent = cam.pos.x + ray.dir_x * dist - map_x;
		ray.y_percent = cam.pos.y + ray.dir_y * dist - map_y;
		if (ray.dir_z > 0)
			ray.x_percent = 1 - ray.x_percent;
		// hit north south
	}
	ray.has_hit = 1;
	ray.dist = dist;
	ray.side = side;
	ray.id = id;
	return;
}


static bool collide(inout ray_t ray, int side, float dist, uint id, int map_x, int map_y, int map_z) {
	switch (id) {
		default:
			get_full_hit(ray, side, dist, id, map_x, map_y, map_z);
			return true;
		case 5:
			if (side == 1)
				return false;
			get_full_hit(ray, side, dist, id, map_x, map_y, map_z);
			return true;
	}
}

static void cast_ray(inout ray_t ray, int map_x, int map_y, int map_z) {
	int step_x = (ray.dir_x > 0) ? (1) : (-1);
	int step_y = (ray.dir_y > 0) ? (1) : (-1);
	int step_z = (ray.dir_z > 0) ? (1) : (-1);

	float dir_x_fabs = abs(ray.dir_x);
	float dir_y_fabs = abs(ray.dir_y);
	float dir_z_fabs = abs(ray.dir_z);

	float delta_x = (dir_x_fabs < EPSILON) ? (INF) : (1 / dir_x_fabs);
	float delta_y = (dir_y_fabs < EPSILON) ? (INF) : (1 / dir_y_fabs);
	float delta_z = (dir_z_fabs < EPSILON) ? (INF) : (1 / dir_z_fabs);

	float max_x = (ray.dir_x < 0) ? (cam.pos.x - map_x) : (map_x + 1 - cam.pos.x);
	max_x *= delta_x;

	float max_y = (ray.dir_y < 0) ? (cam.pos.y - map_y) : (map_y + 1 - cam.pos.y);
	max_y *= delta_y;

	float max_z = (ray.dir_z < 0) ? (cam.pos.z - map_z) : (map_z + 1 - cam.pos.z);
	max_z *= delta_z;

	int side = 0;
	float dist = 0;

	uint render_distance = 125;
	while (true) {
		uint block = get_block_at(int3(map_x, map_y, map_z));
		if (block != 0 && collide(ray, side, dist, block, map_x, map_y, map_z))
				return ;

		if (max_x < max_y && max_x < max_z) {
			dist = max_x;
			max_x += delta_x;
			map_x += step_x;
			side = 0;
		}
		else if (max_y < max_z) {
			dist = max_y;
			max_y += delta_y;
			map_y += step_y;
			side = 1;
		}
		else {
			dist = max_z;
			max_z += delta_z;
			map_z += step_z;
			side = 2;
		}
		if (dist > render_distance)
			break;
		if ((ray.dir_x < 0 && map_x < 0) || (ray.dir_x > 0 && map_x >= WORLD_W * 16))
			break;
		if ((ray.dir_y < 0 && map_y < 0) || (ray.dir_y > 0 && map_y >= WORLD_H * 16))
			break;
		if ((ray.dir_z < 0 && map_z < 0) || (ray.dir_z > 0 && map_z >= WORLD_W * 16))
			break;
	}
}

[numthreads(8, 8, 1)]
void main(uint3 tid : SV_DispatchThreadID)
{
	uint atlas_width;
	uint atlas_height;
	atlas.GetDimensions(atlas_width, atlas_height);
	int tilesX = atlas_width / TILE_SIZE;

	int2 px = int2(tid.xy);

	uint out_width;
	uint out_height;
	outImage.GetDimensions(out_width, out_height);

	if (px.x >= out_width || px.y >= out_height)
		return;

	int map_x = (int)floor(cam.pos.x);
	int map_y = (int)floor(cam.pos.y);
	int map_z = (int)floor(cam.pos.z);

	float sx = cam.A.x * px.x + cam.B.x;
	float sy = cam.A.y * px.y + cam.B.y;

	ray_t ray;
	ray.dir_x = cam.fwd.x + cam.right.x * sx + cam.up.x * sy;
	ray.dir_y = (cam.fwd.y + cam.right.y * sx + cam.up.y * sy);
	ray.dir_z = cam.fwd.z + cam.right.z * sx + cam.up.z * sy;


	ray.has_hit = 0;

	cast_ray(ray, map_x, map_y, map_z);

	if (ray.has_hit == 1) {
		int tile = (int)ray.id - 1;

		int2 tileOrigin = int2((tile % tilesX) * TILE_SIZE, (tile / tilesX) * TILE_SIZE);

		float u = ray.x_percent;
		float v = ray.y_percent;

		int2 t = clamp(int2(float2(u, v) * 16), 0, 16 - 1);

		float4 texel = atlas.Load(int3(tileOrigin + t, 0));
		outImage[uint2(px)] = float4(texel.rgb, 1.0);
	}
	else {
		outImage[uint2(px)] = float4(0.0, 0.0, 0.0, 0.0);
	}
}