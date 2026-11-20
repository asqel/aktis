#include "server.h"

server_t srv = {0};

int main() {
	if (oeso_init()) {
		printf("init oeso\n");
		return 1;
	}

	if (oeso_server_init(&srv.sock, 42124, OESO_FLAG_NODELAY)) {
		printf("init server\n");
		oeso_exit();
		return 1;
	}

	loop_srv();
	clean_srv();

	oeso_exit();
	return 0;
}
