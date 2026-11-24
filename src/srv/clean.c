#include "server.h"

void clean_srv() {
	for (size_t i = 0; i < WORLD_MAX; i++) {
		world_t *world = &srv.worlds[i];
		free(world->chunks->entities);
		world->chunks->entities = NULL;
		world->chunks->entities_len = 0;
		//world->chunks->entities_alloc = 0;
	}

	for (size_t i = 0; i < srv.sock.clients_len; i++) {
		ra_srv_clt_t *clt = &srv.sock.clients[i];
		client_t *client = clt->data;
		buffer_free(&client->recv);
		buffer_free(&client->send);

		free(clt->data);
		clt->data = NULL;
	}
	ra_server_exit(&srv.sock);
}
