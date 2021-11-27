// k3 graphics library
// windows class
// Date: 10/10/2021

#include "k3internal.h"

// ------------------------------------------------------------
// timer class

k3timerImpl::k3timerImpl()
{
    _is_paused = false;
    _start_time = 0;
    _start_delta_time = 0;
    _pause_time = 0;
}

k3timerImpl::~k3timerImpl()
{ }

k3timerObj::k3timerObj()
{
    _data = new k3timerImpl;
}

k3timerObj::~k3timerObj()
{
    if (_data) {
        delete _data;
        _data = NULL;
    }
}

k3timerImpl* k3timerObj::getImpl()
{
    return _data;
}

const k3timerImpl* k3timerObj::getImpl() const
{
    return _data;
}

K3API uint32_t k3timerObj::GetTime()
{
    uint32_t systime = (_data->_is_paused) ? _data->_pause_time : getSystemTime();
    if (systime < _data->_start_time) {
        systime += _data->_start_time;
    } else {
        systime -= _data->_start_time;
    }
    return systime;
}

K3API uint32_t k3timerObj::GetDeltaTime()
{
    uint32_t systime = (_data->_is_paused) ? _data->_pause_time : getSystemTime();
    uint32_t new_start_delta_time = systime;
    if (systime < _data->_start_delta_time) {
        systime += _data->_start_delta_time;
    } else {
        systime -= _data->_start_delta_time;
    }
    _data->_start_delta_time = new_start_delta_time;
    return systime;
}

K3API void k3timerObj::Pause(bool pause)
{
    if (pause == _data->_is_paused) return;

    _data->_is_paused = pause;
    uint32_t systime = getSystemTime();
    if (pause) {
        _data->_pause_time = systime;
    } else {
        _data->_start_time += systime - _data->_pause_time;
        _data->_start_delta_time += systime - _data->_pause_time;
    }
}

K3API bool k3timerObj::IsPaused()
{
    return _data->_is_paused;
}

K3API void k3timerObj::Reset()
{
    uint32_t systime = getSystemTime();
    _data->_start_time = systime;
    _data->_start_delta_time = systime;
    _data->_pause_time = systime;
}

// ------------------------------------------------------------
// font class

FT_Library k3fontImpl::_ft2_lib = NULL;

k3fontImpl::k3fontImpl()
{
    _version = 0;
    uint32_t i;
    for (i = 0; i < NUM_VERSIONS; i++) {
        _upload_cbuf[i] = NULL;
    }
    _font_cbuf = NULL;
    _font_tex = NULL;
    _font_state = NULL;
    _ascender = 0.0f;
    _descender = 0.0f;
    memset(_char_width, 0, sizeof(uint16_t) * NUM_CHARS);
    memset(_char_height, 0, sizeof(uint16_t) * NUM_CHARS);
    memset(_char_offset_x, 0, sizeof(int16_t) * NUM_CHARS * NUM_CHARS);
    memset(_char_offset_y, 0, sizeof(int16_t) * NUM_CHARS);
}

k3fontImpl::~k3fontImpl()
{ }

k3fontObj::k3fontObj()
{
    _data = new k3fontImpl;
}

k3fontObj::~k3fontObj()
{
    if (_data) {
        delete _data;
        _data = NULL;
    }
}

k3fontImpl* k3fontObj::getImpl()
{
    return _data;
}

const k3fontImpl* k3fontObj::getImpl() const
{
    return _data;
}

