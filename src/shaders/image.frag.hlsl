// space2: texture + sampler
// space3: uniform buff
// vk:combinedImageSampler combine texture + sampler

[[vk::combinedImageSampler, vk::binding(0, 2)]]
Texture2D<float4> spriteTexture : register(t0, space2);

[[vk::combinedImageSampler, vk::binding(0, 2)]]
SamplerState spriteSampler : register(s0, space2);

struct PSInput {
	float4 pos : SV_Position;
	[[vk::location(0)]] float2 uv : TEXCOORD0;
	[[vk::location(1)]] float4 tint : COLOR0;
};

float4 main(PSInput i) : SV_Target0 {
	return spriteTexture.Sample(spriteSampler, i.uv) * i.tint;
}
