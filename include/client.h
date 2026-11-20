#ifndef CLIENT_H
#define CLIENT_H

#include "common.h"
#include "raisin.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <stdio.h>
#include <math.h>

#if defined(_WIN32) || defined(__linux__) || defined(__APPLE__)
	#define USE_THREAD
#endif

typedef struct {
	ra_clt_ctx_t sock;
	world_t world;
	int state;
	int has_sock;
	buffer_t recv;
	buffer_t send;
	char name[7];
	size_t ent_idx;
	double x; // this is always update the same as entities[ent_idx].*
	double y;
	double z;
	double rot_x;
	double rot_y;
} client_t;

typedef struct {
	SDL_Mutex *mutex;
	SDL_Condition *cond;
	int waiting_count;
	int waiting_max;
	int generation;
} mutex_barrier_t;

void wait_barriere(mutex_barrier_t *barrier);

typedef struct {
	double x;
	double y;
	double z;
	double rot_x;
	double rot_y;
	double fov;
	size_t render_distance;
	SDL_Texture *texture;
	void *framebuffer;
	int framebuffer_pitch;

	size_t width;
	size_t height;

	size_t res_mult;
	size_t base_width;
	size_t base_height;

	SDL_Renderer *renderer;
	SDL_Window *window;

	mutex_barrier_t start_barrier;
	mutex_barrier_t end_barrier;
	SDL_Thread *threads;
	size_t threads_len;

	SDL_Mutex *kill_thread_mutex;
	uint8_t kill_thread;
} renderer_t;

typedef struct {
	int x;
	int y;
	int z;
	float dir_x;
	float dir_y;
	float dir_z;
	float x_percent;
	float y_percent;
	float dist;
	uint8_t has_hit;
} ray_t;

void raycast_draw(size_t y_start, size_t y_end);

extern renderer_t render;

typedef struct {
	SDL_Texture *texture;
	int width;
	int height;
} font_t;

extern client_t clt;
extern font_t font;

enum {
	CLT_START,
	CLT_LOADING,
	CLT_INGAME,
	CLT_FAIL,
};

void client_draw();
void client_update();


void render_exit();
int render_init();
int render_set_res_base(size_t base_width, size_t base_height);
int render_set_res_mult(size_t new_mult);
void render_reset();
void render_update();

void render_compute();

void clt_on_connect(ra_clt_ctx_t *ctx, int success);
void clt_on_disconnect(ra_clt_ctx_t *ctx, int is_error);
void clt_on_recv(ra_clt_ctx_t *ctx, void *data, size_t data_len);
void clt_on_send_ready(ra_clt_ctx_t *ctx);

int init_font();
void exit_font();
void draw_text(const char *str, int x, int y, double scale, uint8_t r, uint8_t g, uint8_t b);
void draw_text_centered(const char *text, int y, int scale, uint8_t r, uint8_t g, uint8_t b);

void clt_on_key(SDL_Event *ev);

void client_fail(const char *msg);
void client_draw_fail();

enum {
	DIR_NORTH,
	DIR_EAST,
	DIR_SOUTH,
	DIR_WEST,
	DIR_TOP,
	DIR_BOTTOM,
};

#endif
