#include "server.h"

world_t *find_world() {
	for (size_t i = 0; i < WORLD_MAX; i++) {
		world_t *world = &srv.worlds[i];

		if (world->entities_len < ENTITIES_MAX)
			return world;
	}
	return NULL;
}

void srv_add_entity(entity_t *ent, uint16_t world_id) {
	world_t *world = &srv.worlds[world_id];

	if (world->entities_alloc == world->entities_len) {
		size_t alloc_size = world->entities_alloc;
		if (!alloc_size)
			alloc_size = 2;
		alloc_size *= 2;

		void *new = realloc(world->entities, sizeof(entity_t) * alloc_size);
		if (!new)
			abort();

		world->entities = new;
		world->entities_alloc = alloc_size;
	}
	
	world->entities[world->entities_len++] = *ent;
	srv_broadcast_new_player(world->id);
}
