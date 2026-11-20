#include "client.h"
#include <stdbool.h>

client_t clt = {0};

void client_update_start() {
}

void client_update_game() {
	packet_append(&clt.send, OPC_PING, "");
}

void client_update() {
	if (clt.has_sock) {
		if (clt.send.len)
			clt.sock.need_send = 1;
		ra_client_wait(&clt.sock, 10);
	}
	switch (clt.state) {
		case CLT_START:
			client_update_start();
			break;
		case CLT_LOADING:
			if (clt.sock.fd == INVALID_SOCKET)
				client_fail("OULA YA UNE ERREUR la");
			break;
		case CLT_INGAME:
			client_update_game();
			break;
		default:
			break;
	}
}
