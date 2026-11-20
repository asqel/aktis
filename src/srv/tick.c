#include "server.h"


void handle_packet(uint32_t opcode, uint8_t *data, uint16_t data_len, client_t *client) {
	if (opcode == OPC_SET_KEY && data_len == 2) {

		if (data[1] > 31)
			return ;
		if (data[0])
			client->key_pressed |= (1 << data[1]);
		else
			client->key_pressed &= ~(1 << data[1]);
	}
	else if (opcode == OPC_ASK_START) {
		client->can_send = 1;
		entity_t *ent = &srv.worlds[client->world].entities[client->entity_idx];
		if (data_len == 4)
			ent->data = read_u32(data);
		else
			ent->data = 'z' - 'a' + 1;
		printf("LA DATA %x\n", ent->data);

		srv_broadcast_entity_data(ent, &srv.worlds[client->world], client->entity_idx);
	}
	else if (opcode == OPC_PING) {
	}
	else {
		printf("UNkown packet %u %u\n", opcode, data_len);
	}
}

void tick_move(client_t *client) {
	int y_off = 0;
	int x_off = 0;

	if (client->key_pressed & (1 << 0))
		y_off -= 1;
	if (client->key_pressed & (1 << 2))
		y_off += 1;

	if (client->key_pressed & (1 << 1))
		x_off += 1;
	if (client->key_pressed & (1 << 3))
		x_off -= 1;

	if (x_off == 0 && y_off == 0)
		return ;

	int speed = 4;
	x_off *= speed;
	y_off *= speed;

	entity_t *ent = &srv.worlds[client->world].entities[client->entity_idx];
	if (x_off < 0 && ent->x < (uint32_t)-x_off)
		ent->x = 0;
	else if (x_off > 0 && ent->x + x_off >= GAME_WIDTH)
		ent->x = GAME_WIDTH - 1;
	else
		ent->x += x_off;

	if (y_off < 0 && ent->y < (uint32_t)-y_off)
		ent->y = 0;
	else if (y_off > 0 && ent->y + y_off >= GAME_HEIGHT)
		ent->y = GAME_HEIGHT - 1;
	else
		ent->y += y_off;

	srv_broadcast_move(&srv.worlds[client->world], ent, client->entity_idx);
}

void tick_shoot(client_t *client) {
	entity_t *ent = &srv.worlds[client->world].entities[client->entity_idx];
	
	if (client->shoot_delay) {
		client->shoot_delay--;
		return ;
	}

	entity_t ent_bullet = {0};
	ent_bullet.type = ENT_BULLET;

	int v_x = 0;
	int v_y = 0;
	if (client->key_pressed & (1 << 4)) {
		v_y = -1;
		v_x = 0;
	}
	if (client->key_pressed & (1 << 5)) {
		v_x = 1;
		v_y = 0;
	}
	if (client->key_pressed & (1 << 6)) {
		v_y = 1;
		v_x = 0;
	}
	if (client->key_pressed & (1 << 7)) {
		v_x = -1;
		v_y = 0;
	}
	
	if (v_x == 0 && v_y == 0)
		return ;
	
	client->shoot_delay = 20;
	
	v_x *= 3;
	v_y *= 3;

	if (client->key_pressed & (1 << 0))
		v_y -= 1;
	if (client->key_pressed & (1 << 2))
		v_y += 1;

	if (client->key_pressed & (1 << 1))
		v_x += 1;
	if (client->key_pressed & (1 << 3))
		v_x -= 1;
	
	int x = (int)ent->x;
	int y = (int)ent->y;
	x += v_x * 4;
	y += v_y * 4;
	if (x < 0)
		x = 0;
	if (y < 0)
		y = 0;
	if (x > GAME_WIDTH)
		x = GAME_WIDTH - 1;
	if (y > GAME_HEIGHT)
		y = GAME_HEIGHT - 1;
	
	ent_bullet.x = (uint32_t)x;
	ent_bullet.y = (uint32_t)y;
	
	if (v_x < 0) {
		ent_bullet.data |= -v_x;
		ent_bullet.data |= (1 << 7);
	}
	else {
		ent_bullet.data |= v_x;
	}

	if (v_y < 0) {
		ent_bullet.data |= (-v_y) << 8;
		ent_bullet.data |= (1 << 7) << 8;
	}
	else {
		ent_bullet.data |= v_y << 8;
	}

	srv_add_entity(&ent_bullet, client->world);
}

void srv_tick_client(size_t client_idx) {
	ra_srv_clt_t *clt = &srv.sock.clients[client_idx];
	client_t *client = clt->data;
	if (!client)
		return ;

	while (1) {
		if (client->recv.len < 6)
			break;
	
		uint16_t packet_len = client->recv.data[4] | ((uint16_t)client->recv.data[5]<< 8);
		if (client->recv.len < 6u + packet_len)
			break;

		uint32_t opcode = read_u32(client->recv.data);

		handle_packet(opcode, client->recv.data + 6, packet_len, client);

		buffer_rm_front(&client->recv, 6 + packet_len);
	}

	tick_move(client);
	tick_shoot(client);
}

void srv_tick() {
	for (size_t i = 0; i < WORLD_MAX; i++) {
		world_t *world = &srv.worlds[i];
		
		size_t i = 0;
		while (i < world->entities_len) {
			entity_t *ent = &world->entities[i];
			if (ent->type >> 31) {
				i++;
				continue;
			}
			if (ent->type == ENT_BULLET) {
				int cooldown = ent->data >> 24;
				if (cooldown == 60 * 1) {
					client_t *clt = find_client(world->entities_len - 1, world);
					remove_entity(world, i);
					if (clt) {
						clt->entity_idx = i;
					}
					srv_broadcast_remove_ent(world, i);
					continue;
				}
				cooldown++;
				ent->data &= ~(0xff << 24);
				ent->data |= (cooldown << 24);

				int v_x = ent->data & 0x7f;
				int v_y = (ent->data >> 8) & 0x7f;
				if ((ent->data & 0xff) >> 7)
					v_x = -(ent->data & 0x7f);
				if (((ent->data >> 8) & 0xff) >> 7)
					v_y = -((ent->data >> 8) & 0x7f);

				int x = (int)ent->x + v_x * 2;
				int y = (int)ent->y + v_y * 2;

				if (x < 0)
					x = 0;
				if (y < 0)
					y = 0;
				if (x > GAME_WIDTH)
					x = GAME_WIDTH - 1;
				if (y > GAME_HEIGHT)
					y = GAME_HEIGHT - 1;

				ent->x = (uint32_t)x;
				ent->y = (uint32_t)y;
				srv_broadcast_move(world, ent, i);
				i++;
			}
			else
				i++;
		}
	}
}
