// k3 graphics test
// simple test to render a raytraced triangle

#include "k3.h"

struct obj_prop_t {
    float world[16];
    float iworld[16];
    float diffuse_color[3];
    uint32_t diffuse_map_index;
    uint32_t normal_map_index;
    uint32_t prim_start;
    uint32_t dummy0;
    uint32_t dummy1;
};

class App
{
private:
    static const uint32_t NUM_VERSIONS = 4;
    static constexpr char* ray_gen_entry = "rayGen";
    static constexpr char* miss_entry = "miss";
    static constexpr char* closest_hit_entry = "closestHit";
    static constexpr char* hit_group = "HitGroup";

    k3win win;
    k3gfx gfx;
    k3cmdBuf cmd_buf;
    bool use_raster;
    k3fence fence;
    uint64_t version;
    uint32_t dxr_tier;

    k3surf depth_surf;
    k3surf rt_out;
    k3gfxState raster_state;
    k3mesh cube_mesh;
    k3rtState rt_state;
    k3tlas tlas_main_scene[NUM_VERSIONS];
    k3rtStateTable rt_state_table[NUM_VERSIONS];
    k3uploadBuffer cb_camera_upload[NUM_VERSIONS];
    k3buffer cb_camera[NUM_VERSIONS];
    k3uploadBuffer cb_obj_upload[NUM_VERSIONS];
    k3buffer cb_obj[NUM_VERSIONS];
    float eye_distance;
    float eye_rotation_angle;
    float eye_attitude_angle;
    uint32_t last_mouse_x, last_mouse_y;
    uint32_t mouse_button;

public:
    App();
    ~App();

    void Setup();
    void UpdateResources(uint32_t v);
    void Keyboard(k3key k, char c, k3keyState state);
    void Display();
    void Resize(uint32_t width, uint32_t height);
    void MouseMove(uint32_t x, uint32_t y);
    void MouseButton(uint32_t x, uint32_t y, uint32_t b, k3keyState state);
    void MouseScroll(uint32_t x, uint32_t y, int32_t vscroll, int32_t hscroll);
    static void K3CALLBACK KeyboardCallback(void* data, k3key k, char c, k3keyState state);
    static void K3CALLBACK DisplayCallback(void* data);
    static void K3CALLBACK ResizeCallback(void* data, uint32_t width, uint32_t height);
    static void K3CALLBACK MouseMoveCallback(void* data, uint32_t x, uint32_t y);
    static void K3CALLBACK MouseButtonCallback(void* data, uint32_t x, uint32_t y, uint32_t b, k3keyState state);
    static void K3CALLBACK MouseScrollCallback(void* data, uint32_t x, uint32_t y, int32_t vscroll, int32_t hscroll);
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

    gfx = win->GetGfx();
    printf("Adapter: %s\n", gfx->AdapterName());
    dxr_tier = gfx->GetRayTracingSupport();
    printf("Raytracing Tier supported: %d\n", dxr_tier);

    cmd_buf = gfx->CreateCmdBuf();
    uint32_t view_index = 0;

    _chdir("..\\test\\assets");
    k3meshDesc mdesc = {};
    mdesc.view_index = view_index;
    mdesc.cmd_buf = cmd_buf;
    mdesc.up_buf = gfx->CreateUploadBuffer();
    mdesc.name = "cube2e.fbx";
    cube_mesh = gfx->CreateMesh(&mdesc);
    view_index = mdesc.view_index;
    _chdir("..\\..\\bin");

    uint32_t i;
    // if there are any cameras, set window resolution to the first one
    if (cube_mesh->getNumCameras()) {
        uint32_t win_width, win_height;
        cube_mesh->getCameraResolution(0, &win_width, &win_height);
        win->SetSize(win_width, win_height);
        // set all cameras to have infinite far plane
        for (i = 0; i < cube_mesh->getNumCameras(); i++) {
            cube_mesh->setCameraFarPlane(i, INFINITY);
        }
    }

    // Make window visible after setting size
    win->SetVisible(true);
    win->SetCursorVisible(true);

