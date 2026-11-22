#include "client.h"

#include <unistd.h>
#define EPSILON (0.000001f)
#define INF (1e30f)

static inline int block_is_solid(int x, int y, int z, world_t *world) {
	if ((unsigned)x >= WORLD_SIZE || (unsigned)y >= WORLD_SIZE || (unsigned)z >= WORLD_SIZE)
		return 0;
	return world->blocks[z][y][x] != 0;
}

static inline void cast_ray(ray_t *ray, int map_x, int map_y, int map_z, world_t *world) {
	int step_x = (ray->dir_x > 0) ? (1) : (-1);
	int step_y = (ray->dir_y > 0) ? (1) : (-1);
	int step_z = (ray->dir_z > 0) ? (1) : (-1);

	float dir_x_fabs = fabsf(ray->dir_x);
	float dir_y_fabs = fabsf(ray->dir_y);
	float dir_z_fabs = fabsf(ray->dir_z);

	float delta_x = (dir_x_fabs < EPSILON) ? (INF) : (1 / dir_x_fabs);
	float delta_y = (dir_y_fabs < EPSILON) ? (INF) : (1 / dir_y_fabs);
	float delta_z = (dir_z_fabs < EPSILON) ? (INF) : (1 / dir_z_fabs);

	float max_x = (ray->dir_x < 0) ? (render.x - map_x) : (map_x + 1 - render.x);
	max_x *= delta_x;

	float max_y = (ray->dir_y < 0) ? (render.y - map_y) : (map_y + 1 - render.y);
	max_y *= delta_y;

	float max_z = (ray->dir_z < 0) ? (render.z - map_z) : (map_z + 1 - render.z);
	max_z *= delta_z;

	int side = 0;
	float dist = 0;

	size_t render_distance = render.render_distance;
	while (1) {
		if (block_is_solid(map_x, map_y, map_z, world)) {
			if (side == 0) {
			    ray->x_percent = render.z + ray->dir_z * dist - map_z;
			    ray->y_percent = render.y + ray->dir_y * dist - map_y;
				if (ray->dir_x < 0) {
					ray->x_percent = 1 - ray->x_percent;
				}
				// hit  west east
			}
			else if (side == 1) {
			    ray->x_percent = render.x + ray->dir_x * dist - map_x;
			    ray->y_percent = render.z + ray->dir_z * dist - map_z;
				if (ray->dir_y < 0) {
					ray->x_percent = 1 - ray->x_percent;
					ray->y_percent = 1 - ray->y_percent;
				}
				// hit up bot
			}
			else {
			    ray->x_percent = render.x + ray->dir_x * dist - map_x;
			    ray->y_percent = render.y + ray->dir_y * dist - map_y;
				if (ray->dir_z > 0) {
					ray->x_percent = 1 - ray->x_percent;
				}
				// hit north south
			}
			ray->has_hit = 1;
			ray->x = map_x;
			ray->y = map_y;
			ray->z = map_z;
			ray->dist = dist;
			return ;
		}
		
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
		if ((ray->dir_x < 0 && map_x < 0) || (ray->dir_x > 0 && map_x >= WORLD_SIZE))
			break;
		if ((ray->dir_y < 0 && map_y < 0) || (ray->dir_y > 0 && map_y >= WORLD_SIZE))
			break;
		if ((ray->dir_z < 0 && map_z < 0) || (ray->dir_z > 0 && map_z >= WORLD_SIZE))
			break;
	}
}

void raycast_draw(size_t y_start, size_t y_end) {
	int pitch = render.framebuffer_pitch;

	float rot_x = render.rot_x;// - M_PI / 2;
	float rot_y = render.rot_y;// - M_PI / 2;
	
	float yaw_sin = sinf(rot_y);
	float yaw_cos = cosf(rot_y);
	float pitch_sin = sinf(rot_x);
	float pitch_cos = cosf(rot_x);

	float forward_x = pitch_cos * yaw_cos;
	float forward_y = pitch_sin;
	float forward_z = pitch_cos * yaw_sin;

	float right_x = -yaw_sin;
	float right_y = 0;
	float right_z = yaw_cos;

	float up_x = -pitch_sin * yaw_cos;
	float up_y = pitch_cos;
	float up_z = -pitch_sin * yaw_sin;

	float aspect = (float)render.width / (float)render.height;
	float half_fov_tan = tan(render.fov / 2);

	size_t screen_height = render.height;
	size_t screen_width = render.width;

	int map_x = floorf(render.x);
	int map_y = floorf(render.y);
	int map_z = floorf(render.z);

	uint8_t *pixel_line = render.framebuffer;
	if (!pixel_line)
		return ;
	pixel_line += y_start * pitch;

	float sx_inc = 2 * half_fov_tan * aspect / screen_width;
	float sx_0 = (2.0 * (0 + 0.5) / screen_width - 1.0) * aspect * half_fov_tan;

	float sy = (1.0 - 2.0 * (y_start + 0.5) / screen_height) * half_fov_tan;
	float sy_inc = -2.0 / render.height * half_fov_tan;

	for (size_t screen_y = y_start; screen_y < y_end; screen_y++) {
		uint32_t *pixel = (void *)pixel_line;
		float dir_x = forward_x + right_x * sx_0 + up_x * sy;
		float dir_y = forward_y + right_y * sx_0 + up_y * sy;
		float dir_z = forward_z + right_z * sx_0 + up_z * sy;
		for (size_t screen_x = 0; screen_x < screen_width; screen_x++) {

			ray_t ray;
			ray.dir_x = dir_x;
			ray.dir_y = -dir_y;
			ray.dir_z = dir_z;
			ray.has_hit = 0;

			//float vec_len_inv = 1 / sqrtf(ray.dir_x * ray.dir_x + ray.dir_y * ray.dir_y + ray.dir_z * ray.dir_z);

			//ray.dir_x *= vec_len_inv;
			//ray.dir_y *= vec_len_inv;
			//ray.dir_z *= vec_len_inv;

			cast_ray(&ray, map_x, map_y, map_z, &clt.world);
			if (!ray.has_hit)
				*pixel = 0;
			else {
				ray.dist = 1;
				if ((ray.x_percent < 0.5 && ray.y_percent < 0.5) || (ray.x_percent > 0.5 && ray.y_percent > 0.5)) {
					if (ray.y > 0)
						*pixel = 0xffffff00;
					else
						*pixel = 0xff00ff00;
				}
				else
					if (ray.y > 0)
						*pixel = 0xff00ffff;
					else
						*pixel = 0xffff00ff;
			}

			pixel++;
    		dir_x += right_x * sx_inc;
    		dir_y += right_y * sx_inc;
    		dir_z += right_z * sx_inc;
		}
		pixel_line += pitch;
		sy += sy_inc;
	}
}
