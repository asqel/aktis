#include "server.h"

int srv_on_connect(ra_srv_clt_t *clt, ra_srv_ctx_t *srv) {
	printf("SHSHSHS\n");
	(void)srv;
	client_t *client = malloc(sizeof(client_t));
	if (!client)
		return 1;

	world_t *world = find_world();
	if (!world) {
		free(client);
		return 1;
	}
	
	if (world->entities_len == world->entities_alloc) {
		void *new = NULL;
		if (world->entities_alloc == 0)
			new = realloc(world->entities, sizeof(entity_t) * 4);
		else
			new = realloc(world->entities, sizeof(entity_t) * world->entities_alloc * 2);

		if (!new) {
			free(client);
			return 1;
		}

		world->entities = new;
		if (world->entities_alloc == 0)
			world->entities_alloc = 4;
		else
			world->entities_alloc *= 2;
	}
	clt->data = client;
	world->entities[world->entities_len].x = TILE_SIZE * 20;
	world->entities[world->entities_len].y = TILE_SIZE;
	world->entities[world->entities_len].data = 0;
	world->entities[world->entities_len].type = (rand() & 0xff);
	world->entities[world->entities_len].type |= (rand() & 0xff) << 8;
	world->entities[world->entities_len].type |= (rand() & 0xff) << 16;
	world->entities[world->entities_len].type |= 1 << 31;
	printf("new player %x\n", world->entities[world->entities_len].type);
	*client = (client_t){0};
	client->world = world->id;
	client->entity_idx = world->entities_len;
	world->entities_len++;
	
	uint16_t packet_len = WORLD_WIDTH * WORLD_HEIGHT * 4 + 4 * 4 * world->entities_len;
	if (buffer_reserve(&client->send, 6 + packet_len)) {
		free(client);
		world->entities_len--;
		return 1;
	}
	// !TODO inform other player in the world
	srv_broadcast_new_player(client->world);


	buffer_t *buffer = &client->send;
	buffer_append_u32(buffer, OPC_CHANGE_WORLD);
	buffer_append_u16(buffer, packet_len);

	for (size_t y = 0; y < WORLD_HEIGHT; y++) {
		for (size_t x = 0; x < WORLD_WIDTH; x++) {
			buffer_append_u32(buffer, world->blocks[y][x]);
		}
	}
	for (size_t i = 0; i < world->entities_len; i++) {
		buffer_append_u32(buffer, world->entities[i].x);
		buffer_append_u32(buffer, world->entities[i].y);
		buffer_append_u32(buffer, world->entities[i].type);
		buffer_append_u32(buffer, world->entities[i].data);
	}
	return 0;
}

void srv_on_recv(ra_srv_clt_t *clt, ra_srv_ctx_t *srv, void *data, size_t data_len) {
	(void)srv;
	
	client_t *client = clt->data;
	if (!client)
		return ;
	if (buffer_append(&client->recv, data, data_len)) {
		remove_client(client);
		clt->data = NULL;
		ra_server_remove(srv, clt);
		return ;
	}
}

void srv_on_send_ready(ra_srv_clt_t *clt, ra_srv_ctx_t *srv_ctx) {
	client_t *client = clt->data;
	if (!client)
		return ;
	
	ssize_t ret = ra_server_send(srv_ctx, clt, client->send.data, client->send.len);
	if (ret < 0) {
		remove_client(client);
		clt->data = NULL;
		ra_server_remove(srv_ctx, clt);
		return ;
	}
	if (ret == 0)
		return ;
	buffer_rm_front(&client->send, ret);
	if (client->send.len == 0) {
		srv.need_send_count--;
		if (srv.need_send_count == 0)
			srv.sock.need_send = 0;
	}
}

void srv_on_disconnect(ra_srv_clt_t *clt, ra_srv_ctx_t *srv, int is_error) {
	(void)is_error;	
	(void)srv;
	if (clt->data == NULL)
		return ;
	remove_client(clt->data);
	free(clt->data);
	clt->data = NULL;
}
