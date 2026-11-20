#include "server.h"

int srv_get_timeout() {
	if (srv.sock.clients_len)
		return 1000 / 60;
	printf("server empty wait infinitly\n");
	return -1;
}

static int will_end = 0;

#ifndef _WIN32
#include <signal.h>

void sig_hand(int sig) {
	(void)sig;
	will_end = 1; 
}
#endif

void loop_srv() {
	#ifndef _WIN32
		signal(SIGINT, sig_hand);
	#endif
	srv.sock.on_connect = srv_on_connect;
	srv.sock.on_recv = srv_on_recv;
	srv.sock.on_send_ready = srv_on_send_ready;
	srv.sock.on_disconnect = srv_on_disconnect;
	while (!will_end) {
		srv.need_send_count = 0;
		ra_server_wait(&srv.sock, srv_get_timeout());
		srv.sock.need_send = 0;

		for (size_t i = 0; i < srv.sock.clients_len; i++) {
			ra_srv_clt_t *oe_client = &srv.sock.clients[i];
			client_t *client = oe_client->data;
			if (!client)
				continue;

			srv_tick_client(i);
			if (client->send.len && client->can_send)
				srv.need_send_count++;

		}
		if (srv.need_send_count)
			srv.sock.need_send = 1;

		srv_tick();
	}
}
