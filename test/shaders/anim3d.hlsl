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
    uint bone_mat_start;
    uint bone_tex_index;
    // anim_coord.x is the coordinate for the first frame
    // anim_coord.y is the coordinate for the second frame
    // anim_coord.z is the interpolation between the two
    float3 anim_coord;
    uint dummy2;
};

cbuffer object : register(b1)
{
    uint obj_id;
}

StructuredBuffer<object_t> obj_prop : register(t0);

Texture2D<float4> texture_set[] : register(t3);

StructuredBuffer<light_t> light : register(t1);

RWStructuredBuffer<bone_t> bone : register(u2);

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
            accum_light_dir += i.bone_weight[l] * mul(bone[obj_prop[obj_id].bone_mat_start + i.bone_id[l]].ibone_mat, light_dir);
            accum_world_pos += i.bone_weight[l] * mul(bone[obj_prop[obj_id].bone_mat_start + i.bone_id[l]].bone_mat, world_pos);
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

float4x4 quat_to_mat(float4 s)
{
    float4x4 mat;
    mat[0][0] = s[3] * s[3] + s[0] * s[0] - s[1] * s[1] - s[2] * s[2];
    mat[0][1] = 2 * s[0] * s[1] - 2 * s[3] * s[2];
    mat[0][2] = 2 * s[0] * s[2] + 2 * s[3] * s[1];
    mat[1][0] = 2 * s[0] * s[1] + 2 * s[3] * s[2];
    mat[1][1] = s[3] * s[3] - s[0] * s[0] + s[1] * s[1] - s[2] * s[2];;
    mat[1][2] = 2 * s[1] * s[2] - 2 * s[3] * s[0];
    mat[2][0] = 2 * s[0] * s[2] - 2 * s[3] * s[1];
    mat[2][1] = 2 * s[1] * s[2] + 2 * s[3] * s[0];
    mat[2][2] = s[3] * s[3] - s[0] * s[0] - s[1] * s[1] + s[2] * s[2];
    mat[3][0] = 0.0;
    mat[3][1] = 0.0;
    mat[3][2] = 0.0;

    mat[0][3] = 0.0;
    mat[1][3] = 0.0;
    mat[2][3] = 0.0;
    mat[3][3] = 1.0;
    return mat;
}

float4x4 pos_to_mat(float3 s)
{
    float4x4 mat;
    mat[0][0] = 1.0;
    mat[0][1] = 0.0;
    mat[0][2] = 0.0;
    mat[0][3] = s[0];
    mat[1][0] = 0.0;
    mat[1][1] = 1.0;
    mat[1][2] = 0.0;
    mat[1][3] = s[1];
    mat[2][0] = 0.0;
    mat[2][1] = 0.0;
    mat[2][2] = 1.0;
    mat[2][3] = s[2];
    mat[3][0] = 0.0;
    mat[3][1] = 0.0;
    mat[3][2] = 0.0;
    mat[3][3] = 1.0;
    return mat;
}

float4x4 scale_to_mat(float3 s)
{
    float4x4 mat;
    mat[0][0] = s[0];
    mat[0][1] = 0.0;
    mat[0][2] = 0.0;
    mat[0][3] = 0.0;
    mat[1][0] = 0.0;
    mat[1][1] = s[1];
    mat[1][2] = 0.0;
    mat[1][3] = 0.0;
    mat[2][0] = 0.0;
    mat[2][1] = 0.0;
    mat[2][2] = s[2];
    mat[2][3] = 0.0;
    mat[3][0] = 0.0;
    mat[3][1] = 0.0;
    mat[3][2] = 0.0;
    mat[3][3] = 1.0;
    return mat;
}