    k3resourceDesc rdesc;
    win->GetBackBuffer()->GetResource()->getDesc(&rdesc);
    k3viewDesc vdesc = {};
    vdesc.view_index = view_index++;
    rt_out = gfx->CreateSurface(&rdesc, NULL, NULL, &vdesc);

    rdesc.format = k3fmt::D32_FLOAT;
    depth_surf = gfx->CreateSurface(&rdesc, &vdesc, NULL, NULL);

    eye_distance = 3.0f;
    eye_rotation_angle = 45.0f;
    eye_attitude_angle = 45.0f;
    fence = gfx->CreateFence();

    k3bufferDesc bdesc = { 0 };
    for (i = 0; i < NUM_VERSIONS; i++) {
        cb_camera_upload[i] = gfx->CreateUploadBuffer();
        cb_camera_upload[i]->MapForWrite(32 * sizeof(float));  // preallocate space
        cb_camera_upload[i]->Unmap();
        bdesc.view_index = view_index++;
        bdesc.size = 32 * sizeof(float);
        cb_camera[i] = gfx->CreateBuffer(&bdesc);
    }
    uint32_t num_objects = cube_mesh->getNumObjects();
    for (i = 0; i < NUM_VERSIONS; i++) {
        cb_obj_upload[i] = gfx->CreateUploadBuffer();
        cb_obj_upload[i]->MapForWrite(num_objects * sizeof(obj_prop_t));  // preallocate space
        cb_obj_upload[i]->Unmap();
        bdesc.view_index = view_index++;
        bdesc.size = num_objects * sizeof(obj_prop_t);
        bdesc.shader_resource = true;
        bdesc.stride = sizeof(obj_prop_t);
        cb_obj[i] = gfx->CreateBuffer(&bdesc);
    }

    k3bindingParam bind_params[5];
    bind_params[0].type = k3bindingType::VIEW_SET;
    bind_params[0].view_set_desc.type = k3shaderBindType::CBV;
    bind_params[0].view_set_desc.num_views = 1;
    bind_params[0].view_set_desc.reg = 0;
    bind_params[0].view_set_desc.space = 0;
    bind_params[0].view_set_desc.offset = 0;
    bind_params[1].type = k3bindingType::VIEW_SET;
    bind_params[1].view_set_desc.type = k3shaderBindType::SRV;
    bind_params[1].view_set_desc.num_views = cube_mesh->getNumTextures();
    if (bind_params[1].view_set_desc.num_views == 0) bind_params[1].view_set_desc.num_views = 1;
    bind_params[1].view_set_desc.reg = 2;
    bind_params[1].view_set_desc.space = 0;
    bind_params[1].view_set_desc.offset = 1;
    bind_params[2].type = k3bindingType::VIEW_SET;
    bind_params[2].view_set_desc.type = k3shaderBindType::SRV;
    bind_params[2].view_set_desc.num_views = 1;
    bind_params[2].view_set_desc.reg = 0;
    bind_params[2].view_set_desc.space = 0;
    bind_params[2].view_set_desc.offset = 2;
    bind_params[3].type = k3bindingType::CONSTANT;
    bind_params[3].constant.num_const = 1;
    bind_params[3].constant.reg = 1;
    bind_params[3].constant.space = 0;
    bind_params[4].type = k3bindingType::VIEW_SET;
    bind_params[4].view_set_desc.type = k3shaderBindType::SRV;
    bind_params[4].view_set_desc.num_views = 1;
    bind_params[4].view_set_desc.reg = 1;
    bind_params[4].view_set_desc.space = 0;
    bind_params[4].view_set_desc.offset = 4;

    k3samplerDesc sdesc = { 0 };
    sdesc.filter = k3texFilter::MIN_MAG_MIP_LINEAR;
    sdesc.addr_u = k3texAddr::WRAP;
    sdesc.addr_v = k3texAddr::WRAP;
    sdesc.addr_w = k3texAddr::CLAMP;
    k3shaderBinding raster_binding = gfx->CreateShaderBinding(5, bind_params, 1, &sdesc);

