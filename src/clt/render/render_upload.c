#include "client.h"



int upload_gpu_all_map()
{
	const size_t total_chunk = render.render_distance * render.render_distance * render.render_distance;
	const size_t bytes_per_chunk = (size_t)CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE * sizeof(uint32_t);

	SDL_GPUTransferBufferCreateInfo tbci = {0};
	tbci.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
	tbci.size = (uint32_t)(total_chunk * bytes_per_chunk);

	SDL_GPUTransferBuffer *map_transfer_buffer = SDL_CreateGPUTransferBuffer(render.dev, &tbci);

	if (!map_transfer_buffer) {
		SDL_Log("SDL_CreateGPUTransferBuffer, error: %s", SDL_GetError());
		return 0;
	}
	uint8_t *ptr = SDL_MapGPUTransferBuffer(render.dev, map_transfer_buffer, true);
	if (!ptr) {
		SDL_Log("upload_gpu_map: map: %s", SDL_GetError());
		SDL_ReleaseGPUTransferBuffer(render.dev, map_transfer_buffer);
		return 0;
	}
	for (size_t i = 0; i < total_chunk; i++) {
		uint8_t *dst = ptr + i * bytes_per_chunk;
		if (clt.world.chunks[i].blocks)
			SDL_memcpy(dst, *clt.world.chunks[i].blocks, bytes_per_chunk);
		else
			SDL_memset(dst, 0, bytes_per_chunk);
	}
	SDL_UnmapGPUTransferBuffer(render.dev, map_transfer_buffer);

	SDL_GPUCommandBuffer *cmd  = SDL_AcquireGPUCommandBuffer(render.dev);
	SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(cmd);

	SDL_GPUTransferBufferLocation src = {
		.transfer_buffer = map_transfer_buffer,
		.offset = 0,
	};

	SDL_GPUBufferRegion dst = {
		.buffer = render.blocks_buffer,
		.offset = 0,
		.size = (Uint32)bytes_per_chunk * total_chunk,
	};

	SDL_UploadToGPUBuffer(copy, &src, &dst, false);

	SDL_EndGPUCopyPass(copy);
	SDL_SubmitGPUCommandBuffer(cmd);

	SDL_WaitForGPUIdle(render.dev);
	SDL_ReleaseGPUTransferBuffer(render.dev, map_transfer_buffer);
	return 1;
}

int update_gpu_map() {
	if (clt.world.dirty) {
		const size_t total_chunk = render.render_distance * render.render_distance * render.render_distance;
		const size_t bytes_per_chunk = (size_t)CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE * sizeof(uint32_t);
	
		SDL_GPUCommandBuffer *cmd  = SDL_AcquireGPUCommandBuffer(render.dev);
		SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(cmd);
		for (size_t i = 0; i < total_chunk; i++) {
			if (clt.world.chunks[i].dirty){
				uint8_t *ptr = SDL_MapGPUTransferBuffer(render.dev, render.blocks_transfer_buffer, true);

				if (clt.world.chunks[i].blocks)
					SDL_memcpy(ptr, *clt.world.chunks[i].blocks, bytes_per_chunk);
				else
					SDL_memset(ptr, 0, bytes_per_chunk);
				
				SDL_GPUTransferBufferLocation src;
				src.transfer_buffer = render.blocks_transfer_buffer;
				src.offset = 0;

				SDL_GPUBufferRegion dst = {
					.buffer = render.blocks_buffer,
					.offset = bytes_per_chunk * i,
					.size = (Uint32)bytes_per_chunk,
				};

				SDL_UploadToGPUBuffer(copy, &src, &dst, false);
				clt.world.chunks[i].dirty = 0;
			}
		}
		clt.world.dirty = 0;
		SDL_EndGPUCopyPass(copy);
		SDL_SubmitGPUCommandBuffer(cmd);
	
		SDL_WaitForGPUIdle(render.dev);
	}
	return 0;
}