float4x4 inverse(float4x4 m)
{
    float n11 = m[0][0], n12 = m[1][0], n13 = m[2][0], n14 = m[3][0];
    float n21 = m[0][1], n22 = m[1][1], n23 = m[2][1], n24 = m[3][1];
    float n31 = m[0][2], n32 = m[1][2], n33 = m[2][2], n34 = m[3][2];
    float n41 = m[0][3], n42 = m[1][3], n43 = m[2][3], n44 = m[3][3];

    float t11 = n23 * n34 * n42 - n24 * n33 * n42 + n24 * n32 * n43 - n22 * n34 * n43 - n23 * n32 * n44 + n22 * n33 * n44;
    float t12 = n14 * n33 * n42 - n13 * n34 * n42 - n14 * n32 * n43 + n12 * n34 * n43 + n13 * n32 * n44 - n12 * n33 * n44;
    float t13 = n13 * n24 * n42 - n14 * n23 * n42 + n14 * n22 * n43 - n12 * n24 * n43 - n13 * n22 * n44 + n12 * n23 * n44;
    float t14 = n14 * n23 * n32 - n13 * n24 * n32 - n14 * n22 * n33 + n12 * n24 * n33 + n13 * n22 * n34 - n12 * n23 * n34;

    float det = n11 * t11 + n21 * t12 + n31 * t13 + n41 * t14;
    float idet = 1.0f / det;

    float4x4 ret;

    ret[0][0] = t11 * idet;
    ret[0][1] = (n24 * n33 * n41 - n23 * n34 * n41 - n24 * n31 * n43 + n21 * n34 * n43 + n23 * n31 * n44 - n21 * n33 * n44) * idet;
    ret[0][2] = (n22 * n34 * n41 - n24 * n32 * n41 + n24 * n31 * n42 - n21 * n34 * n42 - n22 * n31 * n44 + n21 * n32 * n44) * idet;
    ret[0][3] = (n23 * n32 * n41 - n22 * n33 * n41 - n23 * n31 * n42 + n21 * n33 * n42 + n22 * n31 * n43 - n21 * n32 * n43) * idet;

    ret[1][0] = t12 * idet;
    ret[1][1] = (n13 * n34 * n41 - n14 * n33 * n41 + n14 * n31 * n43 - n11 * n34 * n43 - n13 * n31 * n44 + n11 * n33 * n44) * idet;
    ret[1][2] = (n14 * n32 * n41 - n12 * n34 * n41 - n14 * n31 * n42 + n11 * n34 * n42 + n12 * n31 * n44 - n11 * n32 * n44) * idet;
    ret[1][3] = (n12 * n33 * n41 - n13 * n32 * n41 + n13 * n31 * n42 - n11 * n33 * n42 - n12 * n31 * n43 + n11 * n32 * n43) * idet;

    ret[2][0] = t13 * idet;
    ret[2][1] = (n14 * n23 * n41 - n13 * n24 * n41 - n14 * n21 * n43 + n11 * n24 * n43 + n13 * n21 * n44 - n11 * n23 * n44) * idet;
    ret[2][2] = (n12 * n24 * n41 - n14 * n22 * n41 + n14 * n21 * n42 - n11 * n24 * n42 - n12 * n21 * n44 + n11 * n22 * n44) * idet;
    ret[2][3] = (n13 * n22 * n41 - n12 * n23 * n41 - n13 * n21 * n42 + n11 * n23 * n42 + n12 * n21 * n43 - n11 * n22 * n43) * idet;

    ret[3][0] = t14 * idet;
    ret[3][1] = (n13 * n24 * n31 - n14 * n23 * n31 + n14 * n21 * n33 - n11 * n24 * n33 - n13 * n21 * n34 + n11 * n23 * n34) * idet;
    ret[3][2] = (n14 * n22 * n31 - n12 * n24 * n31 - n14 * n21 * n32 + n11 * n24 * n32 + n12 * n21 * n34 - n11 * n22 * n34) * idet;
    ret[3][3] = (n12 * n23 * n31 - n13 * n22 * n31 + n13 * n21 * n32 - n11 * n23 * n32 - n12 * n21 * n33 + n11 * n22 * n33) * idet;

    return ret;
}

groupshared float4x4 mat[128];
groupshared uint parent[128];