K3API k3font k3gfxObj::CreateFont(k3fontDesc* desc)
{
    const uint32_t DPI = 72;
    uint32_t error;
    FT_Face face = NULL;

    if (k3fontImpl::_ft2_lib == NULL) {
        error = FT_Init_FreeType(&k3fontImpl::_ft2_lib);
        if (error) {
            k3error::Handler("Could not initialize freetype library", "k3gfxObj::CreateFont");
            return NULL;
        }
    }

    error = FT_New_Face(k3fontImpl::_ft2_lib, desc->name, 0, &face);
    if (error == FT_Err_Unknown_File_Format) {
        k3error::Handler("Unsupported font file format", "k3gfxObj::CreateFont");
        return NULL;
    } else if (error) {
        k3error::Handler("Could not load font file", "k3gfxObj::CreateFont");
        return NULL;
    }

    error = FT_Set_Char_Size(face, 0, static_cast<FT_F26Dot6>(desc->point_size * 64), DPI, DPI);
    if (error) {
        FT_Done_Face(face);
        k3error::Handler("Could not set font size", "k3gfxObj::CreateFont");
        return NULL;
    }

    FT_GlyphSlot slot = face->glyph;

    float pixel_size = desc->point_size * DPI / 72.0f;
    k3font font = new k3fontObj;
    k3fontImpl* font_impl = font->getImpl();
    font_impl->_ascender = static_cast<float>(face->ascender * pixel_size / face->units_per_EM);
    font_impl->_descender = static_cast<float>(face->descender * pixel_size / face->units_per_EM);

    uint8_t c, c2;
    int32_t row;
    uint32_t tex_width = 0, tex_height = 0;

    const uint32_t chars_per_row = 16;
    uint32_t cur_xpos = 0, next_ydelta = 0;
    uint32_t cur_char_width, cur_char_height;
    uint32_t char_scale_pix[k3fontImpl::NUM_CHARS * 4];

    // loop through all the glyphs and get the max width and height
    for (c = 0; c < k3fontImpl::NUM_CHARS; c++) {
        if ((c % chars_per_row) == chars_per_row - 1) {
            if (tex_width < cur_xpos) tex_width = cur_xpos;
            tex_height += next_ydelta;
            cur_xpos = 0;
            next_ydelta = 0;
        }

        error = FT_Load_Char(face, c, FT_LOAD_DEFAULT);
        if (error) {
            char msg[64];
            sprintf_s<64>(msg, "Could not load char \'%c\'", c);
            k3error::Handler(msg, "k3gfxObj::CreateFont");
            continue;
        }

        cur_char_width = slot->metrics.width >> 6;
        cur_char_height = slot->metrics.height >> 6;
        char_scale_pix[4 * c + 0] = cur_char_width;
        char_scale_pix[4 * c + 1] = cur_char_height;
        char_scale_pix[4 * c + 2] = cur_xpos;
        char_scale_pix[4 * c + 3] = tex_height;

        cur_xpos += cur_char_height;
        if (next_ydelta < cur_char_height) next_ydelta = cur_char_height;
    }

    // Get next power of 2
    uint32_t p;
    p = 1;
    while (p < tex_width) p = p << 1;
    tex_width = p;
    // pad texture width by 256
    tex_width += 0xFF;
    tex_width &= ~0xFF;
    p = 1;
    while (p < tex_height) p = p << 1;
    tex_height = p;

    uint32_t tex_size = tex_width * tex_height;
    k3uploadImage upload_tex = CreateUploadImage();
    upload_tex->SetDimensions(tex_width, tex_height, 1, k3fmt::R8_UNORM);
    uint8_t* image_data = static_cast<uint8_t*>(upload_tex->MapForWrite());
    uint8_t* image_row;
    FT_Vector delta;
    FT_UInt c_index, c2_index;
    uint32_t i;
    for (i = 0; i < k3fontImpl::NUM_VERSIONS; i++) {
        font_impl->_upload_cbuf[i] = CreateUploadBuffer();
    }
    k3fontCBuffer* cbuf_data = static_cast<k3fontCBuffer*>(font_impl->_upload_cbuf[0]->MapForWrite(sizeof(k3fontCBuffer)));
    memset(cbuf_data, 0, sizeof(k3fontCBuffer));
    float* char_scale = cbuf_data->char_scale;

    memset(image_data, 0, tex_size);
    for (c = 0; c < k3fontImpl::NUM_CHARS; c++) {
        error = FT_Load_Char(face, c, FT_LOAD_RENDER);
        if (error) {
            char msg[64];
            sprintf_s<64>(msg, "Could not load char \'%c\'", c);
            k3error::Handler(msg, "k3gfxObj::CreateFont");
            continue;
        }
        image_row = image_data + (char_scale_pix[4 * c + 2] + char_scale_pix[4 * c + 3] * tex_width);
        for (row = slot->bitmap.rows - 1; row >= 0; row--) {
            memcpy(image_row, slot->bitmap.buffer + (row * slot->bitmap.width), slot->bitmap.width);
            image_row += tex_width;
        }

        font_impl->_char_width[c] = static_cast<uint16_t>(slot->bitmap.width);
        font_impl->_char_height[c] = static_cast<uint16_t>(slot->bitmap.rows);

        c_index = FT_Get_Char_Index(face, c);
        for (c2 = 0; c2 < k3fontImpl::NUM_CHARS; c2++) {
            c2_index = FT_Get_Char_Index(face, c2);
            FT_Get_Kerning(face, c_index, c2_index, FT_KERNING_DEFAULT, &delta);
            font_impl->_char_offset_x[c][c2] = static_cast<int16_t>(((slot->advance.x + delta.x) >> 6) - slot->bitmap_left);
        }

        font_impl->_char_offset_y[c] = static_cast<int16_t>(font_impl->_char_height[c] - (slot->metrics.horiBearingY >> 6));
        char_scale[4 * c + 0] = char_scale_pix[4 * c + 0] / static_cast<float>(tex_width);
        char_scale[4 * c + 1] = char_scale_pix[4 * c + 1] / static_cast<float>(tex_height);
        char_scale[4 * c + 2] = char_scale_pix[4 * c + 2] / static_cast<float>(tex_width);
        char_scale[4 * c + 3] = char_scale_pix[4 * c + 3] / static_cast<float>(tex_height);
    }
    upload_tex->Unmap();
    font_impl->_upload_cbuf[0]->Unmap();
    FT_Done_Face(face);

    k3viewDesc vdesc = { 0 };
    vdesc.view_index = desc->view_index;
    k3resourceDesc rdesc;
    upload_tex->GetDesc(&rdesc);
    font_impl->_font_tex = CreateSurface(&rdesc, NULL, &vdesc, NULL);
    if (font_impl->_font_tex == NULL) {
        k3error::Handler("Could not create font texture", "k3gfxObj::CreateFont");
        return NULL;
    }

    k3bufferDesc bdesc = { 0 };
    bdesc.view_index = desc->view_index + 1;
    bdesc.size = sizeof(k3fontCBuffer);
    font_impl->_font_cbuf = CreateBuffer(&bdesc);

    k3resource tex_resource = font_impl->_font_tex->GetResource();
    k3resource cb_resource = NULL;
    desc->cmd_buf->Reset();
    desc->cmd_buf->TransitionResource(tex_resource, k3resourceState::COPY_DEST);
    desc->cmd_buf->UploadImage(upload_tex, tex_resource);
    cb_resource = font_impl->_font_cbuf->GetResource();
    desc->cmd_buf->TransitionResource(cb_resource, k3resourceState::COPY_DEST);
    desc->cmd_buf->UploadBuffer(font_impl->_upload_cbuf[0], cb_resource);
    desc->cmd_buf->TransitionResource(tex_resource, k3resourceState::SHADER_RESOURCE);
    desc->cmd_buf->Close();
    SubmitCmdBuf(desc->cmd_buf);

    k3bindingParam bind_params[2];
    bind_params[0].type = k3bindingType::VIEW_SET;
    bind_params[0].view_set_desc.type = k3shaderBindType::SRV;
    bind_params[0].view_set_desc.num_views = 1;
    bind_params[0].view_set_desc.reg = 0;
    bind_params[0].view_set_desc.space = 0;
    bind_params[1].type = k3bindingType::VIEW_SET;
    bind_params[1].view_set_desc.type = k3shaderBindType::CBV;
    bind_params[1].view_set_desc.num_views = 1;
    bind_params[1].view_set_desc.reg = 0;
    bind_params[1].view_set_desc.space = 0;
    k3samplerDesc sdesc = { 0 };
    sdesc.filter = k3texFilter::MIN_MAG_MIP_LINEAR;
    sdesc.addr_u = k3texAddr::CLAMP;
    sdesc.addr_v = k3texAddr::CLAMP;
    sdesc.addr_w = k3texAddr::CLAMP;

    k3shaderBinding font_binding = CreateShaderBinding(2, bind_params, 1, &sdesc);

    k3shader vs, ps;
    GetFontShaders(vs, ps);

    k3gfxStateDesc state_desc = { 0 };
    state_desc.shader_binding = font_binding;
    state_desc.vertex_shader = vs;
    state_desc.pixel_shader = ps;
    state_desc.sample_mask = ~0;
    state_desc.rast_state.fill_mode = k3fill::SOLID;
    state_desc.rast_state.cull_mode = k3cull::NONE;
    state_desc.rast_state.front_counter_clockwise = false;
    state_desc.blend_state.blend_op[0].blend_enable = desc->transparent;
    state_desc.blend_state.blend_op[0].src_blend = k3blend::SRC_ALPHA;
    state_desc.blend_state.blend_op[0].dst_blend = k3blend::INV_SRC_ALPHA;
    state_desc.blend_state.blend_op[0].blend_op = k3blendOp::ADD;
    state_desc.blend_state.blend_op[0].alpha_src_blend = k3blend::SRC_ALPHA;
    state_desc.blend_state.blend_op[0].alpha_dst_blend = k3blend::INV_SRC_ALPHA;
    state_desc.blend_state.blend_op[0].alpha_blend_op = k3blendOp::ADD;
    state_desc.blend_state.blend_op[0].rt_write_mask = 0xf;
    state_desc.prim_type = k3primType::TRIANGLE;
    state_desc.num_render_targets = 1;
    state_desc.rtv_format[0] = desc->format;
    state_desc.msaa_samples = 1;
    font_impl->_font_state = CreateGfxState(&state_desc);

    return font;
}

