#include "client.h"

static char fail_msg[4096] = {0};

void client_fail(const char *msg) {
	strncpy(fail_msg, msg, sizeof(fail_msg) - 1);
	clt.state = CLT_FAIL;
}

void client_draw_fail() {
	draw_text(fail_msg, 50, 50, 2, 255, 0, 0);
}
