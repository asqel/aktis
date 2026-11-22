#include <stdio.h>
#include <SDL3/SDL.h>
#include <common.h>
#include "raisin.h"
#include "client.h"

#define FPS_MS (1000 / 60)

void place(int x, int y, int z) {
	clt.world.blocks[z][y][x] = 1;
}

void place_tree(int x, int y, int z) {
	place(x, y, z);
	place(x, y + 1, z);
	place(x, y + 2, z);
	place(x, y + 3, z);
	place(x, y + 4, z);

	place(x + 1, y + 3, z);
	place(x, y + 3, z + 1);
	place(x - 1, y + 3, z);
	place(x, y + 3, z - 1);

	place(x + 1, y + 3, z + 1);
	place(x - 1, y + 3, z - 1);
	place(x - 1, y + 3, z + 1);
	place(x + 1, y + 3, z - 1);

}

int main() {
	if (ra_init())
		return 1;
	if (render_init()) {
		ra_exit();
		return 1;
	}

	if (init_font()) {
		render_exit();
		ra_exit();
		return 1;
	}
	render.y = 2;
	render.x = 2;
	render.z = 2;
	render.rot_x = 0;
	render.rot_y = M_PI / 4;


	for (int i = 0; i < WORLD_SIZE; i++) {
		for (int k = 0; k < WORLD_SIZE; k++) {
			clt.world.blocks[i][0][k] = 1;
			clt.world.blocks[i][k][0] = 1;
			clt.world.blocks[0][k][i] = 1;
			clt.world.blocks[i][WORLD_SIZE - 1][k] = 1;
			clt.world.blocks[i][k][WORLD_SIZE - 1] = 1;
			clt.world.blocks[WORLD_SIZE - 1][k][i] = 1;

		}
		clt.world.blocks[i][i][i] = 1;
	}
	//for (int i = 0; i < WORLD_SIZE; i++) {
	//	for (int k = 0; k < WORLD_SIZE; k++) {
	//		clt.world.blocks[i][k][20] = 1;
	//	}
	//}
	place_tree(10, 1, 10);
	place_tree(15, 1, 10);
	place_tree(10, 1, 15);
	while (1) {
		uint32_t t0 = SDL_GetTicks();

		SDL_Event ev = {0};
		while (SDL_PollEvent(&ev)) {
			if (ev.type == SDL_EVENT_QUIT)
				goto end;
			if (ev.type == SDL_EVENT_MOUSE_MOTION) {
				render.rot_x += ev.motion.yrel * 0.002;
				render.rot_y -= ev.motion.xrel * 0.002;
			}
			if (ev.type == SDL_EVENT_KEY_DOWN) {
				if (ev.key.scancode == SDL_SCANCODE_LEFT)
					render.rot_y -= 0.05;
				else if (ev.key.scancode == SDL_SCANCODE_RIGHT)
					render.rot_y += 0.05;
				if (ev.key.scancode == SDL_SCANCODE_UP)
					render.rot_x -= 0.05;
				if (ev.key.scancode == SDL_SCANCODE_DOWN)
					render.rot_x += 0.05;
				if (ev.key.scancode == SDL_SCANCODE_SPACE)
					render.y += 0.2;
				if (ev.key.scancode == SDL_SCANCODE_LSHIFT)
					render.y -= 0.2;
				if (ev.key.scancode == SDL_SCANCODE_W || ev.key.scancode == SDL_SCANCODE_S) {
					float rot_x = 0;
					float rot_y = render.rot_y;
					
					float yaw_sin = sinf(rot_y);
					float yaw_cos = cosf(rot_y);
					float pitch_sin = sinf(rot_x);
					float pitch_cos = cosf(rot_x);
					float forward_x = pitch_cos * yaw_cos;
					float forward_y = pitch_sin;
					float forward_z = pitch_cos * yaw_sin;

					float speed = 5;
					if (ev.key.scancode == SDL_SCANCODE_W) {
						render.x += forward_x * 0.05 * speed;
						render.y += forward_y * 0.05 * speed;
						render.z += forward_z * 0.05 * speed;
					}
					else {
						render.x -= forward_x * 0.05 * speed;
						render.y -= forward_y * 0.05 * speed;
						render.z -= forward_z * 0.05 * speed;
					}
				}
			}
			clt_on_key(&ev);
		}

		client_update();

		render_reset();
		client_draw();
		render_update();

		uint32_t frame_time = SDL_GetTicks() - t0;
		if (frame_time)
			printf("%u\n", 1000 / frame_time);
		if (frame_time < FPS_MS)
			SDL_Delay(FPS_MS - frame_time);
	}

	end:
		return 0;
}
