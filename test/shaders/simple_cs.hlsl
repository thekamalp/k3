// A simple compute shader that swaps R and B channels of an UAV

RWTexture2D<float4> output_texture : register(u0);

[numthreads(8,8,1)]
void main(uint3 dispatch_id : SV_DispatchThreadID)
{
    float4 pixel = output_texture[dispatch_id.xy];
    output_texture[dispatch_id.xy].r = pixel.b;
    output_texture[dispatch_id.xy].b = pixel.r;
}