K3API void k3cmdBufObj::DrawText(const char* text, k3font font, const float fg_color[4], const float bg_color[4], int32_t x, int32_t y, k3fontAlignment alignment)
{
    k3fontImpl* font_impl = font->getImpl();
    k3fontCBufferDynamic* cbuf_data = static_cast<k3fontCBufferDynamic*>(font_impl->_upload_cbuf[font_impl->_version]->MapForWrite(sizeof(k3fontCBufferDynamic)));
    uint32_t i;
    for (i = 0; i < 4; i++) {
        cbuf_data->fg_color[i] = fg_color[i];
        cbuf_data->bg_color[i] = bg_color[i];
    }

    k3rect viewport;
    GetCurrentViewport(&viewport);
    uint32_t width_div2 = viewport.width / 2;
    uint32_t height_div2 = viewport.height / 2;

    float origin_offset_x = x / static_cast<float>(width_div2);
    float origin_offset_y = y / static_cast<float>(height_div2);

    float text_width = 0.0f, text_height = 0.0f;
    float pix_ascender = font_impl->_ascender / height_div2;
    float pix_descender = font_impl->_descender / height_div2;

    const char* c;
    uint8_t n, text_length;
    uint8_t cv, last_cv = 0;
    for (c = text, n = 0; *c != '\0' && n < 64; c++, n++) {
        cv = *c;
        cbuf_data->text[n] = cv;
        cbuf_data->xform[4 * n + 0] = static_cast<float>(font_impl->_char_width[cv]) / width_div2;
        cbuf_data->xform[4 * n + 1] = static_cast<float>(font_impl->_char_height[cv]) / height_div2;
        cbuf_data->xform[4 * n + 2] = (n == 0) ? 0.0f : (cbuf_data->xform[4 * (n - 1) + 2] + static_cast<float>(font_impl->_char_offset_x[last_cv][cv]) / width_div2);
        last_cv = cv;
    }
    text_length = n;
    if (text_length == 0) return;
    text_width = cbuf_data->xform[4 * (n - 1) + 2] + cbuf_data->xform[4 * (n - 1) + 0];
    text_height = pix_ascender - pix_descender;

    float origin_x = 0.0f, origin_y = 0.0f;
    switch (alignment) {
    case k3fontAlignment::TOP_LEFT:
        origin_x = -1.0f + origin_offset_x;
        origin_y = 1.0f - origin_offset_y - pix_ascender;
        break;
    case k3fontAlignment::TOP_CENTER:
        origin_x = 0.0f + origin_offset_x - text_width / 2.0f;
        origin_y = 1.0f - origin_offset_y - pix_ascender;
        break;
    case k3fontAlignment::TOP_RIGHT:
        origin_x = 1.0f - origin_offset_x - text_width;
        origin_y = 1.0f - origin_offset_y - pix_ascender;
        break;
    case k3fontAlignment::MID_LEFT:
        origin_x = -1.0f + origin_offset_x;
        origin_y = 0.0f - origin_offset_y - text_height / 2.0f;
        break;
    case k3fontAlignment::MID_CENTER:
        origin_x = 0.0f + origin_offset_x - text_width / 2.0f;
        origin_y = 0.0f - origin_offset_y - text_height / 2.0f;
        break;
    case k3fontAlignment::MID_RIGHT:
        origin_x = 1.0f - origin_offset_x - text_width;
        origin_y = 0.0f - origin_offset_y - text_height / 2.0f;
        break;
    case k3fontAlignment::BOTTOM_LEFT:
        origin_x = -1.0f + origin_offset_x;
        origin_y = -1.0f + origin_offset_y - pix_descender;
        break;
    case k3fontAlignment::BOTTOM_CENTER:
        origin_x = 0.0f + origin_offset_x - text_width / 2.0f;
        origin_y = -1.0f + origin_offset_y - pix_descender;
        break;
    case k3fontAlignment::BOTTOM_RIGHT:
        origin_x = 1.0f - origin_offset_x - text_width;
        origin_y = -1.0f + origin_offset_y - pix_descender;
        break;
    }

    for (c = text, n = 0; n < text_length; c++, n++) {
        cv = *c;
        cbuf_data->xform[4 * n + 2] += origin_x;
        cbuf_data->xform[4 * n + 3] = origin_y - static_cast<float>(font_impl->_char_offset_y[cv]) / height_div2;
    }
    font_impl->_upload_cbuf[font_impl->_version]->Unmap();

    k3buffer font_cbuf = font_impl->_font_cbuf;
    k3resource cb_resource = font_cbuf->GetResource();
    TransitionResource(cb_resource, k3resourceState::COPY_DEST);
    UploadBuffer(font_impl->_upload_cbuf[font_impl->_version], cb_resource);
    TransitionResource(cb_resource, k3resourceState::SHADER_BUFFER);
    SetGfxState(font_impl->_font_state);
    SetShaderView(0, font_impl->_font_tex);
    SetConstantBuffer(1, font_cbuf);
    SetDrawPrim(k3drawPrimType::TRIANGLESTRIP);
    Draw(4, 0, text_length, 0);
    font_impl->_version++;
    if (font_impl->_version >= k3fontImpl::NUM_VERSIONS) font_impl->_version = 0;
}

