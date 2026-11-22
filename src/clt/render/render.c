#include "client.h"

renderer_t render = {0};

static int init_sdl() {
	if (SDL_Init(SDL_INIT_VIDEO) == 0) {
		fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
		return 1;
	}

	if (!SDL_CreateWindowAndRenderer("game", 800, 800, SDL_WINDOW_RESIZABLE, &render.window, &render.renderer)) {
		fprintf(stderr, "SDL_CreateWindowAndRenderer: %s\n", SDL_GetError());
		return 1;
	}
	SDL_SetWindowRelativeMouseMode(render.window, 1);
	return 0;
}

static void exit_sdl() {
	SDL_DestroyTexture(render.texture);
	SDL_DestroyRenderer(render.renderer);
	SDL_DestroyWindow(render.window);
	SDL_Quit();

	render.texture = NULL;
	render.renderer = NULL;
	render.window = NULL;
}

void render_exit() {
	exit_sdl();
}

int init_barrier(mutex_barrier_t *barrier);

int render_init() {
	if (init_sdl())
		return 1;
	if (init_barrier(&render.start_barrier))
		return 1;
	if (init_barrier(&render.end_barrier))
		return 1;
	render.kill_thread_mutex = SDL_CreateMutex();
	
	render.fov = 80;
	render.res_mult = 30;
	render.render_distance = 125;
	if (render_set_res_base(16, 9))
		return 1;
	render_set_threads(12);
	return 0;
}

int render_set_res_base(size_t base_width, size_t base_height) {
	size_t old_width = render.base_width;
	size_t old_height = render.base_height;

	render.base_width = base_width;
	render.base_height = base_height;


	if (render_set_res_mult(render.res_mult)) {
		render.base_width = old_width;
		render.base_height = old_height;
		return 1;
	}
	
	return 0;
}

int render_set_res_mult(size_t new_mult) {
	size_t new_width = render.base_width * new_mult;
	size_t new_height = render.base_height * new_mult;

	SDL_Texture *new_texture = SDL_CreateTexture(render.renderer, SDL_PIXELFORMAT_ARGB8888,
									SDL_TEXTUREACCESS_STREAMING, new_width, new_height);
	if (!new_texture)
		return 1;
	
	SDL_SetTextureScaleMode(new_texture, SDL_SCALEMODE_NEAREST);

	
	SDL_DestroyTexture(render.texture);
	render.texture = new_texture;
	render.width = new_width;
	render.res_mult = new_mult;
	render.height = new_height;
	SDL_SetRenderLogicalPresentation(render.renderer, render.width, render.height, SDL_LOGICAL_PRESENTATION_LETTERBOX);
	return 0;
}

void render_reset() {
	SDL_SetRenderDrawColor(render.renderer, 0, 0, 0, 255);
	SDL_RenderClear(render.renderer);
}

void render_update() {
	SDL_RenderPresent(render.renderer);
}

int init_barrier(mutex_barrier_t *barrier) {
	barrier->mutex = SDL_CreateMutex();
	if (!barrier->mutex)
		return 1;
	
	barrier->cond = SDL_CreateCondition();
	if (!barrier->cond) {
		SDL_DestroyMutex(barrier->mutex);
		return 1;
	}
	return 0;
}

void destroy_barrier(mutex_barrier_t *barrier) {
	SDL_DestroyMutex(barrier->mutex);
	SDL_DestroyCondition(barrier->cond);
	barrier->mutex = NULL;
	barrier->cond = NULL;
}

void wait_barrier(mutex_barrier_t *barrier, int force) {

	SDL_LockMutex(barrier->mutex);
	int old_generation = barrier->generation;
	barrier->waiting_count++;

	if (barrier->waiting_count == barrier->waiting_max || force) {
		barrier->waiting_count = 0;
		barrier->generation++;

		SDL_BroadcastCondition(barrier->cond);
	}
	else {
		while (old_generation == barrier->generation)
			SDL_WaitCondition(barrier->cond, barrier->mutex);
	}

	SDL_UnlockMutex(barrier->mutex);
}

int render_slave(void *data) {
	size_t id = (size_t)data;

	while (1) {
		wait_barrier(&render.start_barrier, 0);

		SDL_LockMutex(render.kill_thread_mutex);
		if (render.kill_thread) {
			SDL_UnlockMutex(render.kill_thread_mutex);
			break;
		}
		SDL_UnlockMutex(render.kill_thread_mutex);

		size_t rows = render.height / (render.threads_len + 1);
		size_t y = rows * (id + 1);
		size_t y_end = rows * (id + 2);
		
		if (id == render.threads_len - 1)
			y_end = render.height;

		raycast_draw(y, y_end);

		wait_barrier(&render.end_barrier, 0);
	}
	return 0;
}

void render_compute() {
	if (!SDL_LockTexture(render.texture, NULL, &render.framebuffer, &render.framebuffer_pitch))
		return ;
	
	if (render.threads_len == 0) {
		raycast_draw(0, render.height);
		SDL_UnlockTexture(render.texture);
		SDL_RenderTexture(render.renderer, render.texture, NULL, NULL);
		return ;
	}

	wait_barrier(&render.start_barrier, 0);
	raycast_draw(0, render.height / (render.threads_len + 1));
	wait_barrier(&render.end_barrier, 0);

	SDL_UnlockTexture(render.texture);
	SDL_RenderTexture(render.renderer, render.texture, NULL, NULL);
}

int render_set_threads(size_t number) {
	if (render.threads_len) {
		SDL_LockMutex(render.kill_thread_mutex);
		render.kill_thread = 1;
		SDL_UnlockMutex(render.kill_thread_mutex);

		wait_barrier(&render.start_barrier, 0);
		for (size_t i = 0; i < render.threads_len; i++)
			SDL_WaitThread(render.threads[i], NULL);
		render.kill_thread = 0;
		free(render.threads);
		render.threads_len = 0;
	}

	render.threads = malloc(sizeof(SDL_Thread *) * number);
	if (!render.threads)
		return 1;
	render.start_barrier.waiting_max = number + 1;
	render.start_barrier.waiting_count = 0;
	render.start_barrier.generation = 0;
	render.end_barrier.waiting_max = number + 1;
	render.end_barrier.waiting_count = 0;
	render.end_barrier.generation = 0;
	render.threads_len = number;

	for (size_t i = 0; i < number; i++)
		render.threads[i] = SDL_CreateThread(render_slave, "raycast-slave", (void *)i);
	
	return 0;
}



/*

if (a || b) {


	if (!a && b) {
		
	}
}

*/
