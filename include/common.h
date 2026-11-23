#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#define ENTITIES_MAX 4096

#define CHUNK_SIZE (16)
#define COORD_CUHNK(X) ((X) >> 4)
#define COORD_OFFSET(X) ((X) & 0x0f)
#define CHUNK_OFFSET(CX, CY, CZ, WIDTH) (CY * WIDTH * WIDTH + CZ * WIDTH + CX)
#define COORD_TO_BLOCK(X, Y, Z) ((COORD_OFFSET(Y) << 8) | (COORD_OFFSET(Z) << 4) | COORD_OFFSET(X))

#define RAD(X) ((X) * M_PI / 180.0)

typedef struct {
	float x;
	float y;
	float rot_x;
	float rot_y;
	float rot_z;
	uint32_t type;
	uint32_t data;
} entity_t;

typedef struct {
	uint32_t (*blocks)[CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE];

	uint32_t entitites_alloc; // contains entities that collide with this chunk
	uint32_t entities_len;
	entity_t *entities;
} chunk_t;

typedef struct {
	uint16_t id;
	chunk_t *chunks;
	uint16_t width;
	uint16_t height;
} world_t;

enum {
	OPC_ERR,
	OPC_CHANGE_WORLD,
	OPC_ADD_ENT,
	OPC_RM_ENT,
	OPC_SET_KEY,
	OPC_ASK_START,
	OPC_PING,
	OPC_SET_DATA,
	OPC_MOVE,
};

typedef struct {
	size_t alloc_len;
	size_t len;
	uint8_t *data;
} buffer_t;

int buffer_reserve(buffer_t *buffer, size_t atleast);
int buffer_append(buffer_t *buffer, const uint8_t *data, size_t len);
int buffer_append_u32(buffer_t *buffer, uint32_t val);
int buffer_append_u16(buffer_t *buffer, uint16_t val);
void buffer_rm_front(buffer_t *buffer, size_t len);
void buffer_free(buffer_t *buffer);

int packet_append(buffer_t *dest, uint32_t opcode, const char *format, ...);

void client_handle_packet(uint32_t opcode, uint8_t *data, uint16_t data_len);

uint32_t read_u32(uint8_t *data);
uint16_t read_u16(uint8_t *data);
float read_float(uint8_t *data);

void remove_entity(world_t *world, size_t idx);

#define HITBOX_PLAYER_W (TILE_SIZE / 2)
#define HITBOX_PLAYER_H (TILE_SIZE / 2)

#define HITBOX_BULLET_W (TILE_SIZE / 2)
#define HITBOX_BULLET_H (TILE_SIZE / 2)

enum {
	ENT_BULLET,
};

#endif
