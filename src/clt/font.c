//#include "client.h"
//
//font_t font = {0};
//
//int init_font() {
//	font.texture = IMG_LoadTexture(render.renderer, "./font.png");
//	if (!font.texture) {
//		printf("YA PAS LA FONT\n");
//		return 1;
//	}
//	SDL_SetTextureScaleMode(font.texture, SDL_SCALEMODE_NEAREST);
//
//	font.width = 10;
//	font.height = 20;
//	return 0;
//}
//
//void exit_font() {
//	SDL_DestroyTexture(font.texture);
//}
//
//void draw_text(const char *str, int x, int y, double scale, uint8_t r, uint8_t g, uint8_t b) {
//	SDL_SetTextureColorMod(font.texture, r, g, b);
//
//	size_t i = 0;
//	while (str[i]) {
//		char c = str[i];
//		SDL_FRect dest = {0};
//		dest.x = x + i * font.width * scale;
//		dest.y = y;
//		dest.w = font.width * scale;
//		dest.h = font.height * scale;
//		
//		SDL_FRect src = {0};
//		src.x = (c & 0xf) * font.width;
//		src.y = (c >> 4) * font.height;
//		src.w = font.width;
//		src.h = font.height;
//
//		SDL_RenderTexture(render.renderer, font.texture, &src, &dest);
//		i++;
//	}
//}
//
//void draw_text_centered(const char *text, int y, int scale, uint8_t r, uint8_t g, uint8_t b) {
//	int len = strlen(text);
//	len *= font.width * scale;
//
//	draw_text(text, render.width / 2 - len / 2, y, scale, r, g, b);
//}
//