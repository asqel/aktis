#include "client.h"

renderer_t render = {0};

int upload_gpu_map()
{
	const size_t total_chunk = render.render_distance * render.render_distance * render.render_distance;    // nombre de chunks
	const size_t bytes_per_chunk = (size_t)CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE * sizeof(uint32_t);

	// 2. Copier chaque chunk à sa place : le chunk i va à l'offset i * CHUNK_BYTES,
	//    exactement comme le calcule le shader (chunkIdx << 12).
	uint8_t *ptr = SDL_MapGPUTransferBuffer(render.dev, render.world_transfer_buffer, true);
	if (!ptr) {
		SDL_Log("upload_gpu_map: map: %s", SDL_GetError());
		SDL_ReleaseGPUTransferBuffer(render.dev, render.world_transfer_buffer);
		return 1;
	}
	for (size_t i = 0; i < total_chunk; i++) {
		uint8_t *dst = ptr + i * bytes_per_chunk;
		if (clt.world.chunks[i].blocks) {
			SDL_memcpy(dst, *clt.world.chunks[i].blocks, bytes_per_chunk);
			printf("OU LA LA LA LOCK %d\n", (*clt.world.chunks[i].blocks)[0]);
		}
		else
			SDL_memset(dst, 0, bytes_per_chunk);    // chunk vide = air
	}
	SDL_UnmapGPUTransferBuffer(render.dev, render.world_transfer_buffer);

	// 3. Une seule copie : transfer buffer -> buffer GPU de la map
	SDL_GPUCommandBuffer *cmd  = SDL_AcquireGPUCommandBuffer(render.dev);
	SDL_GPUCopyPass      *copy = SDL_BeginGPUCopyPass(cmd);

	SDL_GPUTransferBufferLocation src = {
		.transfer_buffer = render.world_transfer_buffer,
		.offset = 0
	};
	SDL_GPUBufferRegion dst = {
		.buffer = render.world_buffer,      // ton SDL_GPUBuffer (COMPUTE_STORAGE_READ)
		.offset = 0,
		.size = (Uint32)bytes_per_chunk * total_chunk,
	};
	SDL_UploadToGPUBuffer(copy, &src, &dst, false);

	SDL_EndGPUCopyPass(copy);
	SDL_SubmitGPUCommandBuffer(cmd);

	// 4. On attend la fin de la copie avant de libérer le buffer temporaire
	SDL_WaitForGPUIdle(render.dev);
	SDL_ReleaseGPUTransferBuffer(render.dev, render.world_transfer_buffer);
	return 0;
}

static int init_gpu_map_buffer(void) {
	const size_t total_chunk = render.render_distance * render.render_distance * render.render_distance;    // nombre de chunks
	const size_t bytes_per_chunk = (size_t)CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE * sizeof(uint32_t);

	SDL_GPUBufferCreateInfo bci = {0};
	bci.usage = SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ;
	bci.size = (uint32_t)(total_chunk * bytes_per_chunk);
	render.world_buffer = SDL_CreateGPUBuffer(render.dev, &bci);
	if (!render.world_buffer) {
		SDL_Log("SDL_CreateGPUBuffer, error: %s", SDL_GetError());
		return 1;
	}
	SDL_GPUTransferBufferCreateInfo tbci = {0};
	tbci.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
	tbci.size = (uint32_t)(total_chunk * bytes_per_chunk);
	render.world_transfer_buffer = SDL_CreateGPUTransferBuffer(render.dev, &tbci);
	if (!render.world_transfer_buffer) {
		SDL_Log("SDL_CreateGPUTransferBuffer, error: %s", SDL_GetError());
		return 1;
	}
	return 0;
}

static int init_raycast_pipeline(void) {
	size_t code_size = 0;
	void *code = SDL_LoadFile("src/clt/shaders/build/raycast.comp.spv", &code_size);
	SDL_GPUComputePipelineCreateInfo cpi = {0};
	cpi.code = (const uint8_t *)code;
	cpi.code_size = code_size;
	cpi.entrypoint = "main";
	cpi.format = SDL_GPU_SHADERFORMAT_SPIRV;
	cpi.num_readonly_storage_buffers = 1;
	cpi.num_readwrite_storage_textures = 1;
	cpi.num_uniform_buffers = 1;
	cpi.threadcount_x = 8;
	cpi.threadcount_y = 8;
	cpi.threadcount_z = 1;
	render.raycast_pipeline = SDL_CreateGPUComputePipeline(render.dev, &cpi);
	SDL_free(code);
	if (!render.raycast_pipeline) {
		SDL_Log("SDL_CreateGPUComputePipeline, error: %s", SDL_GetError());
		return 1;
	} 

	SDL_GPUTextureCreateInfo tci = {0};
	tci.type = SDL_GPU_TEXTURETYPE_2D;
	tci.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
	tci.usage = SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_WRITE | SDL_GPU_TEXTUREUSAGE_SAMPLER;
	tci.width = render.base_width * render.res_mult;
	tci.height = render.base_height * render.res_mult;
	tci.layer_count_or_depth = 1;
	tci.num_levels = 1;
	render.raycast_texture = SDL_CreateGPUTexture(render.dev, &tci);
	if (!render.raycast_texture) {
		SDL_Log("SDL_CreateGPUTexture, error: %s", SDL_GetError());
		return 1;
	}
	return 0;
}

int init_render() {

render.base_width = (1920/4);
render.base_height = (1080/4);
render.res_mult = 1;

	int res = 0;
	res = SDL_Init(SDL_INIT_VIDEO);
	if (!res) {
		SDL_Log("SDL_Init, error: %s", SDL_GetError());
		return 1;
	}
	render.win = SDL_CreateWindow("game", 800, 800, SDL_WINDOW_RESIZABLE);
	if (!render.win) {
		SDL_Log("SDL_CreateWindow, error: %s", SDL_GetError());
		return 1;
	}
	SDL_GPUVulkanOptions vk_ops = {0};
	vk_ops.vulkan_api_version = (1u << 22) | (2u << 12);

	SDL_PropertiesID props = SDL_CreateProperties();
	SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_SPIRV_BOOLEAN, true);
	SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_DEBUGMODE_BOOLEAN, true);
	SDL_SetPointerProperty(props, SDL_PROP_GPU_DEVICE_CREATE_VULKAN_OPTIONS_POINTER, &vk_ops);
	render.dev = SDL_CreateGPUDeviceWithProperties(props);
	SDL_DestroyProperties(props);
	if (!render.dev) {
		SDL_Log("SDL_CreateGPUDevice, error: %s", SDL_GetError());
		return 1;
	}
	res = SDL_ClaimWindowForGPUDevice(render.dev, render.win);
	if(!res) {
		SDL_Log("SDL_ClaimWindowForGPUDevice, error: %s", SDL_GetError());
		return 1;
	}

	SDL_SetGPUSwapchainParameters(render.dev, render.win, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC);


	SDL_SetWindowRelativeMouseMode(render.win, 1);

	res = init_raycast_pipeline();
	if (res) {
		SDL_Log("init_raycast_pipeline, error: %s", SDL_GetError());
		return 1;
	}

	res = init_gpu_map_buffer();
	if (res) {
		SDL_Log("init_gpu_map_buffer, error: %s", SDL_GetError());
		return 1;
	}

	upload_gpu_map();

	return 0;
}