    k3shader vs, ps;
    vs = gfx->CreateShaderFromCompiledFile("simple3d_vs.cso");
    ps = gfx->CreateShaderFromCompiledFile("simple3d_ps.cso");

    k3inputElement elem[3];
    elem[0].name = "POSITION";
    elem[0].index = 0;
    elem[0].format = k3fmt::RGB32_FLOAT;
    elem[0].slot = 0;
    elem[0].offset = 0;
    elem[0].in_type = k3inputType::VERTEX;
    elem[0].instance_step = 0;
    elem[1].name = "NORMAL";
    elem[1].index = 0;
    elem[1].format = k3fmt::RGBA32_FLOAT;
    elem[1].slot = 1;
    elem[1].offset = 0;
    elem[1].in_type = k3inputType::VERTEX;
    elem[1].instance_step = 0;
    elem[2].name = "TANGENT";
    elem[2].index = 0;
    elem[2].format = k3fmt::RGBA32_FLOAT;
    elem[2].slot = 1;
    elem[2].offset = 16;
    elem[2].in_type = k3inputType::VERTEX;
    elem[2].instance_step = 0;

    k3gfxStateDesc state_desc = { 0 };
    state_desc.num_input_elements = 3;
    state_desc.input_elements = elem;
    state_desc.shader_binding = raster_binding;
    state_desc.vertex_shader = vs;
    state_desc.pixel_shader = ps;
    state_desc.sample_mask = ~0;
    state_desc.rast_state.fill_mode = k3fill::SOLID;
    state_desc.rast_state.cull_mode = k3cull::BACK;
    state_desc.rast_state.front_counter_clockwise = true;
    state_desc.depth_state.depth_enable = true;
    state_desc.depth_state.depth_write_enable = true;
    state_desc.depth_state.depth_test = k3testFunc::GREATER;
    state_desc.blend_state.blend_op[0].rt_write_mask = 0xf;
    state_desc.prim_type = k3primType::TRIANGLE;
    state_desc.num_render_targets = 1;
    state_desc.rtv_format[0] = k3fmt::RGBA8_UNORM;
    state_desc.dsv_format = k3fmt::D32_FLOAT;
    state_desc.msaa_samples = 1;
    raster_state = gfx->CreateGfxState(&state_desc);

    uint32_t num_meshes = cube_mesh->getNumMeshes();
    k3blasCreateDesc bl_desc;
    k3tlasInstance* tl_inst = NULL;
    k3blas* blas = NULL;
    k3rtStateTableArg rt_state_table_arg[7];
    k3rtStateTableUpdate rt_state_table_update = {};
    k3uploadBuffer table_buffers[NUM_VERSIONS];

