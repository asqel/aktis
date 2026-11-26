#include "client.h"

SDL_GPUShader *load_shader(SDL_GPUDevice *gpu, char *shader_file, Uint32 num_samplers, Uint32 num_uniform_buf) {
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
