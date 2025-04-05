// k3 graphics test
// simple test to create a window and display a single triangle

#include "k3.h"

struct vertex_t {
    float position[4];
    float color[4];
    float tcoord[2];
};

class App
{
private:
    static const uint32_t NUM_VERSIONS = 4;
    k3win win;
    k3gfx gfx;
    k3cmdBuf cmd_buf;
    k3shaderBinding main_binding;
    k3gfxState main_state;
    k3buffer vertex_buffer;
    k3surf texture;
    k3sampler sampler;
    k3font font;
    k3soundBuf sbuf;
    k3sampleData sound_sample0;
    k3sampleData sound_sample1;
    k3uploadBuffer cb_upload_move;
    k3buffer cb_move[NUM_VERSIONS];
    uint32_t cb_move_version;
    bool cb_move_dirty;

public:
    App();
    ~App();

    void Setup();
    void Keyboard(k3key k, char c, k3keyState state);
    void Display();
    static void K3CALLBACK KeyboardCallback(void* data, k3key k, char c, k3keyState state);
    static void K3CALLBACK JoystickAdded(void* data, uint32_t joystick, const k3joyInfo* joy_info, const k3joyState* joy_state);
    static void K3CALLBACK JoystickRemoved(void* data, uint32_t joystick);
    static void K3CALLBACK JoystickMove(void* data, uint32_t joystick, uint32_t axis_num, k3joyAxis axis, uint32_t ordinal, float position);
    static void K3CALLBACK JoystickButton(void* data, uint32_t joystick, uint32_t button, k3keyState state);
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
    win = k3winObj::CreateWindowed("onetri", 100, 100, 640, 480, 128, 32);
    win->SetKeyboardFunc(KeyboardCallback);
    win->SetJoystickFunc(JoystickAdded, JoystickRemoved, JoystickMove, JoystickButton);
    win->SetDisplayFunc(DisplayCallback);
    win->SetIdleFunc(DisplayCallback);
    win->SetVisible(true);
    win->SetCursorVisible(true);
    win->SetDataPtr(this);

    gfx = win->GetGfx();
    sbuf = win->CreateSoundBuffer(2, 44100, 16, 8192, 2);
    sound_sample0 = k3sampleDataObj::Create();
    sound_sample1 = k3sampleDataObj::Create();
    sound_sample0->LoadFromFile("..\\test\\assets\\144862.mp3");
    sound_sample1->LoadFromFile("..\\test\\assets\\59992.flac");
    printf("Adapter: %s\n", gfx->AdapterName());
    printf("Raytracing Tier supported: %d\n", gfx->GetRayTracingSupport());

    cmd_buf = gfx->CreateCmdBuf();

    k3bindingParam bind_params[3];
    bind_params[0].type = k3bindingType::VIEW_SET;
    bind_params[0].view_set_desc.type = k3shaderBindType::SRV;
    bind_params[0].view_set_desc.num_views = 1;
    bind_params[0].view_set_desc.reg = 0;
    bind_params[0].view_set_desc.space = 0;
    bind_params[0].view_set_desc.offset = 0;
    bind_params[1].type = k3bindingType::VIEW_SET;
    bind_params[1].view_set_desc.type = k3shaderBindType::SAMPLER;
    bind_params[1].view_set_desc.num_views = 1;
    bind_params[1].view_set_desc.reg = 0;
    bind_params[1].view_set_desc.space = 0;
    bind_params[1].view_set_desc.offset = 1;
    bind_params[2].type = k3bindingType::VIEW_SET;
    bind_params[2].view_set_desc.type = k3shaderBindType::CBV;
    bind_params[2].view_set_desc.num_views = 1;
    bind_params[2].view_set_desc.reg = 0;
    bind_params[2].view_set_desc.space = 0;
    bind_params[2].view_set_desc.offset = 2;
    main_binding = gfx->CreateShaderBinding(3, bind_params, 0, NULL);