// ------------------------------------------------------------
// win class

k3winImpl::k3winImpl()
{
    gfx = NULL;
    _x_pos = 0;
    _y_pos = 0;
    _width = 0;
    _height = 0;
    _is_visible = false;
    _is_cursor_visible = false;
    _is_fullscreen = false;
    _vsync_interval = 0;
    _title = NULL;
    _color_fmt = k3fmt::UNKNOWN;
    _data = NULL;

    Display = NULL;
    Idle = NULL;
    Keyboard = NULL;
    MouseMove = NULL;
    MouseButton = NULL;
    MouseScroll = NULL;
    Resize = NULL;
    JoystickAdded = NULL;
    JoystickRemoved = NULL;
    JoystickMove = NULL;
    JoystickButton = NULL;
    Destroy = NULL;
}

k3winImpl::~k3winImpl()
{ }

k3winObj::~k3winObj()
{
    if (_data) {
        delete _data;
        _data = NULL;
    }
}

K3API void k3winObj::SetDataPtr(void* data)
{
    _data->_data = data;
}

K3API void k3winObj::SetDisplayFunc(k3win_display_ptr Display)
{
    _data->Display = Display;
}

K3API void k3winObj::SetIdleFunc(k3win_idle_ptr Idle)
{
    _data->Idle = Idle;
}

