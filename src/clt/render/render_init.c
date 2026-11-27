#include "client.h"

renderer_t render = {0};

static int init_gpu_map_buffer(void) {
	const uint32_t total_chunk = render.render_distance * render.render_distance * render.render_distance;
	const uint32_t bytes_per_chunk = (size_t)CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE * sizeof(uint32_t);

	SDL_GPUBufferCreateInfo blocks_bci = {
		.usage = SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ,
		.size = total_chunk * bytes_per_chunk,
	};
	render.blocks_buffer = SDL_CreateGPUBuffer(render.dev, &blocks_bci);
	if (!render.blocks_buffer) {
		SDL_Log("SDL_CreateGPUBuffer, error: %s", SDL_GetError());
		return 0;
	}

	SDL_GPUBufferCreateInfo chunk_bci = {
		.usage = SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ,
		.size = sizeof(uint32_t) * total_chunk,
	};
	render.chunks_buffer = SDL_CreateGPUBuffer(render.dev, &chunk_bci);
	if (!render.chunks_buffer) {
		SDL_Log("SDL_CreateGPUBuffer, error: %s", SDL_GetError());
		return 0;
	}

	SDL_GPUTransferBufferCreateInfo blocks_tbci = {
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.size = bytes_per_chunk,
	};
	render.blocks_transfer_buffer = SDL_CreateGPUTransferBuffer(render.dev, &blocks_tbci);
	if (!render.blocks_transfer_buffer) {
		SDL_Log("SDL_CreateGPUTransferBuffer, error: %s", SDL_GetError());
		return 0;
	}

	SDL_GPUTransferBufferCreateInfo tbci = {0};
	tbci.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
	tbci.size = (uint32_t)bytes_per_chunk;
	render.chunks_transfer_buffer = SDL_CreateGPUTransferBuffer(render.dev, &tbci);
	if (!render.chunks_transfer_buffer) {
		SDL_Log("SDL_CreateGPUTransferBuffer, error: %s", SDL_GetError());
		return 0;
	}
	return 1;
}


static GPUTexture_t *init_gpu_texture_buffer(char *path) {
	GPUTexture_t *res = SDL_malloc(sizeof(GPUTexture_t));

	SDL_GPUCommandBuffer *cmd_buff = SDL_AcquireGPUCommandBuffer(render.dev);
	SDL_GPUCopyPass *cp = SDL_BeginGPUCopyPass(cmd_buff);
	res->tex = load_gpu_texture(render.dev, cp, path, (int *)&res->width, (int *)&res->height);
	if (!res->tex) {
		SDL_Log("error loading texture");
		return NULL;
	}
	SDL_EndGPUCopyPass(cp);
	SDL_SubmitGPUCommandBuffer(cmd_buff);
	return res;
}


static int init_models_buffers() {

	SDL_GPUBufferCreateInfo idx_list_bci = {
		.usage = SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ,
		.size = sizeof(uint32_t) * block_count,
	};
	render.block_idx_buffer = SDL_CreateGPUBuffer(render.dev, &idx_list_bci);

	size_t model_count = 0;
	for (size_t i = 0; i < block_count; i++) {
		if (block_models[i].is_special)
			model_count++;
	}
	SDL_GPUBufferCreateInfo model_bci = {
		.usage = SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ,
		.size = sizeof(float) * 6 * model_count,
	};
	render.block_models_buffer = SDL_CreateGPUBuffer(render.dev, &model_bci);

	if (!render.block_models_buffer || !render.block_idx_buffer) {
		SDL_ReleaseGPUBuffer(render.dev, render.block_models_buffer); 
		SDL_ReleaseGPUBuffer(render.dev, render.block_idx_buffer); 
		SDL_Log("SDL_CreateGPUBuffer, error: %s", SDL_GetError());
		return 0;
	}


	SDL_GPUTransferBufferCreateInfo tbci = {
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.size = idx_list_bci.size + model_bci.size
	};
	SDL_GPUTransferBuffer *transfer_buffer = SDL_CreateGPUTransferBuffer(render.dev, &tbci);
	if (!transfer_buffer) {
		SDL_Log("SDL_CreateGPUTransferBuffer, error: %s", SDL_GetError());
		return 0;
	}

	uint8_t *ptr = SDL_MapGPUTransferBuffer(render.dev, transfer_buffer, true);
	if (!ptr) {
		SDL_Log("upload_gpu_map: map: %s", SDL_GetError());
		SDL_ReleaseGPUTransferBuffer(render.dev, transfer_buffer);
		return 0;
	}
	uint32_t *blocks_idx = (void *)ptr;
	float *block_models_data = (void *)(ptr + idx_list_bci.size);
	uint32_t current = 0;
	for (size_t i = 0; i < block_count; i++) {
		if (block_models[i].is_special) {
			blocks_idx[i] = current;
			memcpy(&block_models_data[current], block_models[i].aabb_min, sizeof(float) * 3);
			memcpy(&block_models_data[current + 3], block_models[i].aabb_max, sizeof(float) * 3);

			current += 6;
		}
		else
			blocks_idx[i] = (uint32_t)-1;
	}
	SDL_UnmapGPUTransferBuffer(render.dev, transfer_buffer);

	SDL_GPUCommandBuffer *cmd  = SDL_AcquireGPUCommandBuffer(render.dev);
	SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(cmd);

	SDL_GPUTransferBufferLocation src;
	SDL_GPUBufferRegion dst;

	src.transfer_buffer = transfer_buffer;
	src.offset = 0;

	dst.buffer = render.block_idx_buffer;
	dst.offset = 0;
	dst.size = idx_list_bci.size;
	SDL_UploadToGPUBuffer(copy, &src, &dst, false);

	src.offset = idx_list_bci.size;

	dst.buffer = render.block_models_buffer,
	dst.offset = 0,
	dst.size = model_bci.size;
	SDL_UploadToGPUBuffer(copy, &src, &dst, false);

	SDL_EndGPUCopyPass(copy);
	SDL_SubmitGPUCommandBuffer(cmd);

	SDL_WaitForGPUIdle(render.dev);
	SDL_ReleaseGPUTransferBuffer(render.dev, transfer_buffer);

	return 0;
}

