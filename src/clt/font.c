#include "client.h"

#define FONT_WIDTH 10
#define FONT_HEIGHT 20

void draw_text(const char *str, int x, int y, double scale, uint8_t r, uint8_t g, uint8_t b) {

	size_t i = 0;
	while (str[i]) {
		char c = str[i];
		SDL_FRect dest = {0};
		dest.x = x + i * FONT_WIDTH * scale;
		dest.y = y;
		dest.w = FONT_WIDTH * scale;
		dest.h = FONT_HEIGHT * scale;
		
		SDL_FRect src = {0};
		src.x = (c & 0xf) * FONT_WIDTH;
		src.y = (c >> 4) * FONT_HEIGHT;
		src.w = FONT_WIDTH;
		src.h = FONT_HEIGHT;

		render_texture(render.font, &src, &dest, (r) | (g << 8) | (b << 16) | (0xff << 24));
		i++;
	}
}

void draw_text_centered(const char *text, int y, int scale, uint8_t r, uint8_t g, uint8_t b) {
	int len = strlen(text);
	len *= FONT_WIDTH * scale;

	draw_text(text, render.swap.width / 2 - len / 2, y, scale, r, g, b);
}
