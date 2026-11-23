// animtest.cpp : Tests animation.
//

#include <k3.h>

struct obj_prop_t {
    float world[16];
    float iworld[16];
    float diffuse_color[3];
    uint32_t diffuse_map_index;
    uint32_t normal_map_index;
    uint32_t prim_start;
    uint32_t bone_mat_start;
    uint32_t bone_tex_index;
    // anim_coord[0] is the coordinate for the first frame
    // anim_coord[1] is the coordinate for the second frame
    // anim_coord[2] is the interpolation between the two
    float anim_coord[3];
    uint32_t dummy2;
};

struct terrain_vert_t {
    float pos[3];
    float normal[3];
    float texcoord[2];
    float tang[3];
};

class App
{
private:
    static const uint32_t NUM_VERSIONS = 4;
    static const uint32_t TERRAIN_RES_X = 16;
    static const uint32_t TERRAIN_RES_Y = 16;

    k3win win;
    k3gfx gfx;
    k3cmdBuf cmd_buf;
    k3fence fence;
    uint64_t version;

    k3surf depth_surf;
    k3gfxState raster_state;
    k3computeState compute_bones;
    k3mesh scene;

    k3buffer terrain_ib;
    k3buffer terrain_vb;
    k3gfxState terrain_state;
    k3surf terrain_diffuse_map;
    k3surf terrain_normal_map;
    k3surf terrain_rotate_map;

    k3uploadBuffer cb_camera_upload[NUM_VERSIONS];
    k3buffer cb_camera[NUM_VERSIONS];
    k3uploadBuffer cb_obj_upload[NUM_VERSIONS];
    k3buffer cb_obj[NUM_VERSIONS];
    //k3uploadBuffer cb_bone_upload[NUM_VERSIONS];
    //k3buffer cb_bone[NUM_VERSIONS];
    k3buffer cb_bone;

    k3timer timer;

    float eye_distance;
    float eye_rotation_angle;
    float eye_attitude_angle;

    uint32_t last_mouse_x, last_mouse_y;
    uint32_t mouse_button;
    uint32_t anim;
    float anim_transition;
    static const uint32_t ANIM_TRANSITION_TIME_MSEC = 1000;

public:
    App();
    ~App();

    void SetupTerrain(k3uploadBuffer up_ib, k3uploadBuffer up_vb, uint32_t res_x, uint32_t res_y, float width, float height, float* height_map);
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
    k3gfxObj::PreferLowPowerGPU(true);
    win = k3winObj::CreateWindowed("animtest", 100, 100, 640, 480, 128, 32);

    gfx = win->GetGfx();
    printf("Adapter: %s\n", gfx->AdapterName());
    if (gfx->GetNonNormalizedSamplerSupport()) {
        printf("Non-normalized sampler supported\n");
    } else {
        printf("Non-normalized sampler not supported, update driver\n");
    }

    cmd_buf = gfx->CreateCmdBuf();
    uint32_t view_index = 0;

    _chdir("..\\test\\assets");
    const char* custom_prop_names[] = { "Solid" };
    const char* custom_anim_prop_names[] = { "interesting_number", "interesting_string" };
    k3meshDesc mdesc = {};
    mdesc.view_index = view_index;
    mdesc.cmd_buf = cmd_buf;
    mdesc.up_buf = gfx->CreateUploadBuffer();
    mdesc.name = "african-elephant-exp2.fbx";
    mdesc.num_custom_model_props = 1;
    mdesc.custom_model_prop_name = custom_prop_names;
    mdesc.num_custom_anim_props = 2;
    mdesc.custom_anim_prop_name = custom_anim_prop_names;
    scene = gfx->CreateMeshAS(&mdesc);
    view_index = mdesc.view_index;

    k3uploadImage terrain_diffuse_upload = gfx->CreateUploadImage();
    k3uploadImage terrain_normal_upload = gfx->CreateUploadImage();
    k3uploadImage terrain_rotate_upload = gfx->CreateUploadImage();
    k3imageObj::LoadFromFile(terrain_diffuse_upload, "brown_mud_leaves_01_diff_1k.dds");
    k3imageObj::LoadFromFile(terrain_normal_upload, "brown_mud_leaves_01_nor_gl_1k.png");
    k3imageObj::LoadFromFile(terrain_rotate_upload, "voronoi.png");
    _chdir("..\\..\\bin");

