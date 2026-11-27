#include "client.h"

void draw_loading() {
	static int loading_state = 0;
	static int cooldown = 0;

	char buffer[] = "ca charge en fait ......";
	buffer[strlen(buffer) - loading_state] = '\0'; 

	cooldown++;

	if (cooldown == 30) {
		cooldown = 0;
		loading_state++;
		if (loading_state > 5)
			loading_state = 0;
	}
	
	draw_text(buffer, 50, 50, 2, 0, 255, 0);
}

void draw_start() {
	draw_text_centered("entre ton nom wesh", 50, 1, 0, 255, 0);
	draw_text_centered(clt.name, 100, 2, 0, 255, 0);
}

void client_draw() {
	switch (clt.state) {
		case CLT_START:
			//draw_start();
			render_raycast(render.gpu_cmd, &render.swap);
			break;
		case CLT_LOADING:
			draw_loading();
			break;
		case CLT_FAIL:
			client_draw_fail();
			break;
		case CLT_INGAME:
			render_raycast(render.gpu_cmd, &render.swap);
			break;
		default:
			break;
	}
}
