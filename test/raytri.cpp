// k3 graphics test
// simple test to render a raytraced triangle

#include "k3.h"

struct vertex_t {
    float position[3];
};

class App
{
private:
    static const uint32_t NUM_VERSIONS = 4;
    k3win win;
    k3gfx gfx;
    k3cmdBuf cmd_buf;
    k3surf rt_out;
    k3tlas tlas_main_scene;
    k3rtState rt_state;
    k3rtStateTable rt_state_table;

public:
    App();
    ~App();

    void Setup();
    void Keyboard(k3key k, char c, k3keyState state);
    void Display();
    static void K3CALLBACK KeyboardCallback(void* data, k3key k, char c, k3keyState state);
    static void K3CALLBACK DisplayCallback(void* data);
};

App::App()
{
    Setup();
}

App::~App()
{ }

void App::Setup()
{
    win = k3winObj::CreateWindowed("raytri", 100, 100, 640, 480, 128, 32);
    win->SetKeyboardFunc(KeyboardCallback);
    win->SetDisplayFunc(DisplayCallback);
    win->SetIdleFunc(DisplayCallback);
    win->SetVisible(true);
    win->SetCursorVisible(true);
    win->SetDataPtr(this);

    gfx = win->GetGfx();
    printf("Adapter: %s\n", gfx->AdapterName());
    printf("Raytracing Tier supported: %d\n", gfx->GetRayTracingSupport());

    cmd_buf = gfx->CreateCmdBuf();
    uint32_t view_index = 0;

    k3resourceDesc rdesc;
    win->GetBackBuffer()->GetResource()->getDesc(&rdesc);
    k3viewDesc vdesc = {};
    vdesc.view_index = view_index++;
    rt_out = gfx->CreateSurface(&rdesc, NULL, NULL, &vdesc);

    k3uploadBuffer buf = gfx->CreateUploadBuffer();
    vertex_t* verts = (vertex_t*)buf->MapForWrite(3 * sizeof(vertex_t));
    verts[0].position[0] = 0.25f;
    verts[0].position[1] = 0.25f;
    verts[0].position[2] = 1.0f;
    verts[1].position[0] = 0.75f;
    verts[1].position[1] = 0.25f;
    verts[1].position[2] = 1.0f;
    verts[2].position[0] = 0.25f;
    verts[2].position[1] = 0.75f;
    verts[2].position[2] = 1.0f;

    //verts[0].position[0] = 0.0f;
    //verts[0].position[1] = 1.0f;
    //verts[0].position[2] = 0.0f;
    //verts[1].position[0] = 0.86f;
    //verts[1].position[1] =-0.50f;
    //verts[1].position[2] = 0.0f;
    //verts[2].position[0] = 0.86f;
    //verts[2].position[1] = 0.50f;
    //verts[2].position[2] = 0.0f;
    buf->Unmap();
    k3bufferDesc bdesc = { 0 };
    bdesc.size = 3 * sizeof(vertex_t);
    bdesc.stride = sizeof(vertex_t);
    bdesc.format = k3fmt::RGB32_FLOAT;
    k3buffer vertex_buffer = gfx->CreateBuffer(&bdesc);
    k3resource buffer_resource = vertex_buffer->GetResource();

    k3blasCreateDesc bl_desc;
    bl_desc.xform_3x4 = NULL;
    bl_desc.ib = NULL;
    bl_desc.vb = vertex_buffer;
    bl_desc.alloc = true;
    k3blas blas_tri = gfx->CreateBlas(&bl_desc);

    k3tlasInstance tl_inst;
    tl_inst.transform[0] = 1.0f;
    tl_inst.transform[1] = 0.0f;
    tl_inst.transform[2] = 0.0f;
    tl_inst.transform[3] = 0.0f;
    tl_inst.transform[4] = 0.0f;
    tl_inst.transform[5] = 1.0f;
    tl_inst.transform[6] = 0.0f;
    tl_inst.transform[7] = 0.0f;
    tl_inst.transform[8] = 0.0f;
    tl_inst.transform[9] = 0.0f;
    tl_inst.transform[10] = 1.0f;
    tl_inst.transform[11] = 0.0f;
    tl_inst.id = 0;
    tl_inst.hit_group = 0;
    tl_inst.mask = 0xff;
    tl_inst.flags = 0;
    tl_inst.blas = blas_tri;

    k3tlasCreateDesc tl_desc;
    tl_desc.num_instances = 1;
    tl_desc.instances = &tl_inst;
    tl_desc.view_index = view_index++;
    tl_desc.alloc = true;
    tlas_main_scene = gfx->CreateTlas(&tl_desc);

    static const char* ray_gen_entry = "rayGen";
    static const char* miss_entry = "miss";
    static const char* closest_hit_entry = "closestHit";
    static const char* hit_group = "HitGroup";

    k3shader ray_shaders = gfx->CreateShaderFromCompiledFile("simple_ray.cso");
    const char* shader_entries[3] = { ray_gen_entry, miss_entry, closest_hit_entry };
    const char* hit_miss_shader_entries[2] = { miss_entry, closest_hit_entry };

    k3bindingViewSet rt_view_sets[2];
    rt_view_sets[0].num_views = 1;
    rt_view_sets[0].reg = 0;
    rt_view_sets[0].space = 0;
    rt_view_sets[0].type = k3shaderBindType::UAV;
    rt_view_sets[0].offset = 0;
    rt_view_sets[1].num_views = 1;
    rt_view_sets[1].reg = 0;
    rt_view_sets[1].space = 0;
    rt_view_sets[1].type = k3shaderBindType::SRV;
    rt_view_sets[1].offset = 1;

    //k3bindingParam rt_bindings[1];
    //rt_bindings[0].type = k3bindingType::VIEW_SET_TABLE;
    //rt_bindings[0].view_set_table_desc.num_view_sets = 2;
    //rt_bindings[0].view_set_table_desc.view_sets = rt_view_sets;

    k3bindingParam rt_bindings[2];
    rt_bindings[0].type = k3bindingType::VIEW_SET;
    rt_bindings[0].view_set_desc.type = k3shaderBindType::UAV;
    rt_bindings[0].view_set_desc.num_views = 1;
    rt_bindings[0].view_set_desc.reg = 0;
    rt_bindings[0].view_set_desc.space = 0;
    rt_bindings[0].view_set_desc.offset = 0;
    rt_bindings[1].type = k3bindingType::VIEW_SET;
    rt_bindings[1].view_set_desc.type = k3shaderBindType::SRV;
    rt_bindings[1].view_set_desc.num_views = 1;
    rt_bindings[1].view_set_desc.reg = 0;
    rt_bindings[1].view_set_desc.space = 0;
    rt_bindings[1].view_set_desc.offset = 1;

    k3shaderBinding ray_gen_bindings = gfx->CreateTypedShaderBinding(2, rt_bindings, 0, NULL, k3shaderBindingType::LOCAL);
    k3shaderBinding hit_miss_bindings = gfx->CreateTypedShaderBinding(0, NULL, 0, NULL, k3shaderBindingType::LOCAL);
    k3shaderBinding global_bindings = gfx->CreateShaderBinding(0, NULL, 0, NULL);

    k3rtStateDesc rt_state_desc[10] = {};
    // Shader library
    rt_state_desc[0].type = k3rtStateType::SHADER;
    rt_state_desc[0].shader.obj = ray_shaders;
    rt_state_desc[0].shader.num_entries = 3;
    rt_state_desc[0].shader.entries = shader_entries;
    // hit group
    rt_state_desc[1].type = k3rtStateType::HIT_GROUP;
    rt_state_desc[1].elem.hit_group.type = k3rtHitGroupType::TRIANGLES;
    rt_state_desc[1].elem.hit_group.name = hit_group;
    rt_state_desc[1].elem.hit_group.any_hit_shader = NULL;
    rt_state_desc[1].elem.hit_group.closest_hit_shader = closest_hit_entry;
    rt_state_desc[1].elem.hit_group.intersection_shader = NULL;
    // ray gen shader binding
    rt_state_desc[2].type = k3rtStateType::SHADER_BINDING;
    rt_state_desc[2].shader_binding = ray_gen_bindings;
    // ray gen association
    rt_state_desc[3].type = k3rtStateType::EXPORT_ASSOCIATION;
    rt_state_desc[3].elem.export_association.num_exports = 1;
    rt_state_desc[3].elem.export_association.export_names = &ray_gen_entry;
    rt_state_desc[3].elem.export_association.association_index = 2;
    // hit-miss shader binding
    rt_state_desc[4].type = k3rtStateType::SHADER_BINDING;
    rt_state_desc[4].shader_binding = hit_miss_bindings;
    // hit-miss association
    rt_state_desc[5].type = k3rtStateType::EXPORT_ASSOCIATION;
    rt_state_desc[5].elem.export_association.num_exports = 2;
    rt_state_desc[5].elem.export_association.export_names = hit_miss_shader_entries;
    rt_state_desc[5].elem.export_association.association_index = 4;
    // shader config
    rt_state_desc[6].type = k3rtStateType::SHADER_CONFIG;
    rt_state_desc[6].elem.shader_config.attrib_size = 2 * sizeof(float);
    rt_state_desc[6].elem.shader_config.payload_size = 3 * sizeof(float);
    // shader config association
    rt_state_desc[7].type = k3rtStateType::EXPORT_ASSOCIATION;
    rt_state_desc[7].elem.export_association.num_exports = 3;
    rt_state_desc[7].elem.export_association.export_names = shader_entries;
    rt_state_desc[7].elem.export_association.association_index = 6;
    // pipe config
    rt_state_desc[8].type = k3rtStateType::PIPELINE_CONFIG;
    rt_state_desc[8].elem.pipeline_config.max_recursion = 1;
    // global bindings
    rt_state_desc[9].type = k3rtStateType::SHADER_BINDING;
    rt_state_desc[9].shader_binding = global_bindings;

    rt_state = gfx->CreateRTState(10, rt_state_desc);

    k3rtStateTableDesc rt_state_table_desc = {};
    rt_state_table_desc.num_entries = 3;
    rt_state_table_desc.num_args = 2;
    rt_state_table_desc.mem_pool = NULL;
    rt_state_table_desc.mem_offset = 0;
    rt_state_table = gfx->CreateRTStateTable(&rt_state_table_desc);
    
    k3rtStateTableArg rt_state_table_arg[2];
    rt_state_table_arg[0].type = k3rtStateTableArgType::HANDLE;
    rt_state_table_arg[0].bind_type = k3shaderBindType::UAV;
    rt_state_table_arg[0].obj = rt_out;
    rt_state_table_arg[1].type = k3rtStateTableArgType::HANDLE;
    rt_state_table_arg[1].bind_type = k3shaderBindType::SRV;
    rt_state_table_arg[1].obj = tlas_main_scene;

    k3rtStateTableEntryDesc rt_state_table_entries[3];
    rt_state_table_entries[0].shader = ray_gen_entry;
    rt_state_table_entries[0].num_args = 2;
    rt_state_table_entries[0].args = rt_state_table_arg;
    rt_state_table_entries[1].shader = miss_entry;
    rt_state_table_entries[1].num_args = 0;
    rt_state_table_entries[1].args = NULL;
    rt_state_table_entries[2].shader = hit_group;
    rt_state_table_entries[2].num_args = 0;
    rt_state_table_entries[2].args = NULL;
    
    k3rtStateTableUpdate rt_state_table_update = {};
    rt_state_table_update.copy_buffer = gfx->CreateUploadBuffer();
    rt_state_table_update.state = rt_state;
    rt_state_table_update.start = 0;
    rt_state_table_update.num_entries = 3;
    rt_state_table_update.entries = rt_state_table_entries;

    cmd_buf->Reset();
    cmd_buf->TransitionResource(buffer_resource, k3resourceState::COPY_DEST);
    cmd_buf->UploadBuffer(buf, buffer_resource);
    cmd_buf->TransitionResource(buffer_resource, k3resourceState::COMMON);
    cmd_buf->BuildBlas(blas_tri);
    cmd_buf->BuildTlas(tlas_main_scene);
    cmd_buf->UpdateRTStateTable(rt_state_table, &rt_state_table_update);
    cmd_buf->Close();
    gfx->SubmitCmdBuf(cmd_buf);

    gfx->WaitGpuIdle();
}