    uint32_t i;
    // if there are any cameras, set window resolution to the first one
    if (scene->getNumCameras()) {
        uint32_t win_width, win_height;
        scene->getCameraResolution(0, &win_width, &win_height);
        win->SetSize(win_width, win_height);
        // set all cameras to have infinite far plane
        for (i = 0; i < scene->getNumCameras(); i++) {
            if (scene->getCameraProjectionType(i) == k3projType::PERSPECTIVE) {
                scene->setCameraFarPlane(i, INFINITY);
            }
        }
    }

    // Make window visible after setting size
    win->SetVisible(true);
    win->SetCursorVisible(true);

    k3resourceDesc rdesc;
    win->GetBackBuffer()->GetResource()->getDesc(&rdesc);
    k3viewDesc vdesc = {};
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
    uint32_t num_objects = scene->getNumObjects();
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
    uint32_t num_bones = scene->getNumBones();
    if (num_bones) {
        //for (i = 0; i < NUM_VERSIONS; i++) {
        //    cb_bone_upload[i] = gfx->CreateUploadBuffer();
        //    cb_bone_upload[i]->MapForWrite(num_bones * 32 * sizeof(float));
        //    cb_bone_upload[i]->Unmap();
        //    bdesc.view_index = view_index++;
        //    bdesc.size = num_bones * 32 * sizeof(float);
        //    bdesc.shader_resource = true;
        //    bdesc.shader_writable = true;
        //    bdesc.stride = 32 * sizeof(float);
        //    cb_bone[i] = gfx->CreateBuffer(&bdesc);
        //}
        bdesc.view_index = view_index++;
        bdesc.size = num_bones * 32 * sizeof(float);
        bdesc.shader_resource = true;
        bdesc.shader_writable = true;
        bdesc.stride = 32 * sizeof(float);
        cb_bone = gfx->CreateBuffer(&bdesc);

        k3viewDesc vdesc = { 0 };
        vdesc.view_index = view_index++;
        scene->genBoneTexture(gfx, cmd_buf, &vdesc);
    }

    k3bindingParam bind_params[6];
    bind_params[0].type = k3bindingType::VIEW_SET;
    bind_params[0].view_set_desc.type = k3shaderBindType::CBV;
    bind_params[0].view_set_desc.num_views = 1;
    bind_params[0].view_set_desc.reg = 0;
    bind_params[0].view_set_desc.space = 0;
    bind_params[0].view_set_desc.offset = 0;
    bind_params[1].type = k3bindingType::VIEW_SET;
    bind_params[1].view_set_desc.type = k3shaderBindType::SRV;
    bind_params[1].view_set_desc.num_views = scene->getNumTextures();
    if (bind_params[1].view_set_desc.num_views == 0) bind_params[1].view_set_desc.num_views = 1;
    bind_params[1].view_set_desc.reg = 3;
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
    bind_params[5].type = k3bindingType::VIEW_SET;
    bind_params[5].view_set_desc.type = k3shaderBindType::UAV;
    bind_params[5].view_set_desc.num_views = 1;
    bind_params[5].view_set_desc.reg = 2;
    bind_params[5].view_set_desc.space = 0;
    bind_params[5].view_set_desc.offset = 5;

    k3samplerDesc sdesc = { 0 };
    sdesc.filter = k3texFilter::MIN_MAG_MIP_LINEAR;
    sdesc.addr_u = k3texAddr::WRAP;
    sdesc.addr_v = k3texAddr::WRAP;
    sdesc.addr_w = k3texAddr::CLAMP;
    k3shaderBinding raster_binding = gfx->CreateShaderBinding(6, bind_params, 1, &sdesc);

    k3shader vs, ps;
    vs = gfx->CreateShaderFromCompiledFile("anim3d_vs.cso");
    ps = gfx->CreateShaderFromCompiledFile("anim3d_ps.cso");

    k3inputElement elem[5];
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
    elem[3].name = "BLENDINDICES";
    elem[3].index = 0;
    elem[3].format = k3fmt::RGBA32_UINT;
    elem[3].slot = 2;
    elem[3].offset = 0;
    elem[3].in_type = k3inputType::VERTEX;
    elem[3].instance_step = 0;
    elem[4].name = "BLENDWEIGHT";
    elem[4].index = 0;
    elem[4].format = k3fmt::RGBA32_FLOAT;
    elem[4].slot = 2;
    elem[4].offset = 16;
    elem[4].in_type = k3inputType::VERTEX;
    elem[4].instance_step = 0;

