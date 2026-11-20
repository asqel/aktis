#include "common.h"
#include <string.h>

int buffer_reserve(buffer_t *buffer, size_t atleast) {
	if (buffer->alloc_len >= atleast)
		return 0;
	void *data = realloc(buffer->data, atleast);
	if (!data)
		return 1;
	buffer->data = data;
	buffer->alloc_len = atleast;
	return 0;
}

void buffer_rm_front(buffer_t *buffer, size_t len) {
	if (len == buffer->len) {
		buffer_free(buffer);
		return ;
	}
	memmove(buffer->data, &buffer->data[len], buffer->len - len);
	buffer->len -= len;
	void *data = realloc(buffer->data, buffer->len);
	if (!data)
		return ;
	buffer->data = data;
	buffer->alloc_len = buffer->len;
	return ;
}

int buffer_append(buffer_t *buffer, const uint8_t *data, size_t len) {
	if (buffer->len + len > buffer->alloc_len) {
		void *new_buff = realloc(buffer->data, buffer->len + len);
		if (!new_buff)
			return 1;
		buffer->data = new_buff;
		buffer->alloc_len = buffer->len + len;
	}
	memmove(&buffer->data[buffer->len], data, len);
	buffer->len += len;
	return 0;
}

int buffer_append_u32(buffer_t *buffer, uint32_t val) {
	uint8_t tmp[4] = {0};

	tmp[0] = val & 0xff;
	tmp[1] = (val >> 8) & 0xff;
	tmp[2] = (val >> 16) & 0xff;
	tmp[3] = (val >> 24) & 0xff;

	return buffer_append(buffer, tmp, 4);
}

int buffer_append_u16(buffer_t *buffer, uint16_t val) {
	uint8_t tmp[2] = {0};

	tmp[0] = val & 0xff;
	tmp[1] = (val >> 8) & 0xff;

	return buffer_append(buffer, tmp, 2);
}

void buffer_free(buffer_t *buffer) {
	free(buffer->data);
	buffer->data = NULL;
	buffer->len = 0;
	buffer->alloc_len = 0;
}
