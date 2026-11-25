#include "client.h"
#include <math.h>

typedef struct {
    float pos[4], fwd[4], right[4], up[4];
    float fov;
} CamUniforms;

typedef struct {
    float position[2];
    float size[2];
    float uvPosition[2];
    float uvSize[2];
    float screenSize[2];
    float _pad[2];       // 48 octets, multiple de 16
    float color[4];
} SpriteUniform;

int render_raycast(SDL_GPUCommandBuffer *cmd, SDL_GPUTexture *swap, Uint32 sw, Uint32 sh) {
    const float fov = 70.f * (float)M_PI / 180.f;
    const float tanHalf = tanf(fov * 0.5f);
    const float aspect  = (float)render.base_width / (float)render.base_height;
    // --- Vecteurs caméra ---

    float yaw_sin = sinf(render.rot_y);
    float yaw_cos = cosf(render.rot_y);
    float pitch_sin = sinf(render.rot_x);
    float pitch_cos = cosf(render.rot_x);



    float fx = pitch_cos * yaw_cos;
    float fy = pitch_sin;
    float fz = pitch_cos * yaw_sin;

    float rx = -yaw_sin;
    float ry = 0;
    float rz = yaw_cos;
    
    float ux = -pitch_sin * yaw_cos;                              // cross(right, fwd)
    float uy = pitch_cos;
    float uz = -pitch_sin * yaw_sin;
    CamUniforms cam = {
        .pos   = {render.x, render.y, render.z, 0 },
        .fwd   = {fx, fy, fz, 0},
        .right = {rx, ry, rz, 0},
        .up    = {ux, uy, uz, 0},
        .fov = fov
    };
    // 1) Compute : un thread par pixel
    SDL_PushGPUComputeUniformData(cmd, 0, &cam, sizeof(cam));
    SDL_GPUStorageTextureReadWriteBinding outBind;
    SDL_zero(outBind);
    outBind.texture = render.raycast_texture;
    outBind.cycle   = true;
    SDL_GPUComputePass *pass = SDL_BeginGPUComputePass(cmd, &outBind, 1, NULL, 0);
    SDL_BindGPUComputePipeline(pass, render.raycast_pipeline);
    SDL_BindGPUComputeStorageTextures(pass, 0, &render.atlas->tex, 1);
    SDL_BindGPUComputeStorageBuffers(pass, 0, &render.world_buffer, 1);
    SDL_DispatchGPUCompute(pass, (render.base_width + 7) / 8, (render.base_height + 7) / 8, 1);
    SDL_EndGPUComputePass(pass);

    // 2) Blit vers la fenêtre (avec mise à l'échelle)
    float scale = SDL_min((float)sw / render.base_width, (float)sh / render.base_height);
    SDL_GPUBlitInfo blit;
    SDL_zero(blit);
    blit.source.texture      = render.raycast_texture;
    blit.source.w            = render.base_width;
    blit.source.h            = render.base_height;
    blit.destination.texture = swap;
    blit.destination.w       = render.base_width * scale;
    blit.destination.h       = render.base_height * scale;
    blit.load_op             = SDL_GPU_LOADOP_DONT_CARE;
    blit.filter              = SDL_GPU_FILTER_NEAREST;
    blit.destination.x = sw / 2 - blit.destination.w / 2;
    blit.destination.y = sh / 2 - blit.destination.h / 2;
    SDL_BlitGPUTexture(cmd, &blit);
}

int render_texture(SDL_GPUCommandBuffer *cmd, SDL_GPUTexture *swap, Uint32 sw, Uint32 sh, GPUTexture_t *texture) {
    SDL_GPUColorTargetInfo target_info = {
        .texture = swap,
        .load_op = SDL_GPU_LOADOP_LOAD,
        .store_op = SDL_GPU_STOREOP_STORE,
    };
    SDL_GPURenderPass *render_pass = SDL_BeginGPURenderPass(cmd, &target_info, 1, NULL);
    if (!render_pass) {
        SDL_Log("error render_pass, %s\n", SDL_GetError());
    }
    char c = '1';
    const float cw = 10.0f, ch = 20.0f;           // taille d'un glyphe (pixels)
    // position du glyphe dans l'atlas, en pixels
    float src_x = (c & 0xF) * cw;
    float src_y = (c >> 4)  * ch;
    SpriteUniform push = {
        .position   = { 200.0f, 100.0f },
        .size       = { cw * 10, ch * 10 },
        .uvPosition = { src_x / texture->width, src_y / texture->height },
        .uvSize     = { cw / texture->width,    ch / texture->height },
        .screenSize = { sw, sh },
        ._pad       = { 0.0f, 0.0f },
        .color = {1.0f, 0.0f, 0.0f, 1.0f}
    };
    SDL_BindGPUGraphicsPipeline(render_pass, render.img_pipeline);
    SDL_PushGPUVertexUniformData(cmd, 0, &push, sizeof(push));
    SDL_GPUTextureSamplerBinding binding = {
        .texture = texture->tex,
        .sampler = render.img_sampler
    };
    SDL_BindGPUFragmentSamplers(render_pass, 0, &binding, 1);
    SDL_DrawGPUPrimitives(render_pass, 6, 1, 0, 0);
    SDL_EndGPURenderPass(render_pass);
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

    // Présentation.
    if (!SDL_SubmitGPUCommandBuffer(cmd)) {
        SDL_Log("Submit : %s", SDL_GetError());
    }
    return 0;
}