// Simple 3d vertex and pixel shaders

struct VS_IN {
    float3 pos : POSITION;
    float4 norm : NORMAL;
    float4 tang : TANGENT;
    uint4 bone_id : BLENDINDICES;
    float4 bone_weight : BLENDWEIGHT;
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

struct skin_t {
    uint4 bone_id;
    float4 weights;
};

struct light_t {
    float3 position;
    float intensity;
    float3 color;
    float decay_start;
    uint light_type;
    uint decay_type;
    uint cast_shadows;
    float spot_angle;
};

struct bone_t {
    row_major float4x4 bone_mat;
    row_major float4x4 ibone_mat;
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

Texture2D<float4> texture_set[] : register(t3);

StructuredBuffer<light_t> light : register(t1);

StructuredBuffer<bone_t> bone : register(t2);

VS2PS vs_main(VS_IN i)
{
    float4 world_pos = float4(i.pos, 1.0);
    float3 light_dir = normalize(light[0].position);
    light_dir = mul(obj_prop[obj_id].iworld, float4(light_dir, 0.0f)).xyz;

    if (i.bone_weight[0] >= 0.1) {
        float4 accum_world_pos = float4(0.0, 0.0, 0.0, 0.0);
        float3 accum_light_dir = float3(0.0, 0.0, 0.0);
        uint l;
        for (l = 0; l < 4; l = l + 1) {
            accum_light_dir += i.bone_weight[l] * mul(bone[i.bone_id[l]].ibone_mat, light_dir);
            accum_world_pos += i.bone_weight[l] * mul(bone[i.bone_id[l]].bone_mat, world_pos);
        }
        light_dir = accum_light_dir;
        world_pos = accum_world_pos;
        //light_dir = mul(bone[0].ibone_mat, float4(light_dir, 0.0f)).xyz;
        //world_pos = mul(bone[0].bone_mat, world_pos);
    }

    world_pos = mul(obj_prop[obj_id].world, world_pos);

    VS2PS o;
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