    k3shader vs, ps;
    vs = gfx->CreateShaderFromCompiledFile("simple_vs.cso");
    ps = gfx->CreateShaderFromCompiledFile("simple_ps.cso");
    //vs = gfx->CompileShaderFromFile("..\\test\\shaders\\simple_vs.hlsl", k3shaderType::VERTEX_SHADER);
    //ps = gfx->CompileShaderFromFile("..\\test\\shaders\\simple_ps.hlsl", k3shaderType::PIXEL_SHADER);

    k3inputElement elem[3];
    elem[0].name = "POSITION";
    elem[0].index = 0;
    elem[0].format = k3fmt::RGBA32_FLOAT;
    elem[0].slot = 0;
    elem[0].offset = 0;
    elem[0].in_type = k3inputType::VERTEX;
    elem[0].instance_step = 0;
    elem[1].name = "COLOR";
    elem[1].index = 0;
    elem[1].format = k3fmt::RGBA32_FLOAT;
    elem[1].slot = 0;
    elem[1].offset = 16;
    elem[1].in_type = k3inputType::VERTEX;
    elem[1].instance_step = 0;
    elem[2].name = "TEXCOORD";
    elem[2].index = 0;
    elem[2].format = k3fmt::RG32_FLOAT;
    elem[2].slot = 0;
    elem[2].offset = 32;
    elem[2].in_type = k3inputType::VERTEX;
    elem[2].instance_step = 0;
    k3gfxStateDesc state_desc = { 0 };
    state_desc.num_input_elements = 3;
    state_desc.input_elements = elem;
    state_desc.shader_binding = main_binding;
    state_desc.vertex_shader = vs;
    state_desc.pixel_shader = ps;
    state_desc.sample_mask = ~0;
    state_desc.rast_state.fill_mode = k3fill::SOLID;
    state_desc.rast_state.cull_mode = k3cull::NONE;
    state_desc.rast_state.front_counter_clockwise = false;
    state_desc.blend_state.blend_op[0].rt_write_mask = 0xf;
    state_desc.prim_type = k3primType::TRIANGLE;
    state_desc.num_render_targets = 1;
    state_desc.rtv_format[0] = k3fmt::RGBA8_UNORM;
    state_desc.msaa_samples = 1;
    main_state = gfx->CreateGfxState(&state_desc);

    k3uploadBuffer buf = gfx->CreateUploadBuffer();
    vertex_t* verts = (vertex_t*)buf->MapForWrite(3 * sizeof(vertex_t));
    verts[0].position[0] = 0.25f;
    verts[0].position[1] = 0.25f;
    verts[0].position[2] = 1.0f;
    verts[0].position[3] = 1.0f;
    verts[0].color[0] = 1.0f;
    verts[0].color[1] = 0.0f;
    verts[0].color[2] = 0.0f;
    verts[0].color[3] = 1.0f;
    verts[0].tcoord[0] = 0.0f;
    verts[0].tcoord[1] = 0.0f;
    verts[1].position[0] = 0.75f;
    verts[1].position[1] = 0.25f;
    verts[1].position[2] = 1.0f;
    verts[1].position[3] = 1.0f;
    verts[1].color[0] = 0.0f;
    verts[1].color[1] = 1.0f;
    verts[1].color[2] = 0.0f;
    verts[1].color[3] = 1.0f;
    verts[1].tcoord[0] = 1.0f;
    verts[1].tcoord[1] = 0.0f;
    verts[2].position[0] = 0.25f;
    verts[2].position[1] = 0.75f;
    verts[2].position[2] = 1.0f;
    verts[2].position[3] = 1.0f;
    verts[2].color[0] = 0.0f;
    verts[2].color[1] = 0.0f;
    verts[2].color[2] = 1.0f;
    verts[2].color[3] = 1.0f;
    verts[2].tcoord[0] = 0.0f;
    verts[2].tcoord[1] = 1.0f;
    buf->Unmap();
    k3bufferDesc bdesc = { 0 };
    bdesc.size = 3 * sizeof(vertex_t);
    bdesc.stride = sizeof(vertex_t);
    vertex_buffer = gfx->CreateBuffer(&bdesc);
    k3resource buffer_resource = vertex_buffer->GetResource();

    k3uploadImage img = gfx->CreateUploadImage();
    k3imageObj::LoadFromFile(img, "..\\test\\assets\\Desert253_rgba8.DDS");
    k3resourceDesc rdesc;
    img->GetDesc(&rdesc);
    k3viewDesc vdesc;
    memset(&vdesc, 0, sizeof(k3viewDesc));
    vdesc.view_index = 0;
    texture = gfx->CreateSurface(&rdesc, NULL, &vdesc, NULL);
    k3resource texture_resource = texture->GetResource();

    cb_upload_move = gfx->CreateUploadBuffer();
    void* cb_move_data = cb_upload_move->MapForWrite(4 * sizeof(float));
    memset(cb_move_data, 0, 4 * sizeof(float));
    cb_upload_move->Unmap();
    cb_move_version = 0;
    cb_move_dirty = false;
    uint32_t i;
    k3bufferDesc cb_move_desc = { 0 };
    cb_move_desc.size = 4 * sizeof(float);
    cb_move_desc.view_index = 2;
    for (i = 0; i < NUM_VERSIONS; i++) {
        cb_move[i] = gfx->CreateBuffer(&cb_move_desc);
        cb_move_desc.view_index++;
    }

    cmd_buf->Reset();
    cmd_buf->TransitionResource(buffer_resource, k3resourceState::COPY_DEST);
    cmd_buf->UploadBuffer(buf, buffer_resource);
    cmd_buf->TransitionResource(texture_resource, k3resourceState::COPY_DEST);
    cmd_buf->UploadImage(img, texture_resource);
    cmd_buf->TransitionResource(cb_move[0]->GetResource(), k3resourceState::COPY_DEST);
    cmd_buf->UploadBuffer(cb_upload_move, cb_move[0]->GetResource());
    cmd_buf->Close();
    gfx->SubmitCmdBuf(cmd_buf);

    k3samplerDesc sdesc = { 0 };
    sdesc.filter = k3texFilter::MIN_MAG_MIP_LINEAR;
    sdesc.addr_u = k3texAddr::CLAMP;
    sdesc.addr_v = k3texAddr::CLAMP;
    sdesc.addr_w = k3texAddr::CLAMP;
    sampler = gfx->CreateSampler(&sdesc);

    k3fontDesc fdesc = { 0 };
    fdesc.view_index = 6;
    fdesc.name = "..\\test\\assets\\LapsusPro-Bold.otf";
    fdesc.point_size = 32.0f;
    fdesc.style = k3fontStyle::NORMAL;
    fdesc.weight = k3fontWeight::NORMAL;
    fdesc.cmd_buf = cmd_buf;
    fdesc.format = k3fmt::RGBA8_UNORM;
    fdesc.transparent = false;
    font = gfx->CreateFont(&fdesc);

    gfx->WaitGpuIdle();
}

void App::Keyboard(k3key k, char c, k3keyState state)
{
    float* cb_move_data = NULL;
    if (state == k3keyState::PRESSED) {
        switch (k) {
        case k3key::ESCAPE:
            k3winObj::ExitLoop();
            break;
        case k3key::UP:
            cb_move_data = (float*)cb_upload_move->MapForWrite(4 * sizeof(float));
            cb_move_data[1] += 0.10f;
            cb_upload_move->Unmap();
            cb_move_dirty = true;
            sbuf->AttachSampleStream(1, sound_sample0);
            break;
        case k3key::DOWN:
            cb_move_data = (float*)cb_upload_move->MapForWrite(4 * sizeof(float));
            cb_move_data[1] -= 0.10f;
            cb_upload_move->Unmap();
            cb_move_dirty = true;
            break;
        case k3key::LEFT:
            cb_move_data = (float*)cb_upload_move->MapForWrite(4 * sizeof(float));
            cb_move_data[0] -= 0.10f;
            cb_upload_move->Unmap();
            cb_move_dirty = true;
            sbuf->AttachSampleStream(0, sound_sample1);
            break;
        case k3key::RIGHT:
            cb_move_data = (float*)cb_upload_move->MapForWrite(4 * sizeof(float));
            cb_move_data[0] += 0.10f;
            cb_upload_move->Unmap();
            cb_move_dirty = true;
            break;
        }
    }
}