static int init_raycast_pipeline(void) {
	size_t code_size = 0;
	void *code = SDL_LoadFile("raycast.comp.spv", &code_size);
	if (!code) {
		SDL_Log("SDL_LoadFile, error: %s", SDL_GetError());
		return 0;
	}
	SDL_GPUComputePipelineCreateInfo cpi = {0};
	cpi.code = (const uint8_t *)code;
	cpi.code_size = code_size;
	cpi.entrypoint = "main";
	cpi.format = SDL_GPU_SHADERFORMAT_SPIRV;
	cpi.num_readonly_storage_textures = 1;
	cpi.num_readonly_storage_buffers = 3;
	cpi.num_readwrite_storage_textures = 1;
	cpi.num_uniform_buffers = 1;
	cpi.threadcount_x = 8;
	cpi.threadcount_y = 8;
	cpi.threadcount_z = 1;
	render.raycast_pipeline = SDL_CreateGPUComputePipeline(render.dev, &cpi);
	SDL_free(code);
	if (!render.raycast_pipeline) {
		SDL_Log("SDL_CreateGPUComputePipeline, error: %s", SDL_GetError());
		return 0;
	}

	SDL_GPUTextureCreateInfo tci = {0};
	tci.type = SDL_GPU_TEXTURETYPE_2D;
	tci.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
	tci.usage = SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_WRITE | SDL_GPU_TEXTUREUSAGE_SAMPLER;
	tci.width = render.base_width;
	tci.height = render.base_height;
	tci.layer_count_or_depth = 1;
	tci.num_levels = 1;
	render.raycast_texture = SDL_CreateGPUTexture(render.dev, &tci);
	if (!render.raycast_texture) {
		SDL_Log("SDL_CreateGPUTexture, error: %s", SDL_GetError());
		return 0;
	}
	return 1;
}

int init_image_pipeline(void) {

	SDL_GPUShader *vertex_shader = load_shader(render.dev, "image.vert.spv", 1, 0);
	SDL_GPUShader *fragment_shader = load_shader(render.dev, "image.frag.spv", 0, 1);
	if (!vertex_shader || !fragment_shader) {
		SDL_Log("load_shader, error: %s", SDL_GetError());
		return 0;
	}


	SDL_GPUColorTargetDescription color_target = {
		.format = SDL_GetGPUSwapchainTextureFormat(render.dev, render.win),
		.blend_state = {
			.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
			.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
			.color_blend_op = SDL_GPU_BLENDOP_ADD,

			.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
			.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
			.alpha_blend_op = SDL_GPU_BLENDOP_ADD,

			.enable_blend = true
		}
	};

	SDL_GPUGraphicsPipelineCreateInfo pipeline_info = {
		.vertex_shader = vertex_shader,
		.fragment_shader = fragment_shader,

		.vertex_input_state = {0},

		.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,

		.rasterizer_state = {
			.fill_mode = SDL_GPU_FILLMODE_FILL,
			.cull_mode = SDL_GPU_CULLMODE_NONE,
			.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE
		},

		.multisample_state = {
			.sample_count = SDL_GPU_SAMPLECOUNT_1
		},

		.target_info = {
			.color_target_descriptions = &color_target,
			.num_color_targets = 1
		}
	};

	render.img_pipeline = SDL_CreateGPUGraphicsPipeline(render.dev, &pipeline_info);
	if (!render.img_pipeline) {
		SDL_Log("Create pipeline : %s", SDL_GetError());
		return 0;
	}

	SDL_ReleaseGPUShader(render.dev, vertex_shader);
	SDL_ReleaseGPUShader(render.dev, fragment_shader);
	return 1;
}

int init_image_sampler(void) {
	 SDL_GPUSamplerCreateInfo sampler_info = {
		.min_filter = SDL_GPU_FILTER_NEAREST,
		.mag_filter = SDL_GPU_FILTER_NEAREST,
		.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
		.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
		.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
		.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE
	};

	render.img_sampler = SDL_CreateGPUSampler(render.dev, &sampler_info);
	if (!render.img_sampler) {
		SDL_Log("Create sampler : %s", SDL_GetError());
		return 0;
	}
	return 1;
}

int init_render() {

	render.base_width = (1920/4);
	render.base_height = (1080/4);

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
	render.dev = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
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
	if (!res) {
		SDL_Log("init_raycast_pipeline, error: %s", SDL_GetError());
		return 1;
	}

	res = init_gpu_map_buffer();
	if (!res) {
		SDL_Log("init_gpu_map_buffer, error: %s", SDL_GetError());
		return 1;
	}

	res = init_image_sampler();
	if (!res) {
		SDL_Log("init_image_sampler, error: %s", SDL_GetError());
		return 1;
	}

	res = init_image_pipeline();
	if (!res) {
		SDL_Log("init_image_pipeline, error: %s", SDL_GetError());
		return 1;
	}

	render.atlas = init_gpu_texture_buffer("dirt.png");
	if (!render.atlas) {
		SDL_Log("init_gpu_texture_buffer, error: %s", SDL_GetError());
		return 1;
	}

	render.font = init_gpu_texture_buffer("font.png");
	if (!render.font) {
		SDL_Log("init_gpu_texture_buffer, error: %s", SDL_GetError());
		return 1;
	}

	upload_gpu_all_map();
	init_models_buffers();

	return 0;
}