K3API void k3winObj::SetKeyboardFunc(k3win_keyboard_ptr Keyboard)
{
    _data->Keyboard = Keyboard;
}

K3API void k3winObj::SetMouseFunc(k3win_mouse_move_ptr MouseMove, k3win_mouse_button_ptr MouseButton, k3win_mouse_scroll_ptr MouseScroll)
{
    _data->MouseMove = MouseMove;
    _data->MouseButton = MouseButton;
    _data->MouseScroll = MouseScroll;
}

K3API void k3winObj::SetResizeFunc(k3win_resize_ptr Resize)
{
    _data->Resize = Resize;
}

K3API void k3winObj::SetJoystickFunc(k3win_joystick_added_ptr JoystickAdded, k3win_joystick_removed_ptr JoystickRemoved, k3win_joystick_move_ptr JoystickMove, k3win_joystick_button_ptr JoystickButton)
{
    _data->JoystickAdded = JoystickAdded;
    _data->JoystickRemoved = JoystickRemoved;
    _data->JoystickMove = JoystickMove;
    _data->JoystickButton = JoystickButton;
}

K3API void k3winObj::SetDestroyFunc(k3win_destroy_ptr Destroy)
{
    _data->Destroy = Destroy;
}

K3API k3gfx k3winObj::GetGfx() const
{
    return _data->gfx;
}

K3API const char* k3winObj::GetTitle() const
{
    return _data->_title;
}

K3API uint32_t k3winObj::GetWidth() const
{
    return _data->_width;
}

K3API uint32_t k3winObj::GetHeight() const
{
    return _data->_height;
}

K3API uint32_t k3winObj::GetXPosition() const
{
    return _data->_x_pos;
}

K3API uint32_t k3winObj::GetYPosition() const
{
    return _data->_y_pos;
}

K3API bool k3winObj::IsVisible() const
{
    return _data->_is_visible;
}

K3API bool k3winObj::IsCursorVisible() const
{
    return _data->_is_cursor_visible;
}

K3API bool k3winObj::IsFullscreen() const
{
    return _data->_is_fullscreen;
}

K3API uint32_t k3winObj::GetVsyncInterval() const
{
    return _data->_vsync_interval;
}