    k3gfxStateDesc state_desc = { 0 };
    state_desc.num_input_elements = 5;
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

    vs = gfx->CreateShaderFromCompiledFile("terrain_vs.cso");
    ps = gfx->CreateShaderFromCompiledFile("terrain_ps.cso");
    bind_params[1].view_set_desc.num_views = 3;
    raster_binding = gfx->CreateShaderBinding(6, bind_params, 1, &sdesc);
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
    elem[1].slot = 0;
    elem[1].offset = 12;
    elem[1].in_type = k3inputType::VERTEX;
    elem[1].instance_step = 0;
    elem[2].name = "TANGENT";
    elem[2].index = 0;
    elem[2].format = k3fmt::RGBA32_FLOAT;
    elem[2].slot = 0;
    elem[2].offset = 28;
    elem[2].in_type = k3inputType::VERTEX;
    elem[2].instance_step = 0;
    state_desc.num_input_elements = 3;
    state_desc.input_elements = elem;
    state_desc.vertex_shader = vs;
    state_desc.pixel_shader = ps;
    terrain_state = gfx->CreateGfxState(&state_desc);

    k3computeStateDesc cs_state_desc = { 0 };
    k3shader cs = gfx->CreateShaderFromCompiledFile("anim3d_cs.cso");
    sdesc.filter = k3texFilter::MIN_MAG_LINEAR_MIP_POINT;
    sdesc.addr_u = k3texAddr::CLAMP;
    sdesc.addr_v = k3texAddr::CLAMP;
    sdesc.addr_w = k3texAddr::CLAMP;
    sdesc.flags = k3samplerDesc::FLAG_NON_NORM_SAMPLER;
    bind_params[1].view_set_desc.num_views = 1;
    k3shaderBinding compute_binding = gfx->CreateShaderBinding(6, bind_params, 1, &sdesc);

    cs_state_desc.compute_shader = cs;
    cs_state_desc.shader_binding = compute_binding;
    compute_bones = gfx->CreateComputeState(&cs_state_desc);

    k3uploadBuffer terrain_up_ib = gfx->CreateUploadBuffer();
    k3uploadBuffer terrain_up_vb = gfx->CreateUploadBuffer();
    SetupTerrain(terrain_up_ib, terrain_up_vb, TERRAIN_RES_X, TERRAIN_RES_Y, 256, 256, NULL);

    k3resource img_res;
    terrain_diffuse_upload->GetDesc(&rdesc);
    vdesc.view_index = view_index++;
    terrain_diffuse_map = gfx->CreateSurface(&rdesc, NULL, &vdesc, NULL);
    terrain_normal_upload->GetDesc(&rdesc);
    vdesc.view_index = view_index++;
    terrain_normal_map = gfx->CreateSurface(&rdesc, NULL, &vdesc, NULL);
    terrain_rotate_upload->GetDesc(&rdesc);
    vdesc.view_index = view_index++;
    terrain_rotate_map = gfx->CreateSurface(&rdesc, NULL, &vdesc, NULL);

    timer = win->CreateTimer();

    k3resource buffer_resource;
    cmd_buf->Reset();
    cmd_buf->TransitionResource(depth_surf->GetResource(), k3resourceState::RENDER_TARGET);
    cmd_buf->TransitionResource(scene->getLightBuffer()->GetResource(), k3resourceState::SHADER_BUFFER);
    buffer_resource = terrain_ib->GetResource();
    cmd_buf->TransitionResource(buffer_resource, k3resourceState::COPY_DEST);
    cmd_buf->UploadBuffer(terrain_up_ib, buffer_resource);
    cmd_buf->TransitionResource(buffer_resource, k3resourceState::COMMON);
    buffer_resource = terrain_vb->GetResource();
    cmd_buf->TransitionResource(buffer_resource, k3resourceState::COPY_DEST);
    cmd_buf->UploadBuffer(terrain_up_vb, buffer_resource);
    cmd_buf->TransitionResource(buffer_resource, k3resourceState::COMMON);
    buffer_resource = terrain_diffuse_map->GetResource();
    cmd_buf->TransitionResource(buffer_resource, k3resourceState::COPY_DEST);
    cmd_buf->UploadImage(terrain_diffuse_upload, buffer_resource);
    cmd_buf->TransitionResource(buffer_resource, k3resourceState::COMMON);
    buffer_resource = terrain_normal_map->GetResource();
    cmd_buf->TransitionResource(buffer_resource, k3resourceState::COPY_DEST);
    cmd_buf->UploadImage(terrain_normal_upload, buffer_resource);
    cmd_buf->TransitionResource(buffer_resource, k3resourceState::COMMON);
    buffer_resource = terrain_rotate_map->GetResource();
    cmd_buf->TransitionResource(buffer_resource, k3resourceState::COPY_DEST);
    cmd_buf->UploadImage(terrain_rotate_upload, buffer_resource);
    cmd_buf->TransitionResource(buffer_resource, k3resourceState::COMMON);
    cmd_buf->Close();
    gfx->SubmitCmdBuf(cmd_buf);
    for (i = 0; i < NUM_VERSIONS; i++) {
        version = fence->SetGpuFence(k3gpuQueue::GRAPHICS);
    }

