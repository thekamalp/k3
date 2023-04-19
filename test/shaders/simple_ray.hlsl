// Simple ray racing set of shaders

struct attrib_t {
    float4 color;
};

StructuredBuffer<attrib_t> vert_attribs : register(t1);

RaytracingAccelerationStructure main_scene : register(t0);
RWTexture2D<float4> render_target : register(u0);

struct Payload
{
    float3 color;
};


[shader("raygeneration")]
void rayGen()
{
    uint3 launch_index = DispatchRaysIndex();
    uint3 launch_dim = DispatchRaysDimensions();
    
    float2 coord = float2(launch_index.xy);
    float2 dim = float2(launch_dim.xy);
    
    float2 d = ((coord / dim) * 2.0 - 1.0);
    float aspect_ratio = dim.x / dim.y;
    RayDesc ray;
    ray.Origin = float3(0.0, 0.0, -2.0);
    ray.Direction = normalize(float3(d.x * aspect_ratio, -d.y, 1.0));
    ray.TMin = 0.0;
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
    uint vert_index = 3 * PrimitiveIndex();

    payload.color = vert_attribs[vert_index + 0].color * baryc.x +
        vert_attribs[vert_index + 1].color * baryc.y +
        vert_attribs[vert_index + 2].color * baryc.z;
}
