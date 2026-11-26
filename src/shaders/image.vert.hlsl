// space0: texture + read obly
// space1: uniform buffer
struct SpriteData {
	float2 position;
	float2 size;
	float2 uvPosition;
	float2 uvSize;
	float2 screenSize;
	float2 _pad;
	float4 color;
};

[[vk::binding(0, 1)]]
ConstantBuffer<SpriteData> sprite : register(b0, space1);

struct VSOutput {
	float4 pos : SV_Position;
	[[vk::location(0)]] float2 uv : TEXCOORD0;
	[[vk::location(1)]] float4 tint : COLOR0;
};

static const float2 positions[6] = {
	float2(0, 0),
	float2(1, 0),
	float2(1, 1),
	float2(0, 0),
	float2(1, 1),
	float2(0, 1)
};

VSOutput main(uint vertexIndex : SV_VertexID) {
	VSOutput o;

	float2 corner = positions[vertexIndex];
	float2 p = sprite.position + corner * sprite.size;

	float2 ndc = (p / sprite.screenSize) * 2.0 - 1.0;
	o.pos = float4(ndc.x, -ndc.y, 0.0, 1.0);

	o.uv = sprite.uvPosition + corner * sprite.uvSize;
	o.tint = sprite.color;

	return o;
}