    last_mouse_x = 0;
    last_mouse_y = 0;
    mouse_button = 0x0;
    anim = 0;
    anim_transition = -1.0f;

    k3AABB scene_aabb;
    scene->getAABB(&scene_aabb, 0, NULL);
    printf("Scene AABB: (%.4f, %.4f, %.4f) - (%.4f, %.4f, %.4f)\n",
        scene_aabb.min[0], scene_aabb.min[1], scene_aabb.min[2],
        scene_aabb.max[0], scene_aabb.max[1], scene_aabb.max[2]);

    gfx->WaitGpuIdle();

    win->SetDataPtr(this);
    win->SetKeyboardFunc(KeyboardCallback);
    win->SetDisplayFunc(DisplayCallback);
    win->SetIdleFunc(DisplayCallback);
    win->SetResizeFunc(ResizeCallback);
    win->SetMouseFunc(MouseMoveCallback, MouseButtonCallback, MouseScrollCallback);
}

void App::SetupTerrain(k3uploadBuffer up_ib, k3uploadBuffer up_vb, uint32_t res_x, uint32_t res_y, float width, float height, float* height_map)
{
    uint32_t num_verts = (res_x + 1) * (res_y + 1);
    uint32_t num_indices = 6 * res_x * res_y;

    k3bufferDesc bdesc = { 0 };
    bdesc.size = num_indices * sizeof(uint32_t);
    bdesc.format = k3fmt::R32_UINT;
    bdesc.stride = 3 * sizeof(uint32_t);
    terrain_ib = gfx->CreateBuffer(&bdesc);

    bdesc.size = num_verts * sizeof(terrain_vert_t);
    bdesc.format = k3fmt::UNKNOWN;
    bdesc.stride = sizeof(terrain_vert_t);
    terrain_vb = gfx->CreateBuffer(&bdesc);

    uint32_t i, j;
    uint32_t* ib_data = (uint32_t*)up_ib->MapForWrite(num_indices * sizeof(uint32_t));
    for (j = 0; j < res_y; j++) {
        for (i = 0; i < res_x; i++) {
            ib_data[0] = (res_x + 1) * j + i;
            ib_data[1] = (res_x + 1) * j + i + 1;
            ib_data[2] = (res_x + 1) * (j + 1) + i;
            ib_data[3] = (res_x + 1) * (j + 1) + i;
            ib_data[4] = (res_x + 1) * j + i + 1;
            ib_data[5] = (res_x + 1) * (j + 1) + i + 1;
            ib_data += 6;
        }
    }
    up_ib->Unmap();

    terrain_vert_t* vb_data = (terrain_vert_t*)up_vb->MapForWrite(num_verts * sizeof(terrain_vert_t));
    float res_width = width / res_x;
    float res_height = height / res_y;
    for (j = 0; j <= res_y; j++) {
        for (i = 0; i <= res_x; i++) {
            vb_data[(res_x + 1) * j + i].pos[0] = i * res_width - width / 2.0f;
            vb_data[(res_x + 1) * j + i].pos[1] = j * res_height - height / 2.0f;
            vb_data[(res_x + 1) * j + i].pos[2] = 0.0f;
            vb_data[(res_x + 1) * j + i].normal[0] = 0.0f;
            vb_data[(res_x + 1) * j + i].normal[1] = 0.0f;
            vb_data[(res_x + 1) * j + i].normal[2] = 1.0f;
            vb_data[(res_x + 1) * j + i].texcoord[0] = 256.0f * (i / width);
            vb_data[(res_x + 1) * j + i].texcoord[1] = 256.0f * (j / height);
            vb_data[(res_x + 1) * j + i].tang[0] = 1.0f;
            vb_data[(res_x + 1) * j + i].tang[1] = 0.0f;
            vb_data[(res_x + 1) * j + i].tang[2] = 0.0f;
        }
    }
    up_vb->Unmap();
}

