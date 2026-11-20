#include "client.h"

void client_handle_packet(uint32_t opcode, uint8_t *data, uint16_t data_len) {
	if (opcode == OPC_ERR) {
		printf("YA UNE ERREUR TA MERE\n\t");
		for (size_t i = 0; i < data_len; i++) {
			if (i % 16 == 0 && i != 0)
				printf("\n\t");
			char *alph = "0123456789abcdef";
			printf("%c%c ", alph[data[i] >> 4], alph[data[i] & 0xf]);
		}
		printf("\n");
	}
	//else if (opcode == OPC_CHANGE_WORLD) {
	//	if (data_len < WORLD_WIDTH * WORLD_HEIGHT * 4) {
	//		printf("wrong size OPC_CHANGE_WORLD %d\n", data_len);
	//		return ;
	//	}

	//	size_t i = 0;
	//	int x = 0;
	//	int y = 0;
	//	while (i < WORLD_WIDTH * WORLD_HEIGHT * 4) {
	//		clt.world.blocks[y][x] = read_u32(&data[i]);
	//		i += 4;
	//		x++;
	//		if (x >= WORLD_WIDTH) {
	//			x = 0;
	//			y++;
	//		}
	//	}

	//	size_t entities_len = data_len - (WORLD_WIDTH * WORLD_HEIGHT * 4);
	//	entities_len = entities_len / (4 * 4);

	//	if (clt.world.entities_alloc < entities_len) {
	//		void *new = realloc(clt.world.entities, sizeof(entity_t) * entities_len);
	//		if (!new)
	//			abort(); // !TODO

	//		clt.world.entities = new;
	//		clt.world.entities_alloc = entities_len;
	//	}
	//	clt.world.entities_len = 0;
	//	while (i + 4 * 4 - 1 < data_len) {
	//		entity_t *ent = &clt.world.entities[clt.world.entities_len];
	//		ent->x = read_u32(&data[i]);
	//		ent->y = read_u32(&data[i + 4]);
	//		ent->type = read_u32(&data[i + 8]);
	//		printf("aaaaaa %x\n", ent->type);
	//		ent->data = read_u32(&data[i + 12]);
	//		clt.world.entities_len++;
	//		i += 4 * 4;
	//	}
	//	printf("world enditites %u\n", clt.world.entities_len);

	//}
	//else if (opcode == OPC_SET_DATA && data_len == 8) {
	//	uint32_t idx = read_u32(data);
	//	uint32_t new_data = read_u32(data + 4);
	//	if (idx < clt.world.entities_len)
	//		clt.world.entities[idx].data = new_data;
	//}
	//else if (opcode == OPC_ADD_ENT) {
	//	if (data_len != 4 * 4) {
	//		printf("wrong size OPC_ADD_ENT %d\n", data_len);
	//		return ;
	//	}
	//	if (clt.world.entities_alloc <= clt.world.entities_len) {
	//		if (clt.world.entities_alloc == 0)
	//			clt.world.entities_alloc = 2;
	//		void *new = realloc(clt.world.entities, sizeof(entity_t) * clt.world.entities_alloc * 2);
	//		if (!new)
	//			abort(); // !TODO
	//		clt.world.entities_alloc *= 2;
	//		clt.world.entities = new;
	//	}

	//	clt.world.entities[clt.world.entities_len].x = read_u32(data);
	//	clt.world.entities[clt.world.entities_len].y = read_u32(data + 4);
	//	clt.world.entities[clt.world.entities_len].type = read_u32(data + 8);
	//	clt.world.entities[clt.world.entities_len].data = read_u32(data + 12);
	//	clt.world.entities_len++;
	//	printf("ADED AN ANTITY\n");
	//}
	//else if (opcode == OPC_RM_ENT) {
	//	printf("removing %u\n", read_u16(data));
	//	remove_entity(&clt.world, read_u16(data));
	//}
	//else if (opcode == OPC_MOVE) {
	//	uint16_t idx = read_u16(data);
	//	clt.world.entities[idx].x = read_u32(data + 2);
	//	clt.world.entities[idx].y = read_u32(data + 6);
	//}
	else {
		printf("unknown opcode %x len %d\n", opcode, data_len);
	}
}
