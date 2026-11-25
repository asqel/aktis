// raycast.hlsl
// Raycaster voxel. Un thread = un pixel.

// DXIL :
//   dxc -T cs_6_0 -E main -Wno-ignored-attributes raycast.hlsl -Fo raycast.comp.dxil
// SPIR-V (Vulkan) :
//   dxc -spirv -fspv-target-env=vulkan1.0 -T cs_6_0 -E main src/clt/shaders/src/raycast.hlsl -Fo src/clt/shaders/build/raycast.comp.spv

// Taille du monde chargé sur le GPU, en chunks. À garder identique au C.
#ifndef WORLD_W
#define WORLD_W 2 // chunks en X et en Z
#endif
#ifndef WORLD_H
#define WORLD_H 2 // chunks en Y
#endif

#define TILE_SIZE 16


static const int3 WORLD_SIZE = int3(WORLD_W * 16, WORLD_H * 16, WORLD_W * 16);

struct Cam
{
    float4 pos;
    float4 fwd;
    float4 right;
    float4 up;
    float fov;
};

// Convention SDL GPU pour le compute :
//   space0 : buffers/textures en lecture seule   (t#)
//   space1 : buffers/textures en lecture-écriture (u#)
//   space2 : uniform buffers                      (b#)
// Les attributs [[vk::...]] ne servent qu'à la cible SPIR-V, ils sont ignorés pour DXIL.

[[vk::binding(0, 0)]]
Texture2D<float4> atlas : register(t0, space0);

[[vk::binding(1, 0)]]
StructuredBuffer<uint> blocks : register(t1, space0);

[[vk::binding(0, 1), vk::image_format("rgba8")]]
RWTexture2D<float4> outImage : register(u0, space1);

[[vk::binding(0, 2)]]
ConstantBuffer<Cam> cam : register(b0, space2);

static const float3 PALETTE[5] = {
    float3(0.0, 0.0, 0.0),
    float3(0.35, 0.75, 0.35),
    float3(0.25, 0.55, 0.25),
    float3(0.75, 0.70, 0.60),
    float3(0.90, 0.35, 0.30)
};



// Lit un bloc à partir de ses coordonnées monde (déjà vérifiées dans les bornes).
// Chunks à la suite : le chunk i commence à i * 4096.
uint fetchBlock(int3 c)
{
    int3 ch = c >> 4;
    int3 l = c & 15;

    int chunkIdx = ch.y * WORLD_W * WORLD_W + ch.z * WORLD_W + ch.x;
    int local = (l.y << 8) | (l.z << 4) | l.x;
    return blocks[(chunkIdx << 12) + local];
}

struct ray_t {
    int x;
    int y;
    int z;

    float dir_x;
    float dir_y;
    float dir_z;
    float x_percent;
    float y_percent;
    float dist;
    int has_hit;
    int side;

};

#define EPSILON (0.000001f)
#define INF (1e30f)

