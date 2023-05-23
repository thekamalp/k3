// Simple 3d vertex and pixel shaders

struct VS_IN {
    float3 pos : POSITION;
    float4 norm : NORMAL;
    float4 tang : TANGENT;
};

struct VS2PS {
    float4 pos : SV_POSITION;
    float4 light : COLOR;
    float2 texcoord : TEXCOORD0;
};

struct attrib_t {
    float4 normal;  // normal in (x, y, z); u in w
    float4 tangent; // v in  x; tangent in (y, z, w)
};

cbuffer camera : register(b0)
{
    row_major float4x4 ivp;
    row_major float4x4 vp;
}

SamplerState sampleLinear : register(s0);

struct object_t {
    row_major float4x4 world;
    row_major float4x4 iworld;
    float3 diffuse_color;
    uint diffuse_map_index;
    uint normal_map_index;
    uint prim_start;
    uint dummy0;
    uint dummy1;
};

cbuffer object : register(b1)
{
    uint obj_id;
}

StructuredBuffer<object_t> obj_prop : register(t0);

Texture2D<float4> texture_set[] : register(t1);

VS2PS vs_main(VS_IN i)
{
    float3 light_dir = normalize(float3(0.25, -0.5, 1.0));
    light_dir = mul(obj_prop[obj_id].iworld, float4(light_dir, 0.0f)).xyz;

    VS2PS o;
    float4 world_pos = mul(obj_prop[obj_id].world, float4(i.pos, 1.0));
    o.pos = mul(vp, world_pos);
    float3 normal = i.norm.xyz;
    float3 tangent = i.tang.yzw;
    float3 bitangent = cross(tangent, normal);
    float3x3 tan_mat = float3x3(tangent, bitangent, normal);

    o.light.xyz = mul(tan_mat, light_dir);
    o.light.w = 1.0;
    o.texcoord = float2(i.norm.w, i.tang.x);
    return o;
}

float4 ps_main(VS2PS i) : SV_Target
{
    float2 uv = i.texcoord;
    float3 light_dir = normalize(i.light.xyz);
    float3 normal;
    float4 color;
    uint diffuse_texture_index = obj_prop[obj_id].diffuse_map_index;
    uint normal_texture_index = obj_prop[obj_id].normal_map_index;
    if (diffuse_texture_index != 0xffffffff) {
        color = texture_set[diffuse_texture_index].Sample(sampleLinear, uv);
    } else {
        color = float4(obj_prop[obj_id].diffuse_color, 1.0);
    }
    if (normal_texture_index != 0xffffffff) {
        normal = normalize(2.0 * texture_set[normal_texture_index].Sample(sampleLinear, uv).xyz - 1.0);
    } else {
        normal = float3(0.0, 0.0, 1.0);
    }

    color = color * dot(normal, light_dir);

    return color;
}