    if (dxr_tier) {
        blas = new k3blas[num_meshes];
        bl_desc.xform_3x4 = NULL;
        bl_desc.ib = cube_mesh->getIndexBuffer();
        bl_desc.vb = cube_mesh->getVertexBuffer();
        bl_desc.alloc = true;
        for (i = 0; i < num_meshes; i++) {
            bl_desc.start_prim = cube_mesh->getMeshStartPrim(i);
            bl_desc.num_prims = cube_mesh->getMeshNumPrims(i);
            blas[i] = gfx->CreateBlas(&bl_desc);
        }

        tl_inst = new k3tlasInstance[num_objects];

        for (i = 0; i < num_objects; i++) {
            memcpy(tl_inst[i].transform, cube_mesh->getTransform(i), 12 * sizeof(float));
            tl_inst[i].id = i;
            tl_inst[i].hit_group = 0;
            tl_inst[i].mask = 0xff;
            tl_inst[i].flags = 0;
            tl_inst[i].blas = blas[cube_mesh->getMeshIndex(i)];
        }

        k3tlasCreateDesc tl_desc;
        tl_desc.num_instances = num_objects;
        tl_desc.instances = tl_inst;
        tl_desc.view_index = view_index++;
        tl_desc.alloc = true;
        for (i = 0; i < NUM_VERSIONS; i++) {
            tlas_main_scene[i] = gfx->CreateTlas(&tl_desc);
        }

        k3shader ray_shaders = gfx->CreateShaderFromCompiledFile("simple_ray.cso");
        const char* shader_entries[3] = { ray_gen_entry, miss_entry, closest_hit_entry };
        const char* hit_miss_shader_entries[2] = { miss_entry, closest_hit_entry };

        k3bindingParam rt_bindings[7];
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
        rt_bindings[2].type = k3bindingType::VIEW_SET;
        rt_bindings[2].view_set_desc.type = k3shaderBindType::CBV;
        rt_bindings[2].view_set_desc.num_views = 1;
        rt_bindings[2].view_set_desc.reg = 0;
        rt_bindings[2].view_set_desc.space = 0;
        rt_bindings[2].view_set_desc.offset = 2;
        rt_bindings[3].type = k3bindingType::VIEW_SET;
        rt_bindings[3].view_set_desc.type = k3shaderBindType::SRV;
        rt_bindings[3].view_set_desc.num_views = 1;
        rt_bindings[3].view_set_desc.reg = 1;
        rt_bindings[3].view_set_desc.space = 0;
        rt_bindings[3].view_set_desc.offset = 0;
        rt_bindings[4].type = k3bindingType::VIEW_SET;
        rt_bindings[4].view_set_desc.type = k3shaderBindType::SRV;
        rt_bindings[4].view_set_desc.num_views = cube_mesh->getNumTextures();
        if (rt_bindings[4].view_set_desc.num_views == 0) rt_bindings[4].view_set_desc.num_views = 1;
        rt_bindings[4].view_set_desc.reg = 4;
        rt_bindings[4].view_set_desc.space = 0;
        rt_bindings[4].view_set_desc.offset = 1;
        rt_bindings[5].type = k3bindingType::VIEW_SET;
        rt_bindings[5].view_set_desc.type = k3shaderBindType::SRV;
        rt_bindings[5].view_set_desc.num_views = 1;
        rt_bindings[5].view_set_desc.reg = 2;
        rt_bindings[5].view_set_desc.space = 0;
        rt_bindings[5].view_set_desc.offset = 2;
        rt_bindings[6].type = k3bindingType::VIEW_SET;
        rt_bindings[6].view_set_desc.type = k3shaderBindType::SRV;
        rt_bindings[6].view_set_desc.num_views = 1;
        rt_bindings[6].view_set_desc.reg = 3;
        rt_bindings[6].view_set_desc.space = 0;
        rt_bindings[6].view_set_desc.offset = 3;

        k3shaderBinding ray_gen_bindings = gfx->CreateTypedShaderBinding(3, rt_bindings, 0, NULL, k3shaderBindingType::LOCAL);
        k3shaderBinding hit_miss_bindings = gfx->CreateTypedShaderBinding(4, &(rt_bindings[3]), 0, NULL, k3shaderBindingType::LOCAL);
        k3shaderBinding global_bindings = gfx->CreateShaderBinding(0, NULL, 1, &sdesc);

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
        rt_state_desc[3].elem.export_association.export_names = shader_entries;
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
        rt_state_table_desc.num_args = 4;
        rt_state_table_desc.mem_pool = NULL;
        rt_state_table_desc.mem_offset = 0;
        for (i = 0; i < NUM_VERSIONS; i++) {
            rt_state_table[i] = gfx->CreateRTStateTable(&rt_state_table_desc);
        }

        rt_state_table_arg[0].type = k3rtStateTableArgType::HANDLE;
        rt_state_table_arg[0].bind_type = k3shaderBindType::UAV;
        rt_state_table_arg[0].obj = rt_out;
        rt_state_table_arg[1].type = k3rtStateTableArgType::HANDLE;
        rt_state_table_arg[1].bind_type = k3shaderBindType::SRV;
        //rt_state_table_arg[1].obj = tlas_main_scene;
        rt_state_table_arg[2].type = k3rtStateTableArgType::HANDLE;
        rt_state_table_arg[2].bind_type = k3shaderBindType::CBV;
        //rt_state_table_arg[2].obj = cb_camera;
        rt_state_table_arg[3].type = k3rtStateTableArgType::HANDLE;
        rt_state_table_arg[3].bind_type = k3shaderBindType::SRV;
        rt_state_table_arg[3].obj = cube_mesh->getAttribBuffer();
        rt_state_table_arg[4].type = k3rtStateTableArgType::HANDLE;
        rt_state_table_arg[4].bind_type = k3shaderBindType::SRV;
        rt_state_table_arg[4].obj = cube_mesh->getTexture(0);
        rt_state_table_arg[5].type = k3rtStateTableArgType::HANDLE;
        rt_state_table_arg[5].bind_type = k3shaderBindType::SRV;
        //rt_state_table_arg[5].obj = cb_obj;
        rt_state_table_arg[6].type = k3rtStateTableArgType::HANDLE;
        rt_state_table_arg[6].bind_type = k3shaderBindType::SRV;
        rt_state_table_arg[6].obj = cube_mesh->getLightBuffer();

        k3rtStateTableEntryDesc rt_state_table_entries[3];
        rt_state_table_entries[0].shader = ray_gen_entry;
        rt_state_table_entries[0].num_args = 3;
        rt_state_table_entries[0].args = rt_state_table_arg;
        rt_state_table_entries[1].shader = miss_entry;
        rt_state_table_entries[1].num_args = 4;
        rt_state_table_entries[1].args = &(rt_state_table_arg[3]);
        rt_state_table_entries[2].shader = hit_group;
        rt_state_table_entries[2].num_args = 4;
        rt_state_table_entries[2].args = &(rt_state_table_arg[3]);

        //rt_state_table_update.copy_buffer = gfx->CreateUploadBuffer();
        rt_state_table_update.state = rt_state;
        rt_state_table_update.start = 0;
        rt_state_table_update.num_entries = 3;
        rt_state_table_update.entries = rt_state_table_entries;

        for (i = 0; i < NUM_VERSIONS; i++) {
            table_buffers[i] = gfx->CreateUploadBuffer();
        }
    }