[numthreads(128,1,1)]
void cs_main(uint3 dispatch_id : SV_DispatchThreadID)
{
    uint bone_id = dispatch_id.x;
    uint obj_id = dispatch_id.y;
    uint bone_tex_index = obj_prop[obj_id].bone_tex_index;

    // Sample the key frame
    float2 bone_coord;
    bone_coord.x = (((float) 3*bone_id) + .5);
    bone_coord.y = obj_prop[obj_id].anim_coord.x;
    
    float4 rot_quat = texture_set[bone_tex_index].SampleLevel(sampleLinear, bone_coord, 0.0);
    bone_coord.x += 1.0;
    float4 position = texture_set[bone_tex_index].SampleLevel(sampleLinear, bone_coord, 0.0);
    bone_coord.x += 1.0;
    float3 scaling = texture_set[bone_tex_index].SampleLevel(sampleLinear, bone_coord, 0.0).xyz;
    parent[bone_id] = (uint) (position.w + 0.5);
    
    float frame_lerp = obj_prop[obj_id].anim_coord.z;
    if (frame_lerp > 0.0) {
        bone_coord.y = obj_prop[obj_id].anim_coord.y;
        float3 scaling1 = texture_set[bone_tex_index].SampleLevel(sampleLinear, bone_coord, 0.0).xyz;
        bone_coord.x -= 1.0;
        float3 position1 = texture_set[bone_tex_index].SampleLevel(sampleLinear, bone_coord, 0.0);
        bone_coord.x -= 1.0;
        float4 rot_quat1 = texture_set[bone_tex_index].SampleLevel(sampleLinear, bone_coord, 0.0);
        scaling = lerp(scaling, scaling1, frame_lerp);
        position.xyz = lerp(position.xyz, position1, frame_lerp);
        rot_quat = lerp(rot_quat, rot_quat1, frame_lerp);
    }

    //uint3 bone_coord;
    //bone_coord.x = 3 * bone_id;
    //bone_coord.y = obj_prop[obj_id].anim_coord.x - 0.5;
    //bone_coord.z = 0;
    //float lerp_amount = frac(obj_prop[obj_id].anim_coord.x - 0.5);
    //
    //float4 rot_quat0 = texture_set[bone_tex_index].Load(bone_coord);
    //bone_coord.x += 1;
    //float4 position0 = texture_set[bone_tex_index].Load(bone_coord);
    //bone_coord.x += 1;
    //float3 scaling0 = texture_set[bone_tex_index].Load(bone_coord).xyz;
    //parent[bone_id] = (uint) (position0.w + 0.5);
    //bone_coord.y += 1;
    //float3 scaling1 = texture_set[bone_tex_index].Load(bone_coord).xyz;
    //bone_coord.x -= 1;
    //float3 position1 = texture_set[bone_tex_index].Load(bone_coord);
    //bone_coord.x -= 1;
    //float4 rot_quat1 = texture_set[bone_tex_index].Load(bone_coord);
    //
    //float3 scaling = lerp(scaling0, scaling1, lerp_amount);
    //float3 position = lerp(position0.xyz, position1, lerp_amount);
    //float4 rot_quat = lerp(rot_quat0, rot_quat1, lerp_amount);
    //
    //float frame_lerp = obj_prop[obj_id].anim_coord.z;
    //if (frame_lerp > 0.0) {
    //    bone_coord.y = obj_prop[obj_id].anim_coord.y - 0.5;
    //    lerp_amount = frac(obj_prop[obj_id].anim_coord.y - 0.5);
    //
    //    rot_quat0 = texture_set[bone_tex_index].Load(bone_coord);
    //    bone_coord.x += 1;
    //    position0 = texture_set[bone_tex_index].Load(bone_coord);
    //    bone_coord.x += 1;
    //    scaling0 = texture_set[bone_tex_index].Load(bone_coord).xyz;
    //    parent[bone_id] = (uint) (position0.w + 0.5);
    //    bone_coord.y += 1;
    //    scaling1 = texture_set[bone_tex_index].Load(bone_coord).xyz;
    //    bone_coord.x -= 1;
    //    position1 = texture_set[bone_tex_index].Load(bone_coord);
    //    bone_coord.x -= 1;
    //    rot_quat1 = texture_set[bone_tex_index].Load(bone_coord);
    //    
    //    scaling1 = lerp(scaling0, scaling1, lerp_amount);
    //    position1 = lerp(position0.xyz, position1, lerp_amount);
    //    rot_quat1 = lerp(rot_quat0, rot_quat1, lerp_amount);
    //    
    //    scaling = lerp(scaling, scaling1, frame_lerp);
    //    position = lerp(position.xyz, position1, frame_lerp);
    //    rot_quat = lerp(rot_quat, rot_quat1, frame_lerp);
    //}

    rot_quat = normalize(rot_quat);
    
    // Load the inverse bind pose matrix
    uint3 pose_coord;
    pose_coord.x = 3 * bone_id;
    pose_coord.y = 0;
    pose_coord.z = 0;

    float4x4 inv_bind_pose;
    inv_bind_pose[0] = texture_set[bone_tex_index].Load(pose_coord);
    pose_coord.x += 1;
    inv_bind_pose[1] = texture_set[bone_tex_index].Load(pose_coord);
    pose_coord.x += 1;
    inv_bind_pose[2] = texture_set[bone_tex_index].Load(pose_coord);
    inv_bind_pose[3] = float4(0.0, 0.0, 0.0, 1.0);
    
    //scaling = float3(1, 1, 1);
    //rot_quat = float4(0, 0, 0, 1);
    //position.xyz = float3(0, 0, 0);

    mat[bone_id] = scale_to_mat(scaling);
    mat[bone_id] = mul(quat_to_mat(rot_quat), mat[bone_id]);
    mat[bone_id] = mul(pos_to_mat(position.xyz), mat[bone_id]);
    GroupMemoryBarrierWithGroupSync();

    uint i;
    // loop through log2(max_bones)
    for (i = 0; i < 7; i++) {
        uint parent_id = parent[bone_id];
        if (parent_id < 128) {
            parent[bone_id] = parent[parent_id];
            mat[bone_id] = mul(mat[parent_id], mat[bone_id]);
        }
        GroupMemoryBarrierWithGroupSync();
    }
    
    mat[bone_id] = mul(mat[bone_id], inv_bind_pose);

    bone[obj_prop[obj_id].bone_mat_start + bone_id].bone_mat = mat[bone_id];
    bone[obj_prop[obj_id].bone_mat_start + bone_id].ibone_mat = inverse(mat[bone_id]);

    //bone[obj_prop[obj_id].bone_mat_start + bone_id].bone_mat =
    //float4x4(1, 0, 0, 0,
    //         0, 1, 0, 0,
    //         0, 0, 1, 0,
    //         0, 0, 0, 1);
    //bone[obj_prop[obj_id].bone_mat_start + bone_id].ibone_mat =
    //    float4x4(1, 0, 0, 0,
    //             0, 1, 0, 0,
    //             0, 0, 1, 0,
    //             0, 0, 0, 1);
}