void App::UpdateResources(uint32_t v)
{
    float* camera_data = (float*)cb_camera_upload[v]->MapForWrite(32 * sizeof(float));
    float mat_data[16];

    if (scene->getNumCameras()) {
        scene->getCameraView(camera_data, 0);
        scene->getCameraProjection(mat_data, 0, false, true, true);
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

    uint32_t num_anims = scene->getNumAnims();
    uint32_t delta_t = timer->GetDeltaTime();
    uint32_t t = timer->GetTime();
    if (num_anims) {
        scene->setAnimation(anim, t, k3meshObj::ANIM_FLAG_MORPHED);
    }

    uint32_t i;
    uint32_t num_objects = scene->getNumObjects();
    obj_prop_t* obj_data = (obj_prop_t*)cb_obj_upload[v]->MapForWrite(num_objects * sizeof(obj_prop_t));
    uint32_t bone_start = 0;
    for (i = 0; i < num_objects; i++) {
        memcpy(obj_data[i].world, scene->getTransform(i), 16 * sizeof(float));
        memcpy(obj_data[i].iworld, scene->getTransform(i), 16 * sizeof(float));
        k3m4_Inverse(obj_data[i].iworld);
        memcpy(obj_data[i].diffuse_color, scene->getDiffuseColor(i), 3 * sizeof(float));
        obj_data[i].diffuse_map_index = scene->getDiffuseMapIndex(i);
        obj_data[i].normal_map_index = scene->getNormalMapIndex(i);
        obj_data[i].prim_start = scene->getStartPrim(i);
        obj_data[i].bone_mat_start = bone_start;
        obj_data[i].bone_tex_index = i;
        if (anim_transition >= 0.0f) {
            anim_transition += ((float)delta_t) / ANIM_TRANSITION_TIME_MSEC;
            if (anim_transition >= 1.0f) {
                anim = !anim;
                // Stop animation transition
                anim_transition = -1.0f;
            }
            obj_data[i].anim_coord[1] = scene->getAnimCoordinate(!anim, t);
        }
        obj_data[i].anim_coord[0] = scene->getAnimCoordinate(anim, t);
        obj_data[i].anim_coord[2] = anim_transition;
        bone_start += scene->getNumBones();
    }
    cb_obj_upload[v]->Unmap();

    //uint32_t num_bones = scene->getNumBones();
    //if (num_bones) {
    //    float* bone_data = (float*)cb_bone_upload[v]->MapForWrite(num_bones * 32 * sizeof(float));
    //    scene->genBoneMatrices(bone_data, true);
    //    cb_bone_upload[v]->Unmap();
    //}

    k3resource buffer_resource;
    buffer_resource = cb_camera[v]->GetResource();
    cmd_buf->TransitionResource(buffer_resource, k3resourceState::COPY_DEST);
    cmd_buf->UploadBuffer(cb_camera_upload[v], buffer_resource);
    cmd_buf->TransitionResource(buffer_resource, k3resourceState::SHADER_BUFFER);
    buffer_resource = cb_obj[v]->GetResource();
    cmd_buf->TransitionResource(buffer_resource, k3resourceState::COPY_DEST);
    cmd_buf->UploadBuffer(cb_obj_upload[v], buffer_resource);
    cmd_buf->TransitionResource(buffer_resource, k3resourceState::SHADER_RESOURCE);
    //if (cb_bone[v] != NULL) {
    //    buffer_resource = cb_bone[v]->GetResource();
    //    cmd_buf->TransitionResource(buffer_resource, k3resourceState::COPY_DEST);
    //    cmd_buf->UploadBuffer(cb_bone_upload[v], buffer_resource);
    //    cmd_buf->TransitionResource(buffer_resource, k3resourceState::SHADER_RESOURCE);
    //}
}

void App::Keyboard(k3key k, char c, k3keyState state)
{
    const char* str = NULL;
    if (state == k3keyState::PRESSED) {
        float rot_axis[3] = { 0.0f, 0.0f, 1.0f };
        float new_xform[16];
        float* model_xform = scene->getTransform(scene->getNumMeshes() - 1);
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
            //anim = !anim;
            if (anim_transition >= 0.0f) {
                // Swap the transition back, continue where the current one left off
                anim_transition = 1.0f - anim_transition;
                anim = !anim;
            } else {
                // Start transition in animation
                anim_transition = 0.0f;
            }
            printf("Anim: %s interesting_numer: %d", scene->getAnimName(anim), scene->getAnimCustomProp(anim, 0).i);
            str = scene->getAnimCustomPropString(anim, 1);
            if (str) {
                printf(" interesting_string: %s\n", str);
            } else {
                printf("\n");
            }
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

    if (scene->getNumBones()) {
        cmd_buf->TransitionResource(cb_bone->GetResource(), k3resourceState::UAV);
        cmd_buf->SetComputeState(compute_bones);
        cmd_buf->SetComputeConstantBuffer(0, cb_camera[v]);
        cmd_buf->SetComputeShaderView(1, scene->getBoneTexture());
        cmd_buf->SetComputeConstantBuffer(2, cb_obj[v]);
        cmd_buf->SetComputeConstantBuffer(4, scene->getLightBuffer());
        cmd_buf->SetComputeConstantBuffer(5, cb_bone);
        cmd_buf->ComputeDispatch(1, 1, 1);
    }

    k3buffer index_buf = scene->getIndexBuffer();
    cmd_buf->TransitionResource(depth_surf->GetResource(), k3resourceState::RENDER_TARGET);
    cmd_buf->ClearDepthTarget(depth_surf, k3depthSelect::DEPTH, 0.0f, 0x0, NULL);
    cmd_buf->SetGfxState(raster_state);
    cmd_buf->TransitionResource(scene->getVertexBuffer()->GetResource(), k3resourceState::SHADER_BUFFER);
    cmd_buf->TransitionResource(scene->getAttribBuffer()->GetResource(), k3resourceState::SHADER_BUFFER);
    cmd_buf->SetIndexBuffer(index_buf);
    cmd_buf->SetVertexBuffer(0, scene->getVertexBuffer());
    cmd_buf->SetVertexBuffer(1, scene->getAttribBuffer());
    cmd_buf->SetVertexBuffer(2, scene->getSkinBuffer());
    cmd_buf->SetConstantBuffer(0, cb_camera[v]);
    cmd_buf->SetShaderView(1, scene->getTexture(0));
    cmd_buf->SetConstantBuffer(2, cb_obj[v]);
    cmd_buf->SetConstantBuffer(4, scene->getLightBuffer());
    if (scene->getNumBones()) {
        //cmd_buf->SetConstantBuffer(5, cb_bone[v]);
        cmd_buf->SetConstantBuffer(5, cb_bone);
    }
    cmd_buf->SetDrawPrim(k3drawPrimType::TRIANGLELIST);
    cmd_buf->SetViewToSurface(back_buffer_resource);
    cmd_buf->SetRenderTargets(&rt);
    uint32_t i;
    for (i = 0; i < scene->getNumObjects(); i++) {
        if (scene->getVisibility(i) > 0.5f) {
            cmd_buf->SetConstant(3, i);
            if (index_buf != NULL) {
                cmd_buf->Draw(3 * scene->getNumPrims(i), scene->getStartVert(i), 1, 0, 3 * scene->getStartPrim(i));
            } else {
                cmd_buf->Draw(3 * scene->getNumPrims(i), 3 * scene->getStartPrim(i));
            }
        }
    }

    cmd_buf->SetGfxState(terrain_state);
    cmd_buf->SetIndexBuffer(terrain_ib);
    cmd_buf->SetVertexBuffer(0, terrain_vb);
    cmd_buf->SetConstantBuffer(0, cb_camera[v]);
    cmd_buf->SetShaderView(1, terrain_diffuse_map);
    cmd_buf->SetConstantBuffer(4, scene->getLightBuffer());
    cmd_buf->Draw(6 * TERRAIN_RES_X * TERRAIN_RES_Y);

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
    rdesc.format = k3fmt::D32_FLOAT;
    depth_surf = gfx->CreateSurface(&rdesc, &vdesc, NULL, NULL);

}

void App::MouseMove(uint32_t x, uint32_t y)
{
    if (mouse_button & 0x1) {
        if (scene->getNumCameras()) {
            float* pos = scene->getCameraPosition(0);
            float* look = scene->getCameraLookAt(0);
            float* up = scene->getCameraUp(0);
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
    if (scene->getNumCameras()) {
        float* pos = scene->getCameraPosition(0);
        float* look = scene->getCameraLookAt(0);
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