    k3resource buffer_resource;
    cmd_buf->Reset();
    cmd_buf->TransitionResource(depth_surf->GetResource(), k3resourceState::RENDER_TARGET);
    if (dxr_tier) {
        for (i = 0; i < num_meshes; i++) {
            cmd_buf->BuildBlas(blas[i]);
        }
        for (i = 0; i < NUM_VERSIONS; i++) {
            cmd_buf->BuildTlas(tlas_main_scene[i]);
            rt_state_table_update.copy_buffer = table_buffers[i];
            rt_state_table_arg[1].obj = tlas_main_scene[i];
            rt_state_table_arg[2].obj = cb_camera[i];
            rt_state_table_arg[5].obj = cb_obj[i];
            cmd_buf->UpdateRTStateTable(rt_state_table[i], &rt_state_table_update);
        }
    }
    cmd_buf->TransitionResource(cube_mesh->getLightBuffer()->GetResource(), k3resourceState::SHADER_BUFFER);
    //cmd_buf->UpdateRTStateTable(rt_state_table, &rt_state_table_update);
    cmd_buf->Close();
    gfx->SubmitCmdBuf(cmd_buf);
    for (i = 0; i < NUM_VERSIONS; i++) {
        version = fence->SetGpuFence(k3gpuQueue::GRAPHICS);
    }

    use_raster = (dxr_tier) ? false : true;
    last_mouse_x = 0;
    last_mouse_y = 0;
    mouse_button = 0x0;

    gfx->WaitGpuIdle();

    if (tl_inst) delete[] tl_inst;
    if (blas) delete[] blas;

    win->SetDataPtr(this);
    win->SetKeyboardFunc(KeyboardCallback);
    win->SetDisplayFunc(DisplayCallback);
    win->SetIdleFunc(DisplayCallback);
    win->SetResizeFunc(ResizeCallback);
    win->SetMouseFunc(MouseMoveCallback, MouseButtonCallback, MouseScrollCallback);
}

