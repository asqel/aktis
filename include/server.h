#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include "common.h"
#include "raisin.h"

#define WORLD_MAX 256

typedef struct {
	ra_srv_ctx_t sock;
	world_t worlds[WORLD_MAX];
	size_t need_send_count;
} server_t;

typedef struct {
	uint16_t world;
	uint16_t entity_idx;
	buffer_t recv;
	buffer_t send;
	uint8_t will_remove;
	uint8_t can_send;
	uint32_t key_pressed;
	uint8_t shoot_delay;
} client_t;

extern server_t srv;

void loop_srv();
world_t *find_world();

int srv_on_connect(ra_srv_clt_t *clt, ra_srv_ctx_t *srv);
void srv_on_recv(ra_srv_clt_t *clt, ra_srv_ctx_t *srv, void *data, size_t data_len);
void srv_on_send_ready(ra_srv_clt_t *clt, ra_srv_ctx_t *srv);
void srv_on_disconnect(ra_srv_clt_t *clt, ra_srv_ctx_t *srv, int is_error);

void remove_client(client_t *client);
void srv_broadcast_new_player(uint16_t world_id);
void srv_broadcast_move(world_t *world, entity_t *ent, uint16_t entity_idx);
void srv_broadcast_entity_data(entity_t *ent, world_t *world, uint32_t entity_idx);
void srv_broadcast_remove_ent(world_t *world, uint16_t idx);

void srv_tick_client(size_t client_idx);
void clean_srv();
client_t *find_client(uint32_t entity_idx, world_t *world);

void srv_add_entity(entity_t *ent, uint16_t world_id);
void srv_tick();

#endif
