// sprite.frag.hlsl
// Fragment shader de sprite : texture * teinte.
//
// dxc -spirv -fspv-target-env=vulkan1.0 -T ps_6_0 -E main sprite.frag.hlsl -Fo sprite.frag.spv

// Convention SDL GPU pour le fragment shader :
//   space2 : textures et samplers
//   space3 : uniform buffers
// [[vk::combinedImageSampler]] fusionne texture + sampler en un seul descripteur
// (l'équivalent du sampler2D GLSL). Les attributs [[vk::...]] sont ignorés en DXIL.
[[vk::combinedImageSampler, vk::binding(0, 2)]]
Texture2D<float4> spriteTexture : register(t0, space2);

[[vk::combinedImageSampler, vk::binding(0, 2)]]
SamplerState spriteSampler : register(s0, space2);

struct PSInput
{
    float4 pos : SV_Position;
    [[vk::location(0)]] float2 uv   : TEXCOORD0;
    [[vk::location(1)]] float4 tint : COLOR0;
};

float4 main(PSInput i) : SV_Target0
{
    return spriteTexture.Sample(spriteSampler, i.uv) * i.tint;
}