void App::UpdateResources(uint32_t v)
{
    float* camera_data = (float*)cb_camera_upload[v]->MapForWrite(32 * sizeof(float));
    float mat_data[16];

    if (cube_mesh->getNumCameras()) {
        cube_mesh->getCameraView(camera_data, 0);
        cube_mesh->getCameraProjection(mat_data, 0);
    } else {
        float y_axis[3] = { 0.0f, 1.0f, 0.0f };
        float z_axis[3] = { 0.0f, 0.0f, 1.0f };
        k3m3_SetRotation(camera_data, deg2rad(eye_attitude_angle), y_axis);
        k3m3_SetRotation(mat_data, deg2rad(eye_rotation_angle), z_axis);
        k3m3_Mul(camera_data, mat_data, camera_data);
        mat_data[0] = eye_distance; mat_data[1] = 0.0f; mat_data[2] = 0.0f;
        k3mv3_Mul(mat_data, camera_data, mat_data);

        //mat_data[0] = 2.0f; mat_data[1] = 2.0f; mat_data[2] = 2.0f; // eye position
        mat_data[4] = 0.0f; mat_data[5] = 0.0f; mat_data[6] = 0.0f;  // look at position
        mat_data[8] = z_axis[0]; mat_data[9] = z_axis[1]; mat_data[10] = z_axis[2]; // up direction
        float aspect = win->GetWidth() / (float)win->GetHeight();
        k3m4_SetLookAtRH(camera_data, &(mat_data[0]), &(mat_data[4]), &(mat_data[8]));
        k3m4_SetDXPerspectiveFovRevZRH(mat_data, deg2rad(60), aspect, 0.5f, INFINITY);
    }
    k3m4_Mul(camera_data, mat_data, camera_data);
    memcpy(camera_data + 16, camera_data, 16 * sizeof(float));
    k3m4_Inverse(camera_data);
    cb_camera_upload[v]->Unmap();

    uint32_t i;
    uint32_t num_objects = cube_mesh->getNumObjects();
    obj_prop_t* obj_data = (obj_prop_t*)cb_obj_upload[v]->MapForWrite(num_objects * sizeof(obj_prop_t));
    for (i = 0; i < num_objects; i++) {
        memcpy(obj_data[i].world, cube_mesh->getTransform(i), 16 * sizeof(float));
        memcpy(obj_data[i].iworld, cube_mesh->getTransform(i), 16 * sizeof(float));
        k3m4_Inverse(obj_data[i].iworld);
        memcpy(obj_data[i].diffuse_color, cube_mesh->getDiffuseColor(i), 3 * sizeof(float));
        obj_data[i].diffuse_map_index = cube_mesh->getDiffuseMapIndex(i);
        obj_data[i].normal_map_index = cube_mesh->getNormalMapIndex(i);
        obj_data[i].prim_start = cube_mesh->getStartPrim(i);
        if(dxr_tier) tlas_main_scene[v]->UpdateTransform(i, obj_data[i].world);
    }
    cb_obj_upload[v]->Unmap();

    k3resource buffer_resource;
    buffer_resource = cb_camera[v]->GetResource();
    cmd_buf->TransitionResource(buffer_resource, k3resourceState::COPY_DEST);
    cmd_buf->UploadBuffer(cb_camera_upload[v], buffer_resource);
    cmd_buf->TransitionResource(buffer_resource, k3resourceState::SHADER_BUFFER);
    buffer_resource = cb_obj[v]->GetResource();
    cmd_buf->TransitionResource(buffer_resource, k3resourceState::COPY_DEST);
    cmd_buf->UploadBuffer(cb_obj_upload[v], buffer_resource);
    cmd_buf->TransitionResource(buffer_resource, k3resourceState::SHADER_RESOURCE);

    if(dxr_tier) cmd_buf->BuildTlas(tlas_main_scene[v]);
}

