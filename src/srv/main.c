#include "server.h"

server_t srv = {0};

int main() {
	if (ra_init()) {
		printf("init ra\n");
		return 1;
	}

	if (ra_server_init(&srv.sock, 42124, RA_FLAG_NODELAY)) {
		printf("init server\n");
		ra_exit();
		return 1;
	}

	loop_srv();
	clean_srv();

	ra_exit();
	return 0;
}
