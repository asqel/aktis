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

int render_raycast(SDL_GPUCommandBuffer *cmd, GPUTexture_t *swap) {
	const float fov = 70.f * (float)M_PI / 180.f;
	const float tanHalf = tanf(fov * 0.5f);
	const float aspect  = (float)render.base_width / (float)render.base_height;

	camera_gpu_t cam;
	cam.pos[0] = render.cam.x;
	cam.pos[1] = render.cam.y;
	cam.pos[2] = render.cam.z;

	cam.fwd[0] = render.cam.forward_x;
	cam.fwd[1] = render.cam.forward_y;
	cam.fwd[2] = render.cam.forward_z;

	cam.right[0] = render.cam.right_x;
	cam.right[1] = render.cam.right_y;
	cam.right[2] = render.cam.right_z;

	cam.up[0] = -render.cam.pitch_sin * render.cam.yaw_cos;
	cam.up[1] = render.cam.pitch_cos;
	cam.up[2] = -render.cam.pitch_sin * render.cam.yaw_sin;

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

	float scale = SDL_min((float)swap->width / render.base_width, (float)swap->height  / render.base_height);

	SDL_GPUBlitInfo blit = {0};
	blit.source.texture	= render.raycast_texture;
	blit.source.w = render.base_width;
	blit.source.h = render.base_height;

	blit.destination.texture = swap->tex;
	blit.destination.w = render.base_width * scale;
	blit.destination.h = render.base_height * scale;

	blit.load_op = SDL_GPU_LOADOP_DONT_CARE;
	blit.filter = SDL_GPU_FILTER_NEAREST;

	blit.destination.x = swap->width / 2 - blit.destination.w / 2;
	blit.destination.y = swap->height / 2 - blit.destination.h / 2;
	SDL_BlitGPUTexture(cmd, &blit);
	return 0;
}

int render_texture(SDL_GPUCommandBuffer *cmd, GPUTexture_t *swap, GPUTexture_t *texture) {
	SDL_GPUColorTargetInfo target_info = {0};
	target_info.texture = swap->tex;
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
	push.screenSize[0] = swap->width;
	push.screenSize[1] = swap->height;
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

SDL_GPUCommandBuffer *cmd = NULL;

int render_draw(void) {
	cmd = SDL_AcquireGPUCommandBuffer(render.dev);
	

    GPUTexture_t swap = {0};
	if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmd, render.win, &swap.tex, &swap.width, &swap.height) || !swap.tex) {
		SDL_SubmitGPUCommandBuffer(cmd);
		return 1;
	}
	
	render_raycast(cmd, &swap);
	render_texture(cmd, &swap, render.font);

	if (!SDL_SubmitGPUCommandBuffer(cmd)) {
		SDL_Log("Submit : %s", SDL_GetError());
	}
    cmd = NULL;
	return 0;
}