// Simple pixel shader
// outputs color for vertex shader

Texture2D<float4> tex : register(t0);

SamplerState sampleLinear : register(s0);

struct VS2PS {
    float4 pos : SV_POSITION;
    float4 color : COLOR;
    float2 tcoord : TEXCOORD0;
};

float4 main(VS2PS input) : SV_Target
{
    return tex.Sample(sampleLinear, input.tcoord);
}
