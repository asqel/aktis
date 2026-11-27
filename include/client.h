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

} client_t;

typedef struct {
	SDL_Mutex *mutex;
	SDL_Condition *cond;
	int waiting_count;
	int waiting_max;
	int generation;
} mutex_barrier_t;

//void wait_barriere(mutex_barrier_t *barrier);

typedef struct {
	SDL_GPUTexture *tex;
	Uint32 width;
	Uint32 height;
} GPUTexture_t;

typedef struct {
	float x;
	float y;
	float z;
	// rot_X
	float pitch;
	// rot_y 
	float yaw;
	float pitch_sin;
	float pitch_cos;
	float yaw_sin;
	float yaw_cos;
	float forward_x;
	float forward_y;
	float forward_z;
	float right_x;
	float right_y;
	float right_z;
	float fov;
} camera_t;


typedef struct {
	camera_t cam;
	size_t render_distance;

	size_t width;
	size_t height;
	
	size_t base_width;
	size_t base_height;
	
	SDL_Window *win;
	SDL_GPUDevice *dev;
	
	SDL_GPUComputePipeline *raycast_pipeline;
	SDL_GPUTexture *raycast_texture;
	SDL_GPUBuffer *world_buffer;
	SDL_GPUTransferBuffer *world_transfer_buffer;
	
	GPUTexture_t *atlas;
	
	SDL_GPUSampler *img_sampler;
	SDL_GPUGraphicsPipeline *img_pipeline;
	
	GPUTexture_t *font;

	SDL_GPUCommandBuffer *gpu_cmd;
	GPUTexture_t swap;
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

//void raycast_draw(size_t y_start, size_t y_end);

extern renderer_t render;

extern client_t clt;

enum {
	CLT_START,
	CLT_LOADING,
	CLT_INGAME,
	CLT_FAIL,
};

int render_start();
void render_end();
int render_raycast(SDL_GPUCommandBuffer *cmd, GPUTexture_t *swap);
void render_texture(GPUTexture_t *texture, SDL_FRect *src, SDL_FRect *dest, uint32_t tint);

void client_draw();
void client_update();

SDL_GPUTexture *load_gpu_texture(SDL_GPUDevice *device, SDL_GPUCopyPass *copy_pass, const char *file, int *width, int *height);
SDL_GPUShader *load_shader(SDL_GPUDevice *gpu, char *shader_file, Uint32 num_uniform_buf, Uint32 num_samplers);
int init_render();
int render_draw(void);

//void render_compute();
//int render_set_threads(size_t number);

void clt_on_connect(ra_clt_ctx_t *ctx, int success);
void clt_on_disconnect(ra_clt_ctx_t *ctx, int is_error);
void clt_on_recv(ra_clt_ctx_t *ctx, void *data, size_t data_len);
void clt_on_send_ready(ra_clt_ctx_t *ctx);

//int init_font();
//void exit_font();
void draw_text(const char *str, int x, int y, double scale, uint8_t r, uint8_t g, uint8_t b);
void draw_text_centered(const char *text, int y, int scale, uint8_t r, uint8_t g, uint8_t b);

void clt_on_key(SDL_Event *ev);

void client_fail(const char *msg);
void client_draw_fail();

typedef struct {
	uint8_t is_special;
	float aabb_min[3];
	float aabb_max[3];
} block_model_t;

extern block_model_t block_models[4096];
extern uint32_t block_count;

void register_block_model(uint16_t id, float min_x, float min_y, float min_z, float max_x, float max_y, float max_z);

#endif
