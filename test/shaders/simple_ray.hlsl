// Simple ray racing set of shaders

struct attrib_t {
    float4 normal;  // normal in (x, y, z); u in w
    float4 tangent; // v in  x; tangent in (y, z, w)
};

StructuredBuffer<attrib_t> vert_attribs : register(t1);

RaytracingAccelerationStructure main_scene : register(t0);
RWTexture2D<float4> render_target : register(u0);

cbuffer camera : register(b0)
{
    row_major float4x4 ivp;
}

struct Payload
{
    float3 color;
};

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

StructuredBuffer<object_t> obj_prop : register(t2);

SamplerState sampleLinear : register(s0);
Texture2D<float4> texture_set[] : register(t4);

StructuredBuffer<light_t> light : register(t3);

[shader("raygeneration")]
void rayGen()
{
    uint3 launch_index = DispatchRaysIndex();
    uint3 launch_dim = DispatchRaysDimensions();
    
    float2 coord = float2(launch_index.xy) + float2(0.5, 0.5);
    float2 dim = float2(launch_dim.xy);
    
    float2 d = ((coord / dim) * 2.0 - 1.0);
    float aspect_ratio = dim.x / dim.y;
    float4x4 cmat = float4x4(1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1);
    float4 origin = mul(ivp,  float4(0.0, 0.0, 1000000.0, 1.0));
    float4 direction = mul(ivp, float4(d.x, -d.y, 0.0, 1.0));
    //float4 origin = mul(ivp, float4(0.0, 0.0, -2.0, 1.0));
    //float4 direction = mul(ivp, float4(d.x * aspect_ratio, -d.y, 1.0, 1.0));
    RayDesc ray;
    //ray.Origin = float3(0.0, 0.0, -2.0);
    //ray.Direction = normalize(float3(d.x * aspect_ratio, -d.y, 1.0));
    ray.Origin = origin.xyz / origin.w;
    ray.Direction = normalize(direction.xyz);
    ray.TMin = 0.2;
    ray.TMax = 1000.0;
    
    Payload payload;
    TraceRay(main_scene, 0, 0xFF, 0, 0, 0, ray, payload);
    render_target[launch_index.xy] = float4(payload.color, 1.0);
}

[shader("miss")]
void miss(inout Payload payload)
{
    payload.color = float3(0.0, 0.25, 0.75);
}

[shader("closesthit")]
void closestHit(inout Payload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    float3 baryc = float3(1.0 - attribs.barycentrics.x - attribs.barycentrics.y,
        attribs.barycentrics.xy);
    uint vert_index = 3 * (PrimitiveIndex() + obj_prop[InstanceID()].prim_start);

    //float3 light_dir = normalize(float3(0.25, -0.5, 1.0));
    float3 light_dir = normalize(light[0].position);
    light_dir = mul(obj_prop[InstanceID()].iworld, float4(light_dir, 0.0f)).xyz;

    float3 normal = vert_attribs[vert_index + 0].normal.xyz * baryc.x +
            vert_attribs[vert_index + 1].normal.xyz * baryc.y +
            vert_attribs[vert_index + 2].normal.xyz * baryc.z;
    float3 tangent = vert_attribs[vert_index + 0].tangent.yzw * baryc.x +
        vert_attribs[vert_index + 1].tangent.yzw * baryc.y +
        vert_attribs[vert_index + 2].tangent.yzw * baryc.z;
    normal = normalize(normal);
    tangent = normalize(tangent);
    float3 bitangent = cross(tangent, normal);
    float3x3 tan_mat = float3x3(tangent, bitangent, normal);

    float2 uv0 = float2(vert_attribs[vert_index + 0].normal.w, vert_attribs[vert_index + 0].tangent.x);
    float2 uv1 = float2(vert_attribs[vert_index + 1].normal.w, vert_attribs[vert_index + 1].tangent.x);
    float2 uv2 = float2(vert_attribs[vert_index + 2].normal.w, vert_attribs[vert_index + 2].tangent.x);
    float2 uv = uv0 * baryc.x + uv1 * baryc.y + uv2 * baryc.z;

    light_dir = mul(tan_mat, light_dir);
    uint diffuse_texture_index = obj_prop[InstanceID()].diffuse_map_index;
    uint normal_texture_index = obj_prop[InstanceID()].normal_map_index;
    if (diffuse_texture_index == 0xffffffff) {
        payload.color = obj_prop[InstanceID()].diffuse_color;
    } else {
        payload.color = texture_set[diffuse_texture_index].SampleLevel(sampleLinear, uv, 0).xyz;
    }
    if (normal_texture_index == 0xffffffff) {
        normal = float3(0.0, 0.0, 1.0);
    } else {
        normal = normalize(2.0 * texture_set[normal_texture_index].SampleLevel(sampleLinear, uv, 0).xyz - 1.0);
    }

    payload.color  = payload.color * dot(normal, light_dir);
}
