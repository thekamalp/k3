// Color vertex shader - outputs transformed position and color

struct vertex {
    float4 pos : POSITION;
    float4 color : COLOR;
};

struct VS2PS {
    float4 pos : SV_POSITION;
    float4 color : COLOR;
};

cbuffer obj : register(b0) {
    row_major float4x4 xform;
};

VS2PS vs_main(vertex inp)
{
    VS2PS output;
    output.pos = mul(xform, inp.pos);
    output.color = inp.color;
    return output;
}

float4 ps_main(VS2PS input) : SV_Target
{
    return input.color;
}
