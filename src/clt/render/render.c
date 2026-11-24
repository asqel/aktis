#include "client.h"
#include <math.h>

typedef struct {
    float pos[4], fwd[4], right[4], up[4];
    float fov;
} CamUniforms;


int render_draw(void) {
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

    SDL_GPUCommandBuffer *cmd = SDL_AcquireGPUCommandBuffer(render.dev);
    SDL_GPUTexture *swap = NULL;
    Uint32 sw = 0, sh = 0;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmd, render.win, &swap, &sw, &sh) || !swap) {
        SDL_SubmitGPUCommandBuffer(cmd);
        return 1;
    }
    // 1) Compute : un thread par pixel
    SDL_PushGPUComputeUniformData(cmd, 0, &cam, sizeof(cam));
    SDL_GPUStorageTextureReadWriteBinding outBind;
    SDL_zero(outBind);
    outBind.texture = render.raycast_texture;
    outBind.cycle   = true;
    SDL_GPUComputePass *pass = SDL_BeginGPUComputePass(cmd, &outBind, 1, NULL, 0);
    SDL_BindGPUComputePipeline(pass, render.raycast_pipeline);
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
    blit.destination.w       = sw;
    blit.destination.h       = sh;
    blit.load_op             = SDL_GPU_LOADOP_DONT_CARE;
    blit.filter              = SDL_GPU_FILTER_NEAREST;
    blit.destination.x = 0;
    blit.destination.y = 0;
    SDL_BlitGPUTexture(cmd, &blit);

    // Présentation.
    if (!SDL_SubmitGPUCommandBuffer(cmd)) {
        SDL_Log("Submit : %s", SDL_GetError());
    }
    return 0;
}