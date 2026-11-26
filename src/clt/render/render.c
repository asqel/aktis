#include "client.h"
#include <math.h>

typedef struct {
	float pos[4];
	float fwd[4];
	float right[4];
	float up[4];
	float A[2];
	float B[2];
} camera_gpu_t;

typedef struct {
	float position[2];
	float size[2];
	float uvPosition[2];
	float uvSize[2];
	float screenSize[2];
	float _pad[2];
	float color[4];
} sprite_gpu_t;

int render_raycast(SDL_GPUCommandBuffer *cmd, SDL_GPUTexture *swap, Uint32 sw, Uint32 sh) {
	const float fov = 70.f * (float)M_PI / 180.f;
	const float tanHalf = tanf(fov * 0.5f);
	const float aspect  = (float)render.base_width / (float)render.base_height;

	float yaw_sin = sinf(render.rot_y);
	float yaw_cos = cosf(render.rot_y);
	float pitch_sin = sinf(render.rot_x);
	float pitch_cos = cosf(render.rot_x);

	camera_gpu_t cam;
	cam.pos[0] = render.x;
	cam.pos[1] = render.y;
	cam.pos[2] = render.z;

	cam.fwd[0] = pitch_cos * yaw_cos;
	cam.fwd[1] = pitch_sin;
	cam.fwd[2] = pitch_cos * yaw_sin;

	cam.right[0] = -yaw_sin;
	cam.right[1] = 0;
	cam.right[2] = yaw_cos;

	cam.up[0] = -pitch_sin * yaw_cos;
	cam.up[1] = pitch_cos;
	cam.up[2] = -pitch_sin * yaw_sin;

	cam.A[0] = 2.0 * aspect * tanHalf / render.base_width;
	cam.A[1] = -2.0 * tanHalf / render.base_height;
	cam.B[0] = aspect * tanHalf / render.base_width - aspect * tanHalf;
	cam.B[1] = -tanHalf / render.base_height + tanHalf;

	SDL_PushGPUComputeUniformData(cmd, 0, &cam, sizeof(cam));

	SDL_GPUStorageTextureReadWriteBinding outBind = {0};
	outBind.texture = render.raycast_texture;
	outBind.cycle   = true;

	SDL_GPUComputePass *pass = SDL_BeginGPUComputePass(cmd, &outBind, 1, NULL, 0);
	SDL_BindGPUComputePipeline(pass, render.raycast_pipeline);
	SDL_BindGPUComputeStorageTextures(pass, 0, &render.atlas->tex, 1);
	SDL_BindGPUComputeStorageBuffers(pass, 0, &render.world_buffer, 1);
	SDL_DispatchGPUCompute(pass, (render.base_width + 7) / 8, (render.base_height + 7) / 8, 1);
	SDL_EndGPUComputePass(pass);

	float scale = SDL_min((float)sw / render.base_width, (float)sh / render.base_height);

	SDL_GPUBlitInfo blit = {0};
	blit.source.texture	= render.raycast_texture;
	blit.source.w = render.base_width;
	blit.source.h = render.base_height;

	blit.destination.texture = swap;
	blit.destination.w = render.base_width * scale;
	blit.destination.h = render.base_height * scale;

	blit.load_op = SDL_GPU_LOADOP_DONT_CARE;
	blit.filter = SDL_GPU_FILTER_NEAREST;

	blit.destination.x = sw / 2 - blit.destination.w / 2;
	blit.destination.y = sh / 2 - blit.destination.h / 2;
	SDL_BlitGPUTexture(cmd, &blit);
	return 0;
}

int render_texture(SDL_GPUCommandBuffer *cmd, SDL_GPUTexture *swap, uint32_t sw, uint32_t sh, GPUTexture_t *texture) {
	SDL_GPUColorTargetInfo target_info = {0};
	target_info.texture = swap;
	target_info.load_op = SDL_GPU_LOADOP_LOAD;
	target_info.store_op = SDL_GPU_STOREOP_STORE;

	SDL_GPURenderPass *render_pass = SDL_BeginGPURenderPass(cmd, &target_info, 1, NULL);
	if (!render_pass)
		SDL_Log("error render_pass, %s\n", SDL_GetError());

	char c = '1';
	float cw = 10.0f;
	float ch = 20.0f;

	float src_x = (c & 0xF) * cw;
	float src_y = (c >> 4)  * ch;

	sprite_gpu_t push = {0};
	push.position[0] = 200;
	push.position[1] = 100;
	push.size[0] = cw * 10;
	push.size[1] = ch * 10;
	push.uvPosition[0] = src_x / (float)texture->width;
	push.uvPosition[1] = src_y / (float)texture->height;
	push.uvSize[0] = cw / texture->width;
	push.uvSize[1] = ch / texture->height;
	push.screenSize[0] = sw;
	push.screenSize[1] = sh;
	push.color[0] = 1;
	push.color[1] = 0;
	push.color[2] = 1;
	push.color[3] = 1;

	SDL_BindGPUGraphicsPipeline(render_pass, render.img_pipeline);
	SDL_PushGPUVertexUniformData(cmd, 0, &push, sizeof(push));
	SDL_GPUTextureSamplerBinding binding = {
		.texture = texture->tex,
		.sampler = render.img_sampler
	};
	SDL_BindGPUFragmentSamplers(render_pass, 0, &binding, 1);
	SDL_DrawGPUPrimitives(render_pass, 6, 1, 0, 0);
	SDL_EndGPURenderPass(render_pass);
	return 0;
}

int render_draw(void) {
	
	SDL_GPUCommandBuffer *cmd = SDL_AcquireGPUCommandBuffer(render.dev);
	SDL_GPUTexture *swap = NULL;
	Uint32 sw = 0, sh = 0;
	if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmd, render.win, &swap, &sw, &sh) || !swap) {
		SDL_SubmitGPUCommandBuffer(cmd);
		return 1;
	}
	
	render_raycast(cmd, swap, sw, sh);
	render_texture(cmd, swap, sw, sh, render.font);

	if (!SDL_SubmitGPUCommandBuffer(cmd)) {
		SDL_Log("Submit : %s", SDL_GetError());
	}
	return 0;
}