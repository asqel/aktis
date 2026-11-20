#include "client.h"

void clt_on_connect(oeso_clt_ctx_t *ctx, int success) {
	(void)ctx;
	if (!success) {
		client_fail("PAS REUSSI A SE CO");
		oeso_client_exit(&clt.sock);
		clt.has_sock = 0;
	}
	else {
		clt.state = CLT_INGAME;
		uint32_t name_data = 0;
		for (int i = 5; i >= 0; i--) {
			name_data <<= 5;
			if (clt.name[i])
				name_data |= clt.name[i] - 'A' + 1;
		}
		printf("LE NAME %x\n", name_data);
		packet_append(&clt.send, OPC_ASK_START, "d", name_data);
	}
}

void clt_on_disconnect(oeso_clt_ctx_t *ctx, int is_error) {
	(void)ctx;
	(void)is_error;
	client_fail("LE SERVER EST DECEDE");
	oeso_client_exit(&clt.sock);
	clt.has_sock = 0;
}

void clt_on_recv(oeso_clt_ctx_t *ctx, void *data, size_t data_len) {
	(void)ctx;
	// !TODO check return alloc
	buffer_append(&clt.recv, data, data_len);

	while (1) {
		if (clt.recv.len < 6)
			break;
	
		uint16_t packet_len = clt.recv.data[4] | ((uint16_t)clt.recv.data[5]<< 8);
		if (clt.recv.len < 6u + packet_len)
			break;

		uint32_t opcode = read_u32(clt.recv.data);

		client_handle_packet(opcode, clt.recv.data + 6, packet_len);

		buffer_rm_front(&clt.recv, 6 + packet_len);
	}
}

void clt_on_send_ready(oeso_clt_ctx_t *ctx) {
	ssize_t ret = oeso_client_send(ctx, clt.send.data, clt.send.len);
	if (ret < 0) {
		oeso_client_exit(ctx);
		clt.has_sock = 0;
		client_fail("LE SERVER!!!");
	}
	buffer_rm_front(&clt.send, ret);
	if (clt.send.len == 0)
		ctx->need_send = 0;
}
