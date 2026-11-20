#include "server.h"

void srv_broadcast_remove(world_t *world, uint16_t entity_idx) {
	for (size_t i = 0; i < srv.sock.clients_len; i++) {
		client_t *client = srv.sock.clients[i].data;
		if (!client)
			continue;
		if (client->world != world->id || client->entity_idx == entity_idx)
			continue;

		printf("SENDING RM ENT\n");
		packet_append(&client->send, OPC_RM_ENT, "w", entity_idx);
		// !TODO check that
	}
}

client_t *find_client(uint32_t entity_idx, world_t *world) {
	for (size_t i = 0; i < srv.sock.clients_len; i++) {
		client_t *client= srv.sock.clients[i].data;
		if (!client)
			continue;
		if (client->world != world->id || client->entity_idx != entity_idx)
			continue;
		return client;
	}
	return NULL;
}

void remove_client(client_t *client) {
	if (!client)
		return ;
	buffer_free(&client->recv);
	buffer_free(&client->send);

	world_t *world = &srv.worlds[client->world];

	client_t *to_reloc = NULL;
	uint16_t to_reloc_idx = 0;
	if (world->entities_len > 1)
		to_reloc = find_client(world->entities_len - 1, world);

	if (to_reloc)
		to_reloc_idx = client->entity_idx;
	
	remove_entity(world, client->entity_idx);
	srv_broadcast_remove(world, client->entity_idx);

	if (to_reloc)
		to_reloc->entity_idx = to_reloc_idx;
}

void srv_broadcast_new_player(uint16_t world_id) {
	world_t *world = &srv.worlds[world_id];
	entity_t *ent = &world->entities[world->entities_len - 1];
	
	for (size_t i = 0; i < srv.sock.clients_len; i++) {
		ra_srv_clt_t *clt = &srv.sock.clients[i];
		client_t *client = clt->data;
		if (!client)
			continue;

		if (client->world != world_id)
			continue;

		packet_append(&client->send, OPC_ADD_ENT, "dddd", ent->x, ent->y, ent->type, ent->data);
	}
}

void srv_broadcast_move(world_t *world, entity_t *ent, uint16_t entity_idx) {
	for (size_t i = 0; i < srv.sock.clients_len; i++) {
		ra_srv_clt_t *clt = &srv.sock.clients[i];
		client_t *client = clt->data;
		if (!client)
			continue;

		if (client->world != world->id)
			continue;

		packet_append(&client->send, OPC_MOVE, "wdd", entity_idx, ent->x, ent->y);
	}
}

void srv_broadcast_entity_data(entity_t *ent, world_t *world, uint32_t ent_idx) {
	for (size_t i = 0; i < srv.sock.clients_len; i++) {
		ra_srv_clt_t *clt = &srv.sock.clients[i];
		client_t *client = clt->data;
		if (!client)
			continue;

		if (client->world != world->id)
			continue;

		packet_append(&client->send, OPC_SET_DATA, "dd", ent_idx, ent->data);
	}
}

void srv_broadcast_remove_ent(world_t *world, uint16_t idx) {
	for (size_t i = 0; i < srv.sock.clients_len; i++) {
		ra_srv_clt_t *clt = &srv.sock.clients[i];
		client_t *client = clt->data;
		if (!client)
			continue;
		if (client->world != world->id)
			continue;

		packet_append(&client->send, OPC_RM_ENT, "w", idx);
	}
}