void App::Keyboard(k3key k, char c, k3keyState state)
{
    if (state == k3keyState::PRESSED) {
        float rot_axis[3] = { 0.0f, 0.0f, 1.0f };
        float new_xform[16];
        float* model_xform = cube_mesh->getTransform(cube_mesh->getNumMeshes() - 1);
        switch (k) {
        case k3key::ESCAPE:
            k3winObj::ExitLoop();
            break;
        case k3key::LEFT:
            k3m4_SetRotation(new_xform, deg2rad(15.0f), rot_axis);
            k3m4_Mul(model_xform, new_xform, model_xform);
            break;
        case k3key::RIGHT:
            k3m4_SetRotation(new_xform, deg2rad(-15.0f), rot_axis);
            k3m4_Mul(model_xform, new_xform, model_xform);
            break;
        case k3key::UP:
            k3m4_SetIdentity(new_xform);
            new_xform[11] = 1.0f;
            k3m4_Mul(model_xform, model_xform, new_xform);
            break;
        case k3key::DOWN:
            k3m4_SetIdentity(new_xform);
            new_xform[11] = -1.0f;
            k3m4_Mul(model_xform, model_xform, new_xform);
            break;
        case k3key::SPACE:
            use_raster = !use_raster || (dxr_tier == 0);
            printf("Using %s\n", (use_raster) ? "raster" : "ray tracing");
            break;
        }
    }
}

void App::Display()
{
    k3surf back_buffer = win->GetBackBuffer();
    k3resource back_buffer_resource = back_buffer->GetResource();
    float clear_color[] = { 0.0f, 0.25f, 0.75f, 1.0f };

    k3renderTargets rt = { NULL };
    rt.render_targets[0] = back_buffer;
    rt.depth_target = depth_surf;
    cmd_buf->Reset();
    cmd_buf->TransitionResource(back_buffer_resource, k3resourceState::RENDER_TARGET);
    cmd_buf->ClearRenderTarget(back_buffer, clear_color, NULL);

    uint32_t v = version % NUM_VERSIONS;
    fence->WaitGpuFence(version - NUM_VERSIONS);
    UpdateResources(v);

    if (use_raster) {
        cmd_buf->ClearDepthTarget(depth_surf, k3depthSelect::DEPTH, 0.0f, 0x0, NULL);
        cmd_buf->SetGfxState(raster_state);
        cmd_buf->TransitionResource(cube_mesh->getVertexBuffer()->GetResource(), k3resourceState::SHADER_BUFFER);
        cmd_buf->TransitionResource(cube_mesh->getAttribBuffer()->GetResource(), k3resourceState::SHADER_BUFFER);
        cmd_buf->SetVertexBuffer(0, cube_mesh->getVertexBuffer());
        cmd_buf->SetVertexBuffer(1, cube_mesh->getAttribBuffer());
        cmd_buf->SetConstantBuffer(0, cb_camera[v]);
        cmd_buf->SetShaderView(1, cube_mesh->getTexture(0));
        cmd_buf->SetConstantBuffer(2, cb_obj[v]);
        cmd_buf->SetConstantBuffer(4, cube_mesh->getLightBuffer());
        cmd_buf->SetDrawPrim(k3drawPrimType::TRIANGLELIST);
        cmd_buf->SetViewToSurface(back_buffer_resource);
        cmd_buf->SetRenderTargets(&rt);
        uint32_t i;
        for (i = 0; i < cube_mesh->getNumObjects(); i++) {
            cmd_buf->SetConstant(3, i);
            cmd_buf->Draw(3 * cube_mesh->getNumPrims(i), 3 * cube_mesh->getStartPrim(i));
        }
    } else {
        cmd_buf->SetRTState(rt_state);
        k3rtDispatch rt_desc;
        rt_desc.width = win->GetWidth();
        rt_desc.height = win->GetHeight();
        rt_desc.depth = 1;
        rt_desc.state_table = rt_state_table[v];
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
    }

    cmd_buf->TransitionResource(back_buffer_resource, k3resourceState::COMMON);
    cmd_buf->Close();
    gfx->SubmitCmdBuf(cmd_buf);
    version = fence->SetGpuFence(k3gpuQueue::GRAPHICS);

    win->SwapBuffer();
}

