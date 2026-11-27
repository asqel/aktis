#include "client.h"

SDL_GPUShader *load_shader(SDL_GPUDevice *gpu, char *shader_file, Uint32 num_uniform_buf, Uint32 num_samplers) {
	SDL_GPUShaderStage stage;
	if (SDL_strstr(shader_file, ".vert"))
		stage = SDL_GPU_SHADERSTAGE_VERTEX;
	else if (SDL_strstr(shader_file, ".frag"))
		stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
	//char *shader_path = my_strjoin(shader_file, ".spv");
	size_t code_size = 0;
	void *code = SDL_LoadFile(shader_file, &code_size);
	if (code == NULL) {
		SDL_Log("Impossible de lire %s : %s", shader_file, SDL_GetError());
		return NULL;
	}

	SDL_GPUShaderCreateInfo info = {0};
	info.code = (const Uint8 *)code;
	info.code_size = code_size;
	info.entrypoint = "main";
	info.format = SDL_GPU_SHADERFORMAT_SPIRV;
	info.stage = stage;
	info.num_samplers = num_samplers;
	info.num_uniform_buffers = num_uniform_buf;

	SDL_GPUShader *shader = SDL_CreateGPUShader(gpu, &info);

	if (shader == NULL) {
		SDL_Log("Impossible de créer le shader %s : %s", shader_file, SDL_GetError());
	}

	SDL_free(code);

	return shader;
}

// Stolen from sdl_image 3.4 because i dont have 3.4
SDL_GPUTexture *load_gpu_texture(SDL_GPUDevice *device, SDL_GPUCopyPass *copy_pass, const char *file, int *width, int *height) {
	SDL_Surface *surface = IMG_Load(file);
	if (!surface)
		return NULL;
	
    if (surface->format != SDL_PIXELFORMAT_RGBA32) {
        SDL_Surface *old_surface = surface;
        surface = SDL_ConvertSurface(old_surface, SDL_PIXELFORMAT_RGBA32);
        SDL_DestroySurface(old_surface);
        if (!surface) {
            return NULL;
        }
    }

    SDL_GPUTextureCreateInfo texture_create_info;
    SDL_zero(texture_create_info);
    texture_create_info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    texture_create_info.type = SDL_GPU_TEXTURETYPE_2D;
    texture_create_info.layer_count_or_depth = 1;
    texture_create_info.num_levels = 1;
    texture_create_info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_READ;
    texture_create_info.width = surface->w;
    texture_create_info.height = surface->h;
    SDL_GPUTexture *texture = SDL_CreateGPUTexture(device, &texture_create_info);
    if (!texture) {
        SDL_DestroySurface(surface);
        return NULL;
    }

    SDL_GPUTransferBufferCreateInfo transfer_buffer_create_info;
    SDL_zero(transfer_buffer_create_info);
    transfer_buffer_create_info.size = surface->w * surface->h * 4;
    transfer_buffer_create_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    SDL_GPUTransferBuffer *transfer_buffer = SDL_CreateGPUTransferBuffer(device, &transfer_buffer_create_info);
    if (!transfer_buffer) {
        SDL_DestroySurface(surface);
        SDL_ReleaseGPUTexture(device, texture);
        return NULL;
    }

    Uint8 *dst = SDL_MapGPUTransferBuffer(device, transfer_buffer, 0);
    if (!dst) {
        SDL_DestroySurface(surface);
        SDL_ReleaseGPUTexture(device, texture);
        SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
        return NULL;
    }
    const Uint8 *src = surface->pixels;
    const int row_bytes = surface->w * 4;
    if (row_bytes == surface->pitch) {
        SDL_memcpy(dst, src, row_bytes * surface->h);
    } else {
        for (int y = 0; y < surface->h; y++) {
            SDL_memcpy(dst + y * row_bytes, src + y * surface->pitch, row_bytes);
        }
    }
    SDL_UnmapGPUTransferBuffer(device, transfer_buffer);

    SDL_GPUTextureTransferInfo texture_transfer_info;
    SDL_GPUTextureRegion texture_region;
    SDL_zero(texture_transfer_info);
    SDL_zero(texture_region);
    texture_transfer_info.transfer_buffer = transfer_buffer;
    texture_region.texture = texture;
    texture_region.w = surface->w;
    texture_region.h = surface->h;
    texture_region.d = 1;
    SDL_UploadToGPUTexture(copy_pass, &texture_transfer_info, &texture_region, 0);

    if (width) {
        *width = surface->w;
    }
    if (height) {
        *height = surface->h;
    }

    SDL_DestroySurface(surface);
    SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);

	return texture;
}
