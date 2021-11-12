// Simple vertex shader
// outputs 3 fixed locations/colors

struct vertex {
    float4 pos : POSITION;
    float4 color : COLOR;
    float2 tcoord : TEXCOORD0;
};

struct VS2PS {
    float4 pos : SV_POSITION;
    float4 color : COLOR;
    float2 tcoord : TEXCOORD0;
};

cbuffer obj : register(b0) {
    float4 offset;
};

VS2PS main(vertex inp)
{
    VS2PS output;
    output.pos = inp.pos + offset;
    output.color = inp.color;
    output.tcoord = inp.tcoord;
    return output;
}
