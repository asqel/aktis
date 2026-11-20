#include "common.h"
#include <string.h>

uint32_t read_u32(uint8_t *data) {
	uint32_t res = 0;
	res |= ((uint32_t)data[0]);
	res |= ((uint32_t)data[1]) << 8;
	res |= ((uint32_t)data[2]) << 16;
	res |= ((uint32_t)data[3]) << 24;
	
	return res;
}

uint16_t read_u16(uint8_t *data) {
	uint16_t res = 0;
	res |= ((uint32_t)data[0]);
	res |= ((uint32_t)data[1]) << 8;

	return res;
}

float read_float(uint8_t *data) {
	uint32_t float_u32 = read_u32(data);
	float res = 0;
	memcpy(&res, &float_u32, 4);
	return res;
}
