#include <stdarg.h>
#include "common.h"
#include <string.h>

static size_t count_format(const char *format, va_list *lst) {
	size_t res = 0;
	size_t i = 0;
	while (format[i] != '\0') {
		switch (format[i]) {
			case 'b':
				res++;
				va_arg(*lst, int);
				break;

			case 'w':
				res += 2;
				va_arg(*lst, int);
				break;

			case 'd':
				res += 4;
				va_arg(*lst, int);
				break;

			case 'f':
				res += 4;
				va_arg(*lst, double);
				break;

			case 'l':
				res += 8;
				va_arg(*lst, uint64_t);
				break;

			case 's':
				char *s = va_arg(*lst, char *);
				if (!s)
					res++;
				else
					res += strlen(s) + 1;
				break;

			case 'p':
				res += va_arg(*lst, uint32_t);
				va_arg(*lst, uint8_t *);
				break;

			default:
				break;
		}
		i++;
	}
	return res;
}

int packet_append(buffer_t *dest, uint32_t opcode, const char *format, ...) {
	va_list lst = {0};
	va_list lst_copy = {0};

	va_start(lst, format);

	va_copy(lst_copy, lst);
	uint16_t packet_len = count_format(format, &lst_copy);
	if (buffer_reserve(dest, 6 + dest->len + packet_len))
		return 1;
	va_end(lst_copy);

	dest->data[dest->len] = opcode & 0xff;
	dest->data[dest->len + 1] = (opcode >> 8) & 0xff;
	dest->data[dest->len + 2] = (opcode >> 16) & 0xff;
	dest->data[dest->len + 3] = (opcode >> 24) & 0xff;
	dest->data[dest->len + 4] = packet_len & 0xff;
	dest->data[dest->len + 5] = packet_len >> 8;
	dest->len += 6;

	size_t i = 0;
	while (format[i] != '\0') {
		switch (format[i]) {
			case 'b':
				dest->data[dest->len] = (uint8_t)va_arg(lst, int);
				dest->len++;
				break;

			case 'w':
				uint16_t word = (uint16_t)va_arg(lst, int);
				dest->data[dest->len] = word & 0xff;
				dest->data[dest->len + 1] = word >> 8;
				dest->len += 2;
				break;

			case 'd':
				uint32_t val = va_arg(lst, int);
				dest->data[dest->len] = val & 0xff;
				dest->data[dest->len + 1] = (val >> 8) & 0xff;
				dest->data[dest->len + 2] = (val >> 16) & 0xff;
				dest->data[dest->len + 3] = (val >> 24) & 0xff;
				dest->len += 4;
				break;
			
			case 'f':
				// !TODO add check for non ieee float but who cares
				float val_float = va_arg(lst, double);
				uint32_t float_u32 = 0;
				memcpy(&float_u32, &val_float, 4);

				dest->data[dest->len] = float_u32 & 0xff;
				dest->data[dest->len + 1] = (float_u32 >> 8) & 0xff;
				dest->data[dest->len + 2] = (float_u32 >> 16) & 0xff;
				dest->data[dest->len + 3] = (float_u32 >> 24) & 0xff;
				dest->len += 4;
				break;

			case 'l':
				uint64_t val_long = va_arg(lst, uint64_t);
				dest->data[dest->len] = val_long & 0xff;
				dest->data[dest->len + 1] = (val_long >> 8) & 0xff;
				dest->data[dest->len + 2] = (val_long >> 16) & 0xff;
				dest->data[dest->len + 3] = (val_long >> 24) & 0xff;
				dest->data[dest->len + 4] = (val_long >> 32) & 0xff;
				dest->data[dest->len + 5] = (val_long >> 40) & 0xff;
				dest->data[dest->len + 6] = (val_long >> 48) & 0xff;
				dest->data[dest->len + 6] = (val_long >> 56) & 0xff;
				dest->len += 8;
				break;
			
			case 's':
				char *str = va_arg(lst, char *);
				if (!str)
					dest->data[dest->len++] = '\0';
				else {
					size_t len = strlen(str);
					memcpy(&dest->data[dest->len], str, len);
					dest->len += len + 1;
				}
				break;
			
			case 'p':
				uint32_t len = va_arg(lst, uint32_t);
				uint8_t *ptr = va_arg(lst, uint8_t *);
				memcpy(&dest->data[dest->len], ptr, len);
				dest->len += len;
				break;

			default:
				break;
		}
		i++;
	}

	va_end(lst);
	return 0;
}
