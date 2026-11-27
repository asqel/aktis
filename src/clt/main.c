#include <stdio.h>
#include <SDL3/SDL.h>
#include <common.h>
#include "raisin.h"
#include "client.h"
#include <malloc.h>

#define FPS_MS (1000 / 60)

void place(int x, int y, int z, uint32_t block) {
	if (x < 0 || y < 0 || z < 0)
		return ;
	size_t cx = COORD_CUHNK(x);
	size_t cy = COORD_CUHNK(y);
	size_t cz = COORD_CUHNK(z);
	if (cx >= clt.world.width || cy >= clt.world.height || cz >= clt.world.width)
		return ;
	chunk_t *chunk = &clt.world.chunks[CHUNK_OFFSET(cx, cy, cz, clt.world.width)];
	if (!chunk->blocks) {
		chunk->blocks = malloc(sizeof(uint32_t) * CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE);
		if (!chunk->blocks)
			return ;
		for (size_t i = 0; i < CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE; i++)
			(*chunk->blocks)[i] = 0;
	}
	(*chunk->blocks)[COORD_TO_BLOCK(x, y, z)] = block;
	clt.world.dirty = 1;
	chunk->dirty = 1;
}

void place_tree(int x, int y, int z) {
	place(x, y, z, 1);
	place(x, y + 1, z, 1);
	place(x, y + 2, z, 1);
	place(x, y + 3, z, 1);
	place(x, y + 4, z, 1);

	place(x + 1, y + 3, z, 5);
	place(x, y + 3, z + 1, 5);
	place(x - 1, y + 3, z, 5);
	place(x, y + 3, z - 1, 5);

	place(x + 1, y + 3, z + 1, 1);
	place(x - 1, y + 3, z - 1, 1);
	place(x - 1, y + 3, z + 1, 1);
	place(x + 1, y + 3, z - 1, 1);

}

int client_set_world_size(size_t width, size_t height) {
	render.render_distance = width;
	clt.world.chunks = malloc(sizeof(chunk_t) * width * width * height);
	if (!clt.world.chunks)
		return 1;
	
	clt.world.width = width;
	clt.world.height = height;
	clt.world.dirty = 0;

	for (size_t i = 0; i < width * height * width; i++) {
		clt.world.chunks[i].blocks = NULL;
		clt.world.chunks[i].entitites_alloc = 0;
		clt.world.chunks[i].entities_len = 0;
		clt.world.chunks[i].entities = NULL;
		clt.world.chunks[i].dirty = 0;
	}

	return 0;
}

int main() {
	init_blocks();
	client_set_world_size(2, 2);
	place_tree(10, 0, 10);
	for (int i = 0; i < clt.world.width * 16; i++) {
		for (int k = 0; k < clt.world.width * 16; k++) {
			place(i, 0, k, 1);
		}
	}
	//if (ra_init())
	//return 1;
	if (init_render()) {
		ra_exit();
		return 1;
	}

	render.cam.y = 2;
	render.cam.x = 2;
	render.cam.z = 2;
	render.cam.pitch = 0;
	render.cam.yaw = M_PI / 4;
	render.cam.pitch_cos = cosf(render.cam.pitch);
	render.cam.pitch_sin = sinf(render.cam.pitch);
	render.cam.yaw_cos = cosf(render.cam.yaw);
	render.cam.yaw_sin = sinf(render.cam.yaw);
	render.cam.forward_x = render.cam.pitch_cos * render.cam.yaw_cos;
	render.cam.forward_y = render.cam.pitch_sin;
	render.cam.forward_z = render.cam.pitch_cos * render.cam.yaw_sin;
	render.cam.right_x = -render.cam.yaw_sin;
	render.cam.right_y = 0;
	render.cam.right_z = render.cam.yaw_cos;

	while (1) {
		uint32_t t0 = SDL_GetTicks();

		SDL_Event ev = {0};
		while (SDL_PollEvent(&ev)) {
			if (ev.type == SDL_EVENT_QUIT)
				goto end;
			if (ev.type == SDL_EVENT_KEY_DOWN) {
				if (ev.key.scancode == SDL_SCANCODE_ESCAPE)
					SDL_SetWindowRelativeMouseMode(render.win, !SDL_GetWindowRelativeMouseMode(render.win));
				if (ev.key.scancode == SDL_SCANCODE_P)
					place(render.cam.x, render.cam.y, render.cam.z, 5 | ((rand() % 4) << 12));
			}
			if (ev.type == SDL_EVENT_MOUSE_MOTION) {
				render.cam.pitch -= ev.motion.yrel * 0.002;
				render.cam.yaw += ev.motion.xrel * 0.002;
				render.cam.pitch_cos = cosf(render.cam.pitch);
				render.cam.pitch_sin = sinf(render.cam.pitch);
				render.cam.yaw_cos = cosf(render.cam.yaw);
				render.cam.yaw_sin = sinf(render.cam.yaw);
				render.cam.forward_x = render.cam.pitch_cos * render.cam.yaw_cos;
				render.cam.forward_y = render.cam.pitch_sin;
				render.cam.forward_z = render.cam.pitch_cos * render.cam.yaw_sin;
				render.cam.right_x = -render.cam.yaw_sin;
				render.cam.right_y = 0;
				render.cam.right_z = render.cam.yaw_cos;

			}
			clt_on_key(&ev);
		}
		const bool *k = SDL_GetKeyboardState(NULL);
		if (k[SDL_SCANCODE_W] || k[SDL_SCANCODE_S] || k[SDL_SCANCODE_A] || k[SDL_SCANCODE_D]) {
			
			float forward_x = 1 * render.cam.yaw_cos;
			float forward_y = 0;
			float forward_z = 1 * render.cam.yaw_sin;
			float right_x = -forward_z ;
			float right_y = 0.0f;
			float right_z = forward_x;

			float speed = 0.25;

			if (k[SDL_SCANCODE_W]) {
				render.cam.x += forward_x * speed;
				render.cam.y += forward_y * speed;
				render.cam.z += forward_z * speed;
			}
			if (k[SDL_SCANCODE_S]) {
				render.cam.x -= forward_x * speed;
				render.cam.y -= forward_y * speed;
				render.cam.z -= forward_z * speed;
			}
			if (k[SDL_SCANCODE_A]) {
				render.cam.x -= right_x * speed;
				render.cam.y -= right_y * speed;
				render.cam.z -= right_z * speed;
			}
			if (k[SDL_SCANCODE_D]) {
				render.cam.x += right_x * speed;
				render.cam.y += right_y * speed;
				render.cam.z += right_z * speed;
			}
		}
		if (k[SDL_SCANCODE_SPACE]) {
			render.cam.y += 0.2;
		}
		if (k[SDL_SCANCODE_LSHIFT]) {
			render.cam.y -= 0.2;
		}

		client_update();
		
		render_start();
		client_draw();
		render_end();

		uint32_t frame_time = SDL_GetTicks() - t0;
		if (frame_time)
			printf("%u\n", 1000 / frame_time);
		malloc_stats();

		//if (frame_time < FPS_MS)
			//SDL_Delay(FPS_MS - frame_time);
	}

	end:
		return 0;
}