void App::Keyboard(k3key k, char c, k3keyState state)
{
    if (state == k3keyState::PRESSED) {
        switch (k) {
        case k3key::ESCAPE:
            k3winObj::ExitLoop();
            break;
        }
    }
}

void App::Display()
{
    k3surf back_buffer = win->GetBackBuffer();
    k3resource back_buffer_resource = back_buffer->GetResource();
    float clear_color[] = { 0.2f, 0.2f, 0.2f, 1.0f };

    k3renderTargets rt = { NULL };
    rt.render_targets[0] = back_buffer;
    cmd_buf->Reset();
    cmd_buf->TransitionResource(back_buffer_resource, k3resourceState::RENDER_TARGET);
    cmd_buf->ClearRenderTarget(back_buffer, clear_color, NULL);

    cmd_buf->SetRTState(rt_state);
    k3rtDispatch rt_desc;
    rt_desc.width = win->GetWidth();
    rt_desc.height = win->GetHeight();
    rt_desc.depth = 1;
    rt_desc.state_table = rt_state_table;
    rt_desc.raygen_index = 0;
    rt_desc.raygen_entries = 1;
    rt_desc.miss_index = 1;
    rt_desc.miss_entries = 1;
    rt_desc.hit_group_index = 2;
    rt_desc.hit_group_entries = 1;
    cmd_buf->TransitionResource(rt_out->GetResource(), k3resourceState::UAV);
    cmd_buf->RTDispatch(&rt_desc);

    cmd_buf->TransitionResource(rt_out->GetResource(), k3resourceState::COPY_SOURCE);
    cmd_buf->TransitionResource(back_buffer_resource, k3resourceState::COPY_DEST);
    cmd_buf->Copy(back_buffer_resource, rt_out->GetResource());

    cmd_buf->TransitionResource(back_buffer_resource, k3resourceState::COMMON);
    cmd_buf->Close();
    gfx->SubmitCmdBuf(cmd_buf);
    win->SwapBuffer();
}

void K3CALLBACK App::KeyboardCallback(void* data, k3key k, char c, k3keyState state)
{
    App* a = (App*)data;
    a->Keyboard(k, c, state);
}

void K3CALLBACK App::DisplayCallback(void* data)
{
    App* a = (App*)data;
    a->Display();
}

int main()
{
    App a;
    k3winObj::WindowLoop();

    return 0;
}
