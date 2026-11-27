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
	float4 min_co;
	float4 max_co;
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

static bool get_aabb_hit(inout ray_t ray, int side, float dist, uint id, int3 map_co, AABB_t hitbox) {
	float3 box_min = float3(map_co) + hitbox.min_co.xyz;
	float3 box_max = float3(map_co) + hitbox.max_co.xyz;

	float3 origin = cam.pos.xyz;
	float3 dir = float3(ray.dir_x, ray.dir_y, ray.dir_z);

	float3 inv_dir = 1.0 / dir;

	float3 t1 = (box_min - origin) * inv_dir;
	float3 t2 = (box_max - origin) * inv_dir;

	float3 t_min = min(t1, t2);
	float3 t_max = max(t1, t2);

	float t_enter = max(max(t_min.x, t_min.y), t_min.z);
	float t_exit  = min(min(t_max.x, t_max.y), t_max.z);

	if (t_enter > t_exit || t_exit < 0.0)
		return false;

	float hit_dist = (t_enter >= 0.0) ? t_enter : t_exit;

	if (hit_dist < dist - EPSILON)
		return false;

	float3 normal;

	if (t_enter >= 0.0) {
		if (t_min.x >= t_min.y && t_min.x >= t_min.z)
			normal = float3((ray.dir_x > 0) ? (-1) : (1), 0, 0);
		else if (t_min.y >= t_min.z)
			normal = float3(0, (ray.dir_y > 0) ? (-1) : (1), 0);
		else
			normal = float3(0, 0, (ray.dir_z > 0) ? (-1) : (1));
	}
	else {
		if (t_max.x <= t_max.y && t_max.x <= t_max.z)
			normal = float3(ray.dir_x > 0 ? 1 : -1, 0, 0);
		else if (t_max.y <= t_max.z)
			normal = float3(0, ray.dir_y > 0 ? 1 : -1, 0);
		else
			normal = float3(0, 0, ray.dir_z > 0 ? 1 : -1);
	}

	float3 hit_pos = origin + dir * hit_dist;
	float3 local = hit_pos - float3(map_co);

	if (abs(normal.x) > 0.5) {
		ray.x_percent = local.z;
		ray.y_percent = local.y;
		ray.side = 0;

		if (normal.x < 0)
			ray.x_percent = 1.0 - ray.x_percent;
	}
	else if (abs(normal.y) > 0.5) {
		ray.x_percent = local.x;
		ray.y_percent = local.z;
		ray.side = 1;

		if (normal.y < 0) {
			ray.x_percent = 1.0 - ray.x_percent;
			ray.y_percent = 1.0 - ray.y_percent;
		}
	}
	else {
		ray.x_percent = local.x;
		ray.y_percent = local.y;
		ray.side = 2;

		if (normal.z > 0)
			ray.x_percent = 1.0 - ray.x_percent;
	}

	ray.has_hit = 1;
	ray.dist = hit_dist;
	ray.id = id;

	return true;
}


static void get_full_hit(inout ray_t ray, int side, float dist, uint id, int3 map_co) {
	if (side == 0) {
		ray.x_percent = cam.pos.z + ray.dir_z * dist - map_co.z;
		ray.y_percent = cam.pos.y + ray.dir_y * dist - map_co.y;
		if (ray.dir_x < 0)
			ray.x_percent = 1 - ray.x_percent;
		// hit  west east
	}
	else if (side == 1) {
		ray.x_percent = cam.pos.x + ray.dir_x * dist - map_co.x;
		ray.y_percent = cam.pos.z + ray.dir_z * dist - map_co.z;
		if (ray.dir_y < 0) {
			ray.x_percent = 1 - ray.x_percent;
			ray.y_percent = 1 - ray.y_percent;
		}
		// hit up bot
	}
	else {
		ray.x_percent = cam.pos.x + ray.dir_x * dist - map_co.x;
		ray.y_percent = cam.pos.y + ray.dir_y * dist - map_co.y;
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

void rotate_AABB_Y(inout AABB_t aabb, int angle) {
    float3 center = (aabb.min_co.xyz + aabb.max_co.xyz) * 0.5;
    float3 half_size = (aabb.max_co.xyz - aabb.min_co.xyz) * 0.5;

    float3 offset = center - 0.5;

    if (angle == 1)
    {
        center.x = 0.5 + offset.z;
        center.z = 0.5 - offset.x;
        half_size.xz = half_size.zx;
    }
    else if (angle == 2)
    {
        center.xz = 0.5 - offset.xz;
    }
    else if (angle == 3)
    {
        center.x = 0.5 - offset.z;
        center.z = 0.5 + offset.x;
        half_size.xz = half_size.zx;
    }

    aabb.min_co.xyz = center - half_size;
    aabb.max_co.xyz = center + half_size;
}

void flip_up_aabb(inout AABB_t aabb) {
	float4 old_min = aabb.min_co;
	aabb.min_co = 1 - aabb.max_co;
	aabb.max_co = 1 - old_min;
}

static bool collide(inout ray_t ray, int side, float dist, uint id, int map_x, int map_y, int map_z) {
	int3 map_co = int3(map_x, map_y, map_z);
	AABB_t hitbox;

	switch (id) {
		default:
			get_full_hit(ray, side, dist, id, map_co);
			return true;
		case 5:
			hitbox.min_co = float4(0, 0, 0, 0);
			hitbox.max_co = float4(0.5, 0.5, 0.5, 0);
			flip_up_aabb(hitbox);
			return get_aabb_hit(ray, side, dist, id, map_co, hitbox);
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
	while (dist < render_distance) {
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
		if (ray.id == 5) {
			if (ray.x_percent < 0.5 && ray.y_percent < 0.5 || ray.x_percent > 0.5 && ray.y_percent > 0.5)
				outImage[uint2(px)] = float4(0, 1, 1, 1.0);
			else
				outImage[uint2(px)] = float4(1, 0, 1, 1.0);
				
			return ;
		}
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