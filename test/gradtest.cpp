// k3 graphics test
// test to display a gradient background with rotating thin object in the foreground

#include "k3.h"

struct vertex_t {
    float position[4];
    float color[4];
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
    k3buffer vb_background;
    k3buffer vb_foreground;
    k3font font;
    k3buffer cb_xform_background;
    k3uploadBuffer cb_upload_xform[NUM_VERSIONS];
    k3buffer cb_xform_foreground[NUM_VERSIONS];
    k3downloadImage screenshot[NUM_VERSIONS];
    k3fence fence;
    uint32_t mouse_button;
    uint32_t cb_version;
    uint32_t captured_screenshot;
    float rot_angle;
    const float rot_inc = deg2rad(0.1f);

public:
    App();
    ~App();

    void Setup();
    void Keyboard(k3key k, char c, k3keyState state);
    void Display();
    void MouseButton(uint32_t x, uint32_t y, uint32_t b, k3keyState state);
    static void K3CALLBACK KeyboardCallback(void* data, k3key k, char c, k3keyState state);
    static void K3CALLBACK DisplayCallback(void* data);
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
    win = k3winObj::CreateWindowed("gradtest", 100, 100, 1024, 1024, 128, 32);

    gfx = win->GetGfx();
    printf("Adapter: %s\n", gfx->AdapterName());
    printf("Raytracing Tier supported: %d\n", gfx->GetRayTracingSupport());

    cmd_buf = gfx->CreateCmdBuf();

    k3bindingParam bind_params[1];
    bind_params[0].type = k3bindingType::VIEW_SET;
    bind_params[0].view_set_desc.type = k3shaderBindType::CBV;
    bind_params[0].view_set_desc.num_views = 1;
    bind_params[0].view_set_desc.reg = 0;
    bind_params[0].view_set_desc.space = 0;
    bind_params[0].view_set_desc.offset = 0;
    main_binding = gfx->CreateShaderBinding(1, bind_params, 0, NULL);

    k3shader vs, ps;
    vs = gfx->CreateShaderFromCompiledFile("color_vs.cso");
    ps = gfx->CreateShaderFromCompiledFile("color_ps.cso");

    k3inputElement elem[2];
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

    k3gfxStateDesc state_desc = { 0 };
    state_desc.num_input_elements = 2;
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

    k3uploadBuffer bg_upbuf = gfx->CreateUploadBuffer();
    vertex_t* verts = (vertex_t*)bg_upbuf->MapForWrite(4 * sizeof(vertex_t));
    verts[0].position[0] = -1.0f;
    verts[0].position[1] = 1.0f;
    verts[0].position[2] = 1.0f;
    verts[0].position[3] = 1.0f;
    verts[0].color[0] = 0.5f;
    verts[0].color[1] = 0.5f;
    verts[0].color[2] = 1.0f;
    verts[0].color[3] = 1.0f;
    verts[1].position[0] = 1.0f;
    verts[1].position[1] = 1.0f;
    verts[1].position[2] = 1.0f;
    verts[1].position[3] = 1.0f;
    verts[1].color[0] = 0.5f;
    verts[1].color[1] = 0.5f;
    verts[1].color[2] = 1.0f;
    verts[1].color[3] = 1.0f;
    verts[2].position[0] = -1.0f;
    verts[2].position[1] = -1.0f;
    verts[2].position[2] = 1.0f;
    verts[2].position[3] = 1.0f;
    verts[2].color[0] = 1.0f;
    verts[2].color[1] = 1.0f;
    verts[2].color[2] = 1.0f;
    verts[2].color[3] = 1.0f;
    verts[3].position[0] = 1.0f;
    verts[3].position[1] = -1.0f;
    verts[3].position[2] = 1.0f;
    verts[3].position[3] = 1.0f;
    verts[3].color[0] = 1.0f;
    verts[3].color[1] = 1.0f;
    verts[3].color[2] = 1.0f;
    verts[3].color[3] = 1.0f;
    bg_upbuf->Unmap();

    k3bufferDesc bdesc = { 0 };
    bdesc.size = 4 * sizeof(vertex_t);
    bdesc.stride = sizeof(vertex_t);
    vb_background = gfx->CreateBuffer(&bdesc);
    k3resource vb_background_resc = vb_background->GetResource();

    k3uploadBuffer fg_upbuf = gfx->CreateUploadBuffer();
    verts = (vertex_t*)fg_upbuf->MapForWrite(4 * sizeof(vertex_t));
    verts[0].position[0] = -0.75f;
    verts[0].position[1] = 0.00125f;
    verts[0].position[2] = 1.0f;
    verts[0].position[3] = 1.0f;
    verts[0].color[0] = 1.0f;
    verts[0].color[1] = 1.0f;
    verts[0].color[2] = 0.0f;
    verts[0].color[3] = 1.0f;
    verts[1].position[0] = 0.75f;
    verts[1].position[1] = 0.00125f;
    verts[1].position[2] = 1.0f;
    verts[1].position[3] = 1.0f;
    verts[1].color[0] = 1.0f;
    verts[1].color[1] = 0.0f;
    verts[1].color[2] = 1.0f;
    verts[1].color[3] = 1.0f;
    verts[2].position[0] = -0.75f;
    verts[2].position[1] = -0.00125f;
    verts[2].position[2] = 1.0f;
    verts[2].position[3] = 1.0f;
    verts[2].color[0] = 1.0f;
    verts[2].color[1] = 1.0f;
    verts[2].color[2] = 0.0f;
    verts[2].color[3] = 1.0f;
    verts[3].position[0] = 0.75f;
    verts[3].position[1] = -0.00125f;
    verts[3].position[2] = 1.0f;
    verts[3].position[3] = 1.0f;
    verts[3].color[0] = 1.0f;
    verts[3].color[1] = 0.0f;
    verts[3].color[2] = 1.0f;
    verts[3].color[3] = 1.0f;
    fg_upbuf->Unmap();

    bdesc.size = 4 * sizeof(vertex_t);
    bdesc.stride = sizeof(vertex_t);
    vb_foreground = gfx->CreateBuffer(&bdesc);
    k3resource vb_foreground_resc = vb_foreground->GetResource();

    uint32_t view_index = 0;
    float* cb_data;
    uint32_t i;
    bdesc.size = 16 * sizeof(float);
    bdesc.stride = 0;
    bdesc.view_index = view_index;
    bdesc.shader_resource = true;
    for (i = 0; i < NUM_VERSIONS; i++) {
        cb_upload_xform[i] = gfx->CreateUploadBuffer();
        cb_data = (float*)cb_upload_xform[i]->MapForWrite(16 * sizeof(float));
        k3m4_SetIdentity(cb_data);
        cb_upload_xform[i]->Unmap();
        cb_xform_foreground[i] = gfx->CreateBuffer(&bdesc);
        view_index++;
        bdesc.view_index = view_index;
        screenshot[i] = gfx->CreateDownloadImage();
    }

    fence = gfx->CreateFence();

    cb_xform_background = gfx->CreateBuffer(&bdesc);

    k3resource resc;
    cmd_buf->Reset();
    cmd_buf->TransitionResource(vb_foreground_resc, k3resourceState::COPY_DEST);
    cmd_buf->UploadBuffer(fg_upbuf, vb_foreground_resc);
    cmd_buf->TransitionResource(vb_background_resc, k3resourceState::COPY_DEST);
    cmd_buf->UploadBuffer(bg_upbuf, vb_background_resc);
    resc = cb_xform_background->GetResource();
    cmd_buf->TransitionResource(resc, k3resourceState::COPY_DEST);
    cmd_buf->UploadBuffer(cb_upload_xform[0], resc);
    cmd_buf->TransitionResource(resc, k3resourceState::SHADER_BUFFER);
    //resc = cb_xform_foreground[0]->GetResource();
    //cmd_buf->TransitionResource(resc, k3resourceState::COPY_DEST);
    //cmd_buf->UploadBuffer(cb_upload_xform[0], resc);
    //cmd_buf->TransitionResource(resc, k3resourceState::SHADER_BUFFER);
    cmd_buf->TransitionResource(vb_background_resc, k3resourceState::SHADER_BUFFER);
    cmd_buf->TransitionResource(vb_foreground_resc, k3resourceState::SHADER_BUFFER);
    cmd_buf->Close();
    gfx->SubmitCmdBuf(cmd_buf);

    for (i = 0; i < NUM_VERSIONS; i++) {
        cb_version = fence->SetGpuFence(k3gpuQueue::GRAPHICS);
    }

    win->SetMouseFunc(MouseMoveCallback, MouseButtonCallback, MouseScrollCallback);
    win->SetKeyboardFunc(KeyboardCallback);
    win->SetDisplayFunc(DisplayCallback);
    win->SetIdleFunc(DisplayCallback);
    win->SetVisible(true);
    win->SetCursorVisible(true);
    win->SetDataPtr(this);
    win->SetVsyncInterval(1);