static void cast_ray(inout ray_t ray, int map_x, int map_y, int map_z) {
    int step_x = (ray.dir_x > 0) ? (1) : (-1);
    int step_y = (ray.dir_y > 0) ? (1) : (-1);
    int step_z = (ray.dir_z > 0) ? (1) : (-1);

    float dir_x_fabs = abs(ray.dir_x);
    float dir_y_fabs = abs(ray.dir_y);
    float dir_z_fabs = abs(ray.dir_z);

    float delta_x = (dir_x_fabs < EPSILON) ? (INF) : (1 / dir_x_fabs);
    float delta_y = (dir_y_fabs < EPSILON) ? (INF) : (1 / dir_y_fabs);
    float delta_z = (dir_z_fabs < EPSILON) ? (INF) : (1 / dir_z_fabs);

    float max_x = (ray.dir_x < 0) ? (cam.pos.x - map_x) : (map_x + 1 - cam.pos.x);
    max_x *= delta_x;

    float max_y = (ray.dir_y < 0) ? (cam.pos.y - map_y) : (map_y + 1 - cam.pos.y);
    max_y *= delta_y;

    float max_z = (ray.dir_z < 0) ? (cam.pos.z - map_z) : (map_z + 1 - cam.pos.z);
    max_z *= delta_z;

    int side = 0;
    float dist = 0;

    uint render_distance = 125;
    while (true) {
        uint block = fetchBlock(int3(map_x, map_y, map_z));
        if (block != 0) {
            if (side == 0) {
                ray.x_percent = cam.pos.z + ray.dir_z * dist - map_z;
                ray.y_percent = cam.pos.y + ray.dir_y * dist - map_y;
                if (ray.dir_x < 0) {
                    ray.x_percent = 1 - ray.x_percent;
                }
                // hit  west east
            }
            else if (side == 1) {
                ray.x_percent = cam.pos.x + ray.dir_x * dist - map_x;
                ray.y_percent = cam.pos.z + ray.dir_z * dist - map_z;
                if (ray.dir_y < 0) {
                    ray.x_percent = 1 - ray.x_percent;
                    ray.y_percent = 1 - ray.y_percent;
                }
                // hit up bot
            }
            else {
                ray.x_percent = cam.pos.x + ray.dir_x * dist - map_x;
                ray.y_percent = cam.pos.y + ray.dir_y * dist - map_y;
                if (ray.dir_z > 0) {
                    ray.x_percent = 1 - ray.x_percent;
                }
                // hit north south
            }
            ray.has_hit = 1;
            ray.x = map_x;
            ray.y = map_y;
            ray.z = map_z;
            ray.dist = dist;
            ray.side = side;
            return;
        }

        if (max_x < max_y && max_x < max_z) {
            dist = max_x;
            max_x += delta_x;
            map_x += step_x;
            side = 0;
        }
        else if (max_y < max_z) {
            dist = max_y;
            max_y += delta_y;
            map_y += step_y;
            side = 1;
        }
        else {
            dist = max_z;
            max_z += delta_z;
            map_z += step_z;
            side = 2;
        }
        if (dist > render_distance)
            break;
        if ((ray.dir_x < 0 && map_x < 0) || (ray.dir_x > 0 && map_x >= WORLD_W * 16))
            break;
        if ((ray.dir_y < 0 && map_y < 0) || (ray.dir_y > 0 && map_y >= WORLD_H * 16))
            break;
        if ((ray.dir_z < 0 && map_z < 0) || (ray.dir_z > 0 && map_z >= WORLD_W * 16))
            break;
    }
}

#define RGB(R, G, B) (float4((float)(R) / 0xff, (float)(G) / 0xff, (float)(B) / 0xff, 1.0))

[numthreads(8, 8, 1)]
void main(uint3 tid : SV_DispatchThreadID)
{
    uint aw, ah;
    atlas.GetDimensions(aw, ah);
    int tilesX = aw / TILE_SIZE;

    int2 px = int2(tid.xy);

    // Résolution lue depuis la texture de sortie
    uint rw, rh;
    outImage.GetDimensions(rw, rh);
    int2 res = int2(rw, rh);

    if (px.x >= res.x || px.y >= res.y)
        return;

    int map_x = (int)floor(cam.pos.x);
    int map_y = (int)floor(cam.pos.y);
    int map_z = (int)floor(cam.pos.z);

    float aspect = (float)rw / (float)rh;
    float tanH = tan(cam.fov * 0.5);

    float sx = (2.0 * (px.x + 0.5) / rw - 1.0) * aspect * tanH;
    float sy = (1.0 - 2.0 * (px.y + 0.5) / rh) * tanH;

    ray_t ray;
    ray.dir_x = cam.fwd.x + cam.right.x * sx + cam.up.x * sy;
    ray.dir_y = (cam.fwd.y + cam.right.y * sx + cam.up.y * sy);
    ray.dir_z = cam.fwd.z + cam.right.z * sx + cam.up.z * sy;


    ray.has_hit = 0;

    cast_ray(ray, map_x, map_y, map_z);

    if (ray.has_hit == 1) {
        uint id = fetchBlock(int3(ray.x, ray.y, ray.z));
        int tile = (int)id - 1; // id 1 -> tuile 0, id 2 -> tuile 1, ...

        int2 tileOrigin = int2((tile % tilesX) * TILE_SIZE, (tile / tilesX) * TILE_SIZE);

        // Sur les faces verticales y_percent monte vers le haut, la texture descend
        float u = ray.x_percent;
        float v = (ray.side == 1) ? ray.y_percent : (1.0 - ray.y_percent);

        int2 t = clamp(int2(float2(u, v) * 16), 0, 16 - 1);

        float4 texel = atlas.Load(int3(tileOrigin + t, 0));
        outImage[uint2(px)] = float4(texel.rgb, 1.0);
    }
    else {
        outImage[uint2(px)] = float4(0.0, 0.0, 0.0, 0.0);
    }
}