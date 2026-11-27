#include "client.h"

block_model_t block_models[4096] = {0};
uint32_t block_count = 0;

void register_block_model(uint16_t id, float min_x, float min_y, float min_z, float max_x, float max_y, float max_z) {
	block_models[id].is_special = 1;
	block_models[id].aabb_min[0] = min_x;
	block_models[id].aabb_min[1] = min_y;
	block_models[id].aabb_min[2] = min_z;

	block_models[id].aabb_max[0] = max_x;
	block_models[id].aabb_max[1] = max_y;
	block_models[id].aabb_max[2] = max_z;
}
