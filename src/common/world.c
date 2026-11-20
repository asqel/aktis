#include "common.h"

void remove_entity(world_t *world, size_t idx) {
	world->entities[idx] = world->entities[world->entities_len - 1];
	world->entities_len--;
}