    mouse_button = 0;
    rot_angle = 0.0f;
    captured_screenshot = 0;

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

void App::MouseButton(uint32_t x, uint32_t y, uint32_t b, k3keyState state)
{
    uint32_t button_mask = 1 << (b - 1);
    if (state == k3keyState::PRESSED) {
        mouse_button |= button_mask;
    } else if (state == k3keyState::RELEASED) {
        mouse_button &= ~button_mask;
    }
}

void App::Display()
{
    k3surf back_buffer = win->GetBackBuffer();
    k3resource back_buffer_resource = back_buffer->GetResource();
    float clear_color[] = { 0.5f, 0.0f, 0.5f, 1.0f };
    k3renderTargets rt = { NULL };
    rt.render_targets[0] = back_buffer;
    cmd_buf->Reset();

    uint32_t v = cb_version % NUM_VERSIONS;
    fence->WaitGpuFence(cb_version - NUM_VERSIONS);

    if (mouse_button && captured_screenshot >= NUM_VERSIONS) {
        char filename[64];
        sprintf(filename, "..//test//screenshots//screen%03d.dds", captured_screenshot - NUM_VERSIONS);
        screenshot[v]->SaveToFile(filename, k3imageObj::FILE_HANDLER_DDS);
    }

    float rot_axis[] = { 0.0f, 0.0f, -1.0f };
    float* cb_data = (float*)cb_upload_xform[v]->MapForWrite(16 * sizeof(float));
    k3m4_SetRotation(cb_data, rot_angle, rot_axis);
    rot_angle += rot_inc;
    cb_upload_xform[v]->Unmap();
    k3resource resc = cb_xform_foreground[v]->GetResource();
    cmd_buf->TransitionResource(resc, k3resourceState::COPY_DEST);
    cmd_buf->UploadBuffer(cb_upload_xform[v], resc);
    cmd_buf->TransitionResource(resc, k3resourceState::SHADER_BUFFER);

    cmd_buf->TransitionResource(back_buffer_resource, k3resourceState::RENDER_TARGET);
    cmd_buf->ClearRenderTarget(back_buffer, clear_color, NULL);
    cmd_buf->SetGfxState(main_state);
    cmd_buf->SetViewToSurface(back_buffer_resource);
    cmd_buf->SetRenderTargets(&rt);
    cmd_buf->SetVertexBuffer(0, vb_background);
    cmd_buf->SetConstantBuffer(0, cb_xform_background);
    cmd_buf->SetDrawPrim(k3drawPrimType::TRIANGLESTRIP);
    cmd_buf->Draw(4);

    cmd_buf->SetVertexBuffer(0, vb_foreground);
    cmd_buf->SetConstantBuffer(0, cb_xform_foreground[v]);
    cmd_buf->SetDrawPrim(k3drawPrimType::TRIANGLESTRIP);
    cmd_buf->Draw(4);
    //cmd_buf->DrawText("Hi", font, text_fg_color, text_bg_color, -5, -10, k3fontAlignment::TOP_LEFT);
    //cmd_buf->DrawText("Me Oh My Everyone", font, text_fg_color, text_bg_color, 0, 0, k3fontAlignment::TOP_CENTER);

    if (mouse_button) {
        cmd_buf->TransitionResource(back_buffer_resource, k3resourceState::COPY_SOURCE);
        cmd_buf->DownloadImage(screenshot[v], back_buffer_resource);
        captured_screenshot++;
    }

    cmd_buf->TransitionResource(back_buffer_resource, k3resourceState::COMMON);
    cmd_buf->Close();
    gfx->SubmitCmdBuf(cmd_buf);
    cb_version = fence->SetGpuFence(k3gpuQueue::GRAPHICS);

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

void K3CALLBACK App::MouseMoveCallback(void* data, uint32_t x, uint32_t y)
{ }

void K3CALLBACK App::MouseButtonCallback(void* data, uint32_t x, uint32_t y, uint32_t b, k3keyState state)
{
    App* a = (App*)data;
    a->MouseButton(x, y, b, state);
}

void K3CALLBACK App::MouseScrollCallback(void* data, uint32_t x, uint32_t y, int32_t vscroll, int32_t hscroll)
{ }

int main()
{
    App a;
    k3winObj::WindowLoop();

    return 0;
}