void App::Display()
{
    sbuf->PlayStreams();
    k3surf back_buffer = win->GetBackBuffer();
    k3resource back_buffer_resource = back_buffer->GetResource();
    float clear_color[] = { 0.5f, 0.0f, 0.5f, 1.0f };
    float text_fg_color[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    float text_bg_color[] = { 0.0, 0.0, 0.0, 1.0 };
    k3renderTargets rt = { NULL };
    rt.render_targets[0] = back_buffer;
    cmd_buf->Reset();
    cmd_buf->TransitionResource(back_buffer_resource, k3resourceState::RENDER_TARGET);
    cmd_buf->ClearRenderTarget(back_buffer, clear_color, NULL);
    cmd_buf->SetGfxState(main_state);
    cmd_buf->TransitionResource(vertex_buffer->GetResource(), k3resourceState::SHADER_BUFFER);
    cmd_buf->SetVertexBuffer(0, vertex_buffer);
    cmd_buf->TransitionResource(texture->GetResource(), k3resourceState::SHADER_RESOURCE);
    cmd_buf->SetShaderView(0, texture);
    cmd_buf->SetSampler(1, sampler);
    if (cb_move_dirty) {
        cb_move_version++;
        if (cb_move_version >= NUM_VERSIONS) cb_move_version = 0;
        cmd_buf->TransitionResource(cb_move[cb_move_version]->GetResource(), k3resourceState::COPY_DEST);
        cmd_buf->UploadBuffer(cb_upload_move, cb_move[cb_move_version]->GetResource());
        cb_move_dirty = false;
    }
    cmd_buf->TransitionResource(cb_move[cb_move_version]->GetResource(), k3resourceState::SHADER_BUFFER);
    cmd_buf->SetConstantBuffer(2, cb_move[cb_move_version]);
    cmd_buf->SetDrawPrim(k3drawPrimType::TRIANGLELIST);
    cmd_buf->SetViewToSurface(back_buffer_resource);
    cmd_buf->SetRenderTargets(&rt);
    cmd_buf->Draw(3);
    cmd_buf->DrawText("Hi", font, text_fg_color, text_bg_color, -5, -10, k3fontAlignment::TOP_LEFT);
    cmd_buf->DrawText("Me Oh My Everyone", font, text_fg_color, text_bg_color, 0, 0, k3fontAlignment::TOP_CENTER);
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

void K3CALLBACK App::JoystickAdded(void* data, uint32_t joystick, const k3joyInfo* joy_info, const k3joyState* joy_state)
{
    printf("Joystick %d added\n", joystick);
}

void K3CALLBACK App::JoystickRemoved(void* data, uint32_t joystick)
{
    printf("Joystick %d removed\n", joystick);
}

void K3CALLBACK App::JoystickMove(void* data, uint32_t joystick, uint32_t axis_num, k3joyAxis axis, uint32_t ordinal, float position)
{
    printf("joy 0x%x: a: %d o: %d pos: %f\n", joystick, axis_num, ordinal, position);
}
void K3CALLBACK App::JoystickButton(void* data, uint32_t joystick, uint32_t button, k3keyState state)
{
    printf("joy 0x%x: b: %d state: %d\n", joystick, button, state);
}

int main()
{
    App a;

    k3winObj::WindowLoop();

    return 0;
}