void App::Resize(uint32_t width, uint32_t height)
{
    gfx->WaitGpuIdle();

    k3resourceDesc rdesc;
    win->GetBackBuffer()->GetResource()->getDesc(&rdesc);

    k3viewDesc vdesc = {};
    vdesc.view_index = rt_out->GetUAVViewIndex();
    rt_out = gfx->CreateSurface(&rdesc, NULL, NULL, &vdesc);

}

void App::MouseMove(uint32_t x, uint32_t y)
{
    if (mouse_button & 0x1) {
        if (cube_mesh->getNumCameras()) {
            float* pos = cube_mesh->getCameraPosition(0);
            float* look = cube_mesh->getCameraLookAt(0);
            float* up = cube_mesh->getCameraUp(0);
            float right[3];
            float look_dir[3];
            k3v3_Sub(look_dir, look, pos);
            k3v3_Cross(right, look_dir, up);
            float rot[9];
            k3m3_SetRotation(rot, deg2rad(((float)last_mouse_y - y) / 16.0f), right);
            k3mv3_Mul(look_dir, rot, look_dir);
            k3mv3_Mul(up, rot, up);
            k3m3_SetRotation(rot, deg2rad(((float)last_mouse_x - x) / 16.0f), up);
            k3mv3_Mul(look_dir, rot, look_dir);
            k3v3_Add(look, pos, look_dir);
        } else {
            eye_rotation_angle += ((float)x - last_mouse_x) / 4.0f;
            eye_attitude_angle += ((float)y - last_mouse_y) / 4.0f;
            if (eye_attitude_angle > 89.0f) eye_attitude_angle = 89.0f;
            if (eye_attitude_angle < -89.0f) eye_attitude_angle = -89.0f;
        }
    }
    last_mouse_x = x;
    last_mouse_y = y;
}

void App::MouseButton(uint32_t x, uint32_t y, uint32_t b, k3keyState state)
{
    uint32_t button_mask = 1 << (b - 1);
    if (state == k3keyState::PRESSED) {
        mouse_button |= button_mask;
    } else if (state == k3keyState::RELEASED) {
        mouse_button &= ~button_mask;
    }
}

void App::MouseScroll(uint32_t x, uint32_t y, int32_t vscroll, int32_t hscroll)
{
    if (cube_mesh->getNumCameras()) {
        float* pos = cube_mesh->getCameraPosition(0);
        float* look = cube_mesh->getCameraLookAt(0);
        float look_dir[3];
        float move = (float)-vscroll;
        k3v3_Sub(look_dir, look, pos);
        k3sv3_Mul(look_dir, move, look_dir);
        k3v3_Add(pos, pos, look_dir);
        k3v3_Add(look, look, look_dir);
    } else {
        eye_distance += vscroll;
        if (eye_distance < 2.0f) eye_distance = 2.0f;
    }
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

void K3CALLBACK App::ResizeCallback(void* data, uint32_t width, uint32_t height)
{
    App* a = (App*)data;
    a->Resize(width, height);
}

void K3CALLBACK App::MouseMoveCallback(void* data, uint32_t x, uint32_t y)
{
    App* a = (App*)data;
    a->MouseMove(x, y);
}

void K3CALLBACK App::MouseButtonCallback(void* data, uint32_t x, uint32_t y, uint32_t b, k3keyState state)
{
    App* a = (App*)data;
    a->MouseButton(x, y, b, state);
}

void K3CALLBACK App::MouseScrollCallback(void* data, uint32_t x, uint32_t y, int32_t vscroll, int32_t hscroll)
{
    App* a = (App*)data;
    a->MouseScroll(x, y, vscroll, hscroll);
}

int main()
{
    App a;
    k3winObj::WindowLoop();

    return 0;
}
