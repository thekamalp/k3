// k3 graphics library
// windows class
// Date: 10/10/2021

#include "k3internal.h"

// ------------------------------------------------------------
// k3 bit tracker

k3bitTrackerImpl::k3bitTrackerImpl()
{
    _size = 0;
    _array = NULL;
}

k3bitTrackerImpl::~k3bitTrackerImpl()
{
    if (_array) {
        delete[] _array;
        _array = NULL;
    }
}

k3bitTrackerObj::k3bitTrackerObj()
{
    _data = new k3bitTrackerImpl;
}

k3bitTrackerObj::~k3bitTrackerObj()
{
    if (_data) {
        delete _data;
        _data = NULL;
    }
}

k3bitTrackerImpl* k3bitTrackerObj::getImpl()
{
    return _data;
}

const k3bitTrackerImpl* k3bitTrackerObj::getImpl() const
{
    return _data;
}

K3API k3bitTracker k3bitTrackerObj::Create(uint32_t size)
{
    k3bitTracker bt = new k3bitTrackerObj;
    bt->Resize(size);
    return bt;
}

K3API void k3bitTrackerObj::Resize(uint32_t size)
{
    if (size > _data->_size) {
        if(_data->_array) delete[] _data->_array;
        _data->_array = new uint64_t[(size + 63) / 64];
    }
    _data->_size = size;
    SetAll(false);
}

K3API void k3bitTrackerObj::SetAll(bool value)
{
    uint32_t elem_size = (_data->_size + 63) / 64;
    uint32_t i;
    for (i = 0; i < elem_size; i++) {
        _data->_array[i] = (value) ? 0xffffffffffffffffULL : 0x0;
    }
}

K3API void k3bitTrackerObj::SetBit(uint32_t bit, bool value)
{
    if (bit >= _data->_size) {
        k3error::Handler("Illegal bit", "k3bitTrackerObj::SetBit");
        return;
    }
    uint32_t elem_index = bit / 64;
    uint64_t flag = 1 << (bit & 63);
    if (value) {
        _data->_array[elem_index] |= flag;
    } else {
        _data->_array[elem_index] &= ~flag;
    }
}

K3API bool k3bitTrackerObj::GetBit(uint32_t bit)
{
    if (bit >= _data->_size) {
        k3error::Handler("Illegal bit", "k3bitTrackerObj::GetBit");
        return false;
    }
    uint32_t elem_index = bit / 64;
    uint64_t flag = 1 << (bit & 63);
    return (_data->_array[elem_index] & flag) ? true : false;
}

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

        cur_xpos += cur_char_width;
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
    vdesc.view_index = desc->view_index++;
    k3resourceDesc rdesc;
    upload_tex->GetDesc(&rdesc);
    font_impl->_font_tex = CreateSurface(&rdesc, NULL, &vdesc, NULL);
    if (font_impl->_font_tex == NULL) {
        k3error::Handler("Could not create font texture", "k3gfxObj::CreateFont");
        return NULL;
    }

    k3bufferDesc bdesc = { 0 };
    bdesc.view_index = desc->view_index++;
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
    sdesc.filter = k3texFilter::MIN_MAG_MIP_POINT;
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
// BVH functions

bool k3bvh_CheckCollision(k3AABB* s1, k3AABB* s2)
{
    bool x_collision = (s1->max[0] >= s2->min[0]) && (s2->max[0] >= s1->min[0]);
    bool y_collision = (s1->max[1] >= s2->min[1]) && (s2->max[1] >= s1->min[1]);
    bool z_collision = (s1->max[2] >= s2->min[2]) && (s2->max[2] >= s1->min[2]);
    return x_collision && y_collision && z_collision;
}

bool k3bvh_CheckDirectedCollision(k3AABB* s1, k3AABB* s2, float* vec, k3AABB* slip_bounds)
{
    uint32_t axis;
    bool collision = true;
    float s1_pos;
    float mod_vec[3] = {};
    for (axis = 0; axis < 3; axis++) {
        if (s1->min[axis] < s2->min[axis]) {
            s1_pos = s1->max[axis] + vec[axis];
            if (s1_pos >= s2->min[axis]) {
                mod_vec[axis] = (s1_pos - s2->min[axis]);
            } else {
                collision = false;
            }
        } else {
            s1_pos = s1->min[axis] + vec[axis];
            if (s1_pos < s2->max[axis]) {
                mod_vec[axis] = (s1_pos - s2->max[axis]);
            } else {
                collision = false;
            }
        }
    }
    if (collision) {
        bool slip_done[3] = { false, false, false };
        if (slip_bounds) {
            // Find an axis within the slip bounds
            for (axis = 0; axis < 3; axis++) {
                if (mod_vec[axis] > 0.0f && -mod_vec[axis] > slip_bounds->min[axis]) {
                    slip_done[axis] = true;
                }
                if (mod_vec[axis] < 0.0f && -mod_vec[axis] < slip_bounds->max[axis]) {
                    slip_done[axis] = true;
                }
            }
        }
        // Use an axis within the slip bounds, and if none exists, find the axis with 
        // the smallest absolute delta, and modify that axis only
        if (slip_done[0]) {
            vec[0] -= mod_vec[0];
        } else if (slip_done[1]) {
            vec[1] -= mod_vec[1];
        } else if (slip_done[2]) {
            vec[2] -= mod_vec[2];
        } else if (fabsf(mod_vec[0]) < fabsf(mod_vec[1])) { 
            if (fabsf(mod_vec[0]) < fabsf(mod_vec[2])) {
                vec[0] -= mod_vec[0];
            } else {
                vec[2] -= mod_vec[2];
            }
        } else {
            if (fabsf(mod_vec[1]) < fabsf(mod_vec[2])) {
                vec[1] -= mod_vec[1];
            } else {
                vec[2] -= mod_vec[2];
            }
        }
    }
    return collision;
}

void k3bvh_ScaleOffsetAABB(k3AABB* d, const float* scale, const float* offset)
{
    float center;
    float delta;
    uint32_t axis;
    for (axis = 0; axis < 3; axis++) {
        center= (d->max[axis] + d->min[axis]) * 0.5f;
        delta = (d->max[axis] - d->min[axis]);
        center += offset[axis] * delta;
        delta *= scale[axis] * 0.5f;
        d->min[axis] = center - delta;
        d->max[axis] = center + delta;
    }
}


// ------------------------------------------------------------
// mesh class

k3meshImpl::k3meshImpl()
{
    _num_meshes = 0;
    _num_model_custom_props = 0;
    _num_models = 0;
    _num_tris = 0;
    _num_textures = 0;
    _num_cameras = 0;
    _num_lights = 0;
    _num_empties = 0;
    _num_bones = 0;
    _num_anims = 0;
    _geom_data = NULL;
    _mesh_start = NULL;
    _model = NULL;
    _model_custom_props = NULL;
    _textures = NULL;
    _cameras = NULL;
    _lights = NULL;
    _empties = NULL;
    _bones = NULL;
    _anim = NULL;
    _ib = NULL;
    _vb = NULL;
    _ab = NULL;
    _sb = NULL;
    _lb = NULL;
}

k3meshImpl::~k3meshImpl()
{
    if (_geom_data) {
        delete[] _geom_data;
        _geom_data = NULL;
    }
    if (_mesh_start) {
        delete[] _mesh_start;
        _mesh_start = NULL;
    }
    if (_model) {
        delete[] _model;
        _model = NULL;
    }
    if (_model_custom_props) {
        delete[] _model_custom_props;
        _model_custom_props = NULL;
    }
    if (_textures) {
        delete[] _textures;
        _textures = NULL;
    }
    if (_cameras) {
        delete[] _cameras;
        _cameras = NULL;
    }
    if (_lights) {
        delete[] _lights;
        _lights = NULL;
    }
    if (_empties) {
        delete[] _empties;
        _empties = NULL;
    }
    if (_bones) {
        delete[] _bones;
        _bones = NULL;
    }
    if (_anim) {
        uint32_t i;
        for (i = 0; i < _num_anims; i++) {
            if (_anim[i].bone_data) delete[] _anim[i].bone_data;
        }
        delete[] _anim;
        _anim = NULL;
    }
}

k3meshObj::k3meshObj()
{
    _data = new k3meshImpl;
}

k3meshObj::~k3meshObj()
{
    if (_data) {
        delete _data;
        _data = NULL;
    }
}

k3meshImpl* k3meshObj::getImpl()
{
    return _data;
}

const k3meshImpl* k3meshObj::getImpl() const
{
    return _data;
}

K3API uint32_t k3meshObj::getNumObjects()
{
    return _data->_num_models;
}

K3API uint32_t k3meshObj::getNumTextures()
{
    return _data->_num_textures;
}

K3API uint32_t k3meshObj::getNumMeshes()
{
    return _data->_num_meshes;
}

K3API uint32_t k3meshObj::getNumEmpties()
{
    return _data->_num_empties;
}

K3API uint32_t k3meshObj::getNumLights()
{
    return _data->_num_lights;
}

K3API uint32_t k3meshObj::getNumCameras()
{
    return _data->_num_cameras;
}

K3API uint32_t k3meshObj::getNumBones()
{
    return _data->_num_bones;
}

K3API uint32_t k3meshObj::getNumAnims()
{
    return _data->_num_anims;
}

K3API k3surf k3meshObj::getTexture(uint32_t tex)
{
    if (tex < _data->_num_textures) {
        return _data->_textures[tex];
    } else {
        return NULL;
    }
}

K3API uint32_t k3meshObj::getMeshStartPrim(uint32_t mesh)
{
    if (mesh < _data->_num_meshes) {
        return _data->_mesh_start[mesh];
    } else {
        return 0;
    }
}

K3API uint32_t k3meshObj::getMeshNumPrims(uint32_t mesh)
{
    if (mesh < _data->_num_meshes) {
        uint32_t num_prims = (mesh == _data->_num_meshes - 1) ? _data->_num_tris : _data->_mesh_start[mesh + 1];
        num_prims -= _data->_mesh_start[mesh];
        return num_prims;
    } else {
        return 0;
    }
}

K3API uint32_t k3meshObj::getMeshIndex(uint32_t obj)
{
    if (obj < _data->_num_models) {
        return _data->_model[obj].mesh_index;
    } else {
        return 0;
    }
}

K3API uint32_t k3meshObj::getStartPrim(uint32_t obj)
{
    if (obj < _data->_num_models) {
        return _data->_model[obj].prim_start;
    } else {
        return 0;
    }
}

K3API uint32_t k3meshObj::getNumPrims(uint32_t obj)
{
    if (obj < _data->_num_models) {
        return _data->_model[obj].num_prims;
    } else {
        return 0;
    }
}

K3API float* k3meshObj::getTransform(uint32_t obj)
{
    if (obj < _data->_num_models) {
        return _data->_model[obj].world_xform;
    } else {
        return NULL;
    }
}

K3API float* k3meshObj::getDiffuseColor(uint32_t obj)
{
    if (obj < _data->_num_models) {
        return _data->_model[obj].diffuse_color;
    } else {
        return NULL;
    }
}

K3API uint32_t k3meshObj::getDiffuseMapIndex(uint32_t obj)
{
    if (obj < _data->_num_models) {
        return _data->_model[obj].diffuse_map_index;
    } else {
        return NULL;
    }
}

K3API uint32_t k3meshObj::getNormalMapIndex(uint32_t obj)
{
    if (obj < _data->_num_models) {
        return _data->_model[obj].normal_map_index;
    } else {
        return NULL;
    }
}
K3API float k3meshObj::getVisibility(uint32_t obj)
{
    if (obj < _data->_num_models) {
        return _data->_model[obj].visibility;
    } else {
        return 0.0f;
    }
}

K3API k3flint32 k3meshObj::getCustomProp(uint32_t obj, uint32_t custom_prop_index)
{
    if (obj < _data->_num_models && custom_prop_index < _data->_num_model_custom_props) {
        return _data->_model_custom_props[obj * _data->_num_model_custom_props + custom_prop_index];
    } else {
        k3flint32 n;
        n.i = 0;
        return n;
    }
}

K3API uint32_t k3meshObj::getParent(uint32_t obj)
{
    if (obj < _data->_num_models) {
        return _data->_model[obj].parent;
    } else {
        return ~0x0;
    }
}

K3API uint32_t k3meshObj::getRootParent(uint32_t obj)
{
    if (obj < _data->_num_models) {
        uint32_t parent = _data->_model[obj].parent;
        if (parent < _data->_num_models) {
            return getRootParent(parent);
        } else {
            return obj;
        }
    } else {
        return ~0x0;
    }
}

K3API float* k3meshObj::getEmptyTransform(uint32_t empty)
{
    if (empty < _data->_num_empties) {
        return _data->_empties[empty].world_xform;
    } else {
        return NULL;
    }
}


K3API k3buffer k3meshObj::getIndexBuffer()
{
    return _data->_ib;
}

K3API k3buffer k3meshObj::getVertexBuffer()
{
    return _data->_vb;
}

K3API k3buffer k3meshObj::getAttribBuffer()
{
    return _data->_ab;
}

K3API k3buffer k3meshObj::getLightBuffer()
{
    return _data->_lb;
}

K3API k3buffer k3meshObj::getSkinBuffer()
{
    return _data->_sb;
}

K3API k3projType k3meshObj::getCameraProjectionType(uint32_t camera)
{
    if (camera >= _data->_num_cameras) return k3projType::PERSPECTIVE;
    return _data->_cameras[camera].proj_type;
}

K3API float* k3meshObj::getCameraProjection(float* d, uint32_t camera, bool left_handed, bool dx_style, bool reverse_z)
{
    if (camera >= _data->_num_cameras) return d;
    k3camera* cam = _data->_cameras + camera;
    if (cam->proj_type == k3projType::ORTHOGRAPIC) {
        float aspect = cam->res_y / (float)cam->res_x;
        float ortho_scale = cam->ortho_scale;
        float l = -ortho_scale * 0.5f;
        float r = -l;
        float b = -aspect * ortho_scale * 0.5f;
        float t = -b;
        k3m4_SetOrthoOffCenter(d, l, r, b, t, cam->near_plane, cam->far_plane, left_handed, dx_style, reverse_z);
    } else {
        float aspect = cam->res_x / (float)cam->res_y;
        k3m4_SetPerspectiveFov(d, deg2rad(cam->fovy), aspect, cam->near_plane, cam->far_plane, left_handed, dx_style, reverse_z);
    }
    return d;
}

K3API float* k3meshObj::getCameraView(float* d, uint32_t camera, bool left_handed)
{
    if (camera > _data->_num_cameras) return d;
    k3camera* cam = _data->_cameras + camera;
    k3m4_SetLookAt(d, cam->translation, cam->look_at, cam->up, left_handed);
    return d;
}

K3API float* k3meshObj::getCameraPosition(uint32_t camera)
{
    if (camera > _data->_num_cameras) return NULL;
    return _data->_cameras[camera].translation;
}

K3API float* k3meshObj::getCameraLookAt(uint32_t camera)
{
    if (camera > _data->_num_cameras) return NULL;
    return _data->_cameras[camera].look_at;
}

K3API float* k3meshObj::getCameraUp(uint32_t camera)
{
    if (camera > _data->_num_cameras) return NULL;
    return _data->_cameras[camera].up;
}

K3API void k3meshObj::getCameraResolution(uint32_t camera, uint32_t* width, uint32_t* height)
{
    if (camera > _data->_num_cameras) return;
    k3camera* cam = _data->_cameras + camera;
    *width = cam->res_x;
    *height = cam->res_y;
}

K3API float k3meshObj::getCameraNearPlane(uint32_t camera)
{
    if (camera > _data->_num_cameras) return 0.0f;
    return _data->_cameras[camera].near_plane;
}

K3API float k3meshObj::getCameraFarPlane(uint32_t camera)
{
    if (camera > _data->_num_cameras) return 0.0f;
    return _data->_cameras[camera].far_plane;
}

K3API float k3meshObj::getCameraFovX(uint32_t camera)
{
    if (camera > _data->_num_cameras) return 0.0f;
    float aspect = _data->_cameras[camera].res_x / (float)_data->_cameras[camera].res_y;
    return aspect * _data->_cameras[camera].fovy;
}

K3API float k3meshObj::getCameraFovY(uint32_t camera)
{
    if (camera > _data->_num_cameras) return 0.0f;
    return _data->_cameras[camera].fovy;
}

K3API float k3meshObj::getCameraOrthoScaleX(uint32_t camera)
{
    if (camera > _data->_num_cameras) return 0.0f;
    return _data->_cameras[camera].ortho_scale;
}

K3API float k3meshObj::getCameraOrthoScaleY(uint32_t camera)
{
    if (camera > _data->_num_cameras) return 0.0f;
    float aspect = _data->_cameras[camera].res_y / (float)_data->_cameras[camera].res_x;
    return aspect * _data->_cameras[camera].ortho_scale;
}

K3API void k3meshObj::setCameraResolution(uint32_t camera, uint32_t width, uint32_t height)
{
    if (camera > _data->_num_cameras) return;
    k3camera* cam = _data->_cameras + camera;
    cam->res_x = width;
    cam->res_y = height;
}

K3API void k3meshObj::setCameraNearPlane(uint32_t camera, float near)
{
    if (camera > _data->_num_cameras) return;
    k3camera* cam = _data->_cameras + camera;
    cam->near_plane = near;
}

K3API void k3meshObj::setCameraFarPlane(uint32_t camera, float far)
{
    if (camera > _data->_num_cameras) return;
    k3camera* cam = _data->_cameras + camera;
    cam->far_plane = far;
}

K3API void k3meshObj::genBoneMatrices(float* mat, bool gen_inv)
{
    uint32_t bone_id, parent_bone_id;
    uint32_t mat_stride = 16 + ((gen_inv) ? 16 : 0);
    float* cur_mat = mat;
    float* parent_mat = NULL;
    float xlat_mat[16];
    float rot_xlat_mat[16];
    for (bone_id = 0; bone_id < _data->_num_bones; bone_id++, cur_mat += mat_stride) {
        k3m4_QuatToMat(rot_xlat_mat, _data->_bones[bone_id].rot_quat);
        k3m4_SetIdentity(xlat_mat);
        xlat_mat[3] = _data->_bones[bone_id].position[0];
        xlat_mat[7] = _data->_bones[bone_id].position[1];
        xlat_mat[11] = _data->_bones[bone_id].position[2];
        k3m4_Mul(rot_xlat_mat, xlat_mat, rot_xlat_mat);
        k3m4_SetIdentity(xlat_mat);
        xlat_mat[0] = _data->_bones[bone_id].scaling[0];
        xlat_mat[5] = _data->_bones[bone_id].scaling[1];
        xlat_mat[10] = _data->_bones[bone_id].scaling[2];
        k3m4_Mul(cur_mat, rot_xlat_mat, xlat_mat);

        parent_bone_id = _data->_bones[bone_id].parent;
        if (parent_bone_id < _data->_num_bones) {
            parent_mat = mat + mat_stride * parent_bone_id;
            k3m4_Mul(cur_mat, parent_mat, cur_mat);
        }
        if (gen_inv) {
            // inverse of transform without scaling factored
            if (parent_bone_id < _data->_num_bones) {
                parent_mat = mat + mat_stride * parent_bone_id + 16;
                k3m4_Mul(cur_mat + 16, parent_mat, rot_xlat_mat);
            } else {
                memcpy(cur_mat + 16, rot_xlat_mat, 16 * sizeof(float));
            }

        }
 
    }

    cur_mat = mat;
    for (bone_id = 0; bone_id < _data->_num_bones; bone_id++, cur_mat += mat_stride) {
        k3m4_Mul(cur_mat, cur_mat, _data->_bones[bone_id].inv_bind_pose);
        if (gen_inv) {
            k3m4_Mul(cur_mat + 16, cur_mat + 16, _data->_bones[bone_id].inv_bind_pose);
            k3m4_Inverse(cur_mat + 16);
        }
    }
}

K3API uint32_t k3meshObj::findModel(const char* name)
{
    uint32_t i;
    for (i = 0; i < _data->_num_models; i++) {
        if (!strncmp(name, _data->_model[i].name, K3_FBX_MAX_NAME_LENGTH)) return i;
    }
    return ~0x0;
}

K3API uint32_t k3meshObj::findEmpty(const char* name)
{
    uint32_t i;
    for (i = 0; i < _data->_num_empties; i++) {
        if (!strncmp(name, _data->_empties[i].name, K3_FBX_MAX_NAME_LENGTH)) return i;
    }
    return ~0x0;
}

K3API uint32_t k3meshObj::findLight(const char* name)
{
    uint32_t i;
    for (i = 0; i < _data->_num_lights; i++) {
        if (!strncmp(name, _data->_lights[i].name, K3_FBX_MAX_NAME_LENGTH)) return i;
    }
    return ~0x0;
}

K3API uint32_t k3meshObj::findCamera(const char* name)
{
    uint32_t i;
    for (i = 0; i < _data->_num_cameras; i++) {
        if (!strncmp(name, _data->_cameras[i].name, K3_FBX_MAX_NAME_LENGTH)) return i;
    }
    return ~0x0;
}

K3API uint32_t k3meshObj::findBone(const char* name)
{
    uint32_t i;
    for (i = 0; i < _data->_num_bones; i++) {
        if (!strncmp(name, _data->_bones[i].name, K3_FBX_MAX_NAME_LENGTH)) return i;
    }
    return ~0x0;
}

K3API uint32_t k3meshObj::findAnim(const char* name)
{
    uint32_t i;
    for (i = 0; i < _data->_num_anims; i++) {
        if (!strncmp(name, _data->_anim[i].name, K3_FBX_MAX_NAME_LENGTH)) return i;
    }
    return ~0x0;
}

K3API const char* k3meshObj::getModelName(uint32_t i)
{
    if (i < _data->_num_models) {
        return _data->_model[i].name;
    }
    k3error::Handler("Invalid model", "k3meshObj::getModelName");
    return NULL;
}

K3API const char* k3meshObj::getEmptyName(uint32_t i)
{
    if (i < _data->_num_empties) {
        return _data->_empties[i].name;
    }
    k3error::Handler("Invalid empty", "k3meshObj::getEmptyName");
    return NULL;
}

K3API const char* k3meshObj::getLightName(uint32_t i)
{
    if (i < _data->_num_lights) {
        return _data->_lights[i].name;
    }
    k3error::Handler("Invalid light", "k3meshObj::getLightName");
    return NULL;
}

K3API const char* k3meshObj::getCameraName(uint32_t i)
{
    if (i < _data->_num_cameras) {
        return _data->_cameras[i].name;
    }
    k3error::Handler("Invalid camera", "k3meshObj::getCameraName");
    return NULL;
}

K3API const char* k3meshObj::getBoneName(uint32_t i)
{
    if (i < _data->_num_bones) {
        return _data->_bones[i].name;
    }
    k3error::Handler("Invalid bone", "k3meshObj::getBoneName");
    return NULL;
}

K3API const char* k3meshObj::getAnimName(uint32_t a)
{
    if (a < _data->_num_anims) {
        return _data->_anim[a].name;
    }
    k3error::Handler("Invalid animation value", "k3meshObj::getAnimName");
    return NULL;
}

K3API uint32_t k3meshObj::getAnimLength(uint32_t a)
{
    if (a < _data->_num_anims) {
        uint32_t num_frame_intervals = _data->_anim[a].num_keyframes - 1;
        uint32_t delta_msec = _data->_anim[a].keyframe_delta_msec;
        uint32_t total_anim_time_msec = num_frame_intervals * delta_msec;
        return total_anim_time_msec;
    }
    k3error::Handler("Invalid animation value", "k3meshObj::getAnimLength");
    return 0;
}


K3API void k3meshObj::setAnimation(uint32_t anim_index, uint32_t time_msec, uint32_t flags)
{
    if (anim_index >= _data->_num_anims) {
        k3error::Handler("Invalid animation index", "k3meshObj::setAnimation");
        return;
    }
    uint32_t num_frame_intervals = _data->_anim[anim_index].num_keyframes - 1;
    uint32_t delta_msec = _data->_anim[anim_index].keyframe_delta_msec;
    uint32_t total_anim_time_msec = num_frame_intervals * delta_msec;
    uint32_t norm_time_msec = (num_frame_intervals) ? time_msec % total_anim_time_msec : 0;
    uint32_t anim_frame = (num_frame_intervals) ? norm_time_msec / delta_msec : 0;
    float anim_frame_frac = (num_frame_intervals) ? ((norm_time_msec % delta_msec) / (float)delta_msec) : 0.0f;
    uint32_t bone_id;
    float* dest_pos_ptr;
    float* dest_scale_ptr;
    float* dest_quat_ptr;
    uint32_t src0_index = anim_frame * _data->_num_bones;
    uint32_t src1_index = src0_index + ((num_frame_intervals) ? _data->_num_bones : 0);
    float temp_vec[4];
    float temp_vec2[4];
    const k3boneData* bone_data = _data->_anim[anim_index].bone_data;
    const uint32_t* bone_flag = _data->_anim[anim_index].bone_flag;
    bool force_anim = (flags & ANIM_FLAG_INCREMENTAL) ? false : true;
    bool overwrite_morphed = (~flags & ANIM_FLAG_MORPHED) ? false : true;

    for (bone_id = 0; bone_id < _data->_num_bones; bone_id++) {
        dest_pos_ptr = _data->_bones[bone_id].position;
        dest_scale_ptr = _data->_bones[bone_id].scaling;
        dest_quat_ptr = _data->_bones[bone_id].rot_quat;

        if (force_anim || ((bone_flag[bone_id] & K3_BONE_FLAG_MORPH) && overwrite_morphed)) {
            k3sv3_Mul(dest_pos_ptr, 1.0f - anim_frame_frac, bone_data[src0_index].position);
            k3sv3_Mul(temp_vec, anim_frame_frac, bone_data[src1_index].position);
            k3v3_Add(dest_pos_ptr, dest_pos_ptr, temp_vec);

            k3sv3_Mul(dest_scale_ptr, 1.0f - anim_frame_frac, bone_data[src0_index].scaling);
            k3sv3_Mul(temp_vec, anim_frame_frac, bone_data[src1_index].scaling);
            k3v3_Add(dest_scale_ptr, dest_scale_ptr, temp_vec);

            k3sv4_Mul(dest_quat_ptr, 1.0f - anim_frame_frac, bone_data[src0_index].rot_quat);
            k3sv4_Mul(temp_vec, anim_frame_frac, bone_data[src1_index].rot_quat);
            k3v4_Add(dest_quat_ptr, dest_quat_ptr, temp_vec);
            k3v4_Normalize(dest_quat_ptr);
        } else if (bone_flag[bone_id] & K3_BONE_FLAG_MORPH) {
            //k3sv3_Mul(temp_vec2, 1.0f - anim_frame_frac, bone_data[src0_index].position);
            //k3sv3_Mul(temp_vec, anim_frame_frac, bone_data[src1_index].position);
            //k3v3_Add(dest_pos_ptr, dest_pos_ptr, temp_vec);
            //k3v3_Add(dest_pos_ptr, dest_pos_ptr, temp_vec2);

            k3sv3_Mul(dest_pos_ptr, 1.0f - anim_frame_frac, bone_data[src0_index].position);
            k3sv3_Mul(temp_vec, anim_frame_frac, bone_data[src1_index].position);
            k3v3_Add(dest_pos_ptr, dest_pos_ptr, temp_vec);

            k3sv3_Mul(temp_vec2, 1.0f - anim_frame_frac, bone_data[src0_index].scaling);
            k3sv3_Mul(temp_vec, anim_frame_frac, bone_data[src1_index].scaling);
            k3v3_Add(temp_vec, temp_vec2, temp_vec);
            k3v3_Mul(dest_scale_ptr, dest_scale_ptr, temp_vec);

            k3sv4_Mul(temp_vec2, 1.0f - anim_frame_frac, bone_data[src0_index].rot_quat);
            k3sv4_Mul(temp_vec, anim_frame_frac, bone_data[src1_index].rot_quat);
            k3v4_Add(temp_vec, temp_vec2, temp_vec);
            k3v4_Normalize(temp_vec);
            k3v4_QuatMul(dest_quat_ptr, dest_quat_ptr, temp_vec);
            k3v4_Normalize(dest_quat_ptr);
        }
 
        src0_index++;
        src1_index++;
    }
}

K3API void k3meshObj::getAABB(k3AABB* aabb, uint32_t model, k3bitTracker bone_exclude_mask)
{
    uint32_t m, m_start, m_end;
    uint32_t v_start, v_end;
    float model_xform[16];
    if (model >= _data->_num_models) {
        // Get AABB of all meshes
        m_start = 0;
        m_end = _data->_num_models;
    } else {
        m_start = model;
        m_end = model + 1;

    }


    aabb->min[0] = INFINITY;
    aabb->min[1] = INFINITY;
    aabb->min[2] = INFINITY;
    aabb->max[0] = -INFINITY;
    aabb->max[1] = -INFINITY;
    aabb->max[2] = -INFINITY;

    uint32_t v, b;
    float* bone_mat = new float[16 * _data->_num_bones];
    genBoneMatrices(bone_mat, false);

    float xform_vert[4];
    float temp_vec1[4];
    float temp_vec2[4];
    bool include_bone = true;
    for (m = m_start; m < m_end; m++) {
        uint32_t mesh = _data->_model[m].mesh_index;
        memcpy(model_xform, _data->_model[m].world_xform, 16 * sizeof(float));

        v_start = _data->_mesh_start[mesh];
        v_end = (mesh == _data->_num_meshes - 1) ? _data->_num_tris : _data->_mesh_start[mesh + 1];
        v_start *= 3;
        v_end *= 3;

        const float* verts = _data->_geom_data + 3 * v_start;
        const float* attrs = _data->_geom_data + 3 * _data->_num_verts + 8 * v_start;
        const float* skin_f = _data->_geom_data + 11 * _data->_num_verts + 8 * v_start;
        const uint32_t* skin_i = (const uint32_t*)(skin_f);
        skin_f += 4;

        if (_data->_ib != NULL) {
            uint32_t i, i_start = v_start, i_end = v_end;
            uint32_t vert_size = (_data->_num_bones > 0) ? 19 : 11;
            uint32_t* indices = (uint32_t*)(_data->_geom_data + vert_size * _data->_num_verts);
            v_start = _data->_num_verts;
            v_end = 0;
            for (i = i_start; i < i_end; i++) {
                v_start = (indices[i] < v_start) ? indices[i] : v_start;
                v_end = (indices[i] > v_end) ? indices[i] : v_end;
            }
        }
        for (v = v_start; v < v_end; v++) {
            xform_vert[3] = 1.0f;
            if (_data->_num_bones > 0 && skin_f[0] >= 0.1f) {
                xform_vert[0] = 0.0f;
                xform_vert[1] = 0.0f;
                xform_vert[2] = 0.0f;
                temp_vec1[0] = verts[0];
                temp_vec1[1] = verts[1];
                temp_vec1[2] = verts[2];
                temp_vec1[3] = 1.0f;
                include_bone = true;
                for (b = 0; b < 4; b++) {
                    //if (bone >= _data->_num_bones || skin_i[b] == bone) include_bone = true;
                    if (bone_exclude_mask != NULL && bone_exclude_mask->GetBit(skin_i[b])) include_bone = false;
                    // Transform vertex with this bone matrix
                    k3mv4_Mul(temp_vec2, bone_mat + 16 * skin_i[b], temp_vec1);
                    // scale by weight
                    k3sv3_Mul(temp_vec2, skin_f[b], temp_vec2);
                    // Add in the weighted position to transformed vertex
                    k3v3_Add(xform_vert, xform_vert, temp_vec2);
                }
            } else {
                xform_vert[0] = verts[0];
                xform_vert[1] = verts[2];
                xform_vert[2] = verts[2];
            }
            k3mv4_Mul(xform_vert, model_xform, xform_vert);

            if (include_bone) {
                aabb->min[0] = (xform_vert[0] < aabb->min[0]) ? xform_vert[0] : aabb->min[0];
                aabb->min[1] = (xform_vert[1] < aabb->min[1]) ? xform_vert[1] : aabb->min[1];
                aabb->min[2] = (xform_vert[2] < aabb->min[2]) ? xform_vert[2] : aabb->min[2];
                aabb->max[0] = (xform_vert[0] > aabb->max[0]) ? xform_vert[0] : aabb->max[0];
                aabb->max[1] = (xform_vert[1] > aabb->max[1]) ? xform_vert[1] : aabb->max[1];
                aabb->max[2] = (xform_vert[2] > aabb->max[2]) ? xform_vert[2] : aabb->max[2];
            }

            verts += 3;
            attrs += 8;
            skin_f += 8;
            skin_i += 8;
        }
    }

    delete[] bone_mat;
}

K3API void k3meshObj::createMeshPartitions(k3meshPartions* p)
{
    if (p == NULL) {
        k3error::Handler("Bad input", "k3meshObj::createPartionList");
        return;
    }
    if (p->x_part_size == 0.0f && p->y_part_size == 0.0f && p->z_part_size == 0.0f) {
        // Generate AABB for entire mesh
        k3AABB overall_aabb;
        getAABB(&overall_aabb, ~0x0, NULL);
        p->x_start = overall_aabb.min[0];
        p->y_start = overall_aabb.min[1];
        p->z_start = overall_aabb.min[2];
        p->x_part_size = (overall_aabb.max[0] - overall_aabb.min[0]) / (float)p->x_parts;
        p->y_part_size = (overall_aabb.max[1] - overall_aabb.min[1]) / (float)p->y_parts;
        p->z_part_size = (overall_aabb.max[2] - overall_aabb.min[2]) / (float)p->z_parts;
    }

    uint32_t o, p_index = 0;
    float x, y, z;
    float x_end = p->x_start + (p->x_part_size * p->x_parts);
    float y_end = p->y_start + (p->y_part_size * p->y_parts);
    float z_end = p->z_start + (p->z_part_size * p->z_parts);
    k3AABB obj_aabb, part_aabb;
    for (o = 0; o < _data->_num_models; o++) {
        getAABB(&obj_aabb, o, NULL);
        p_index = 0;
        for (z = p->z_start; z < z_end; z += p->z_part_size) {
            part_aabb.min[2] = z;
            part_aabb.max[2] = z + p->z_part_size;
            for (y = p->y_start; y < y_end; y += p->y_part_size) {
                part_aabb.min[1] = y;
                part_aabb.max[1] = y + p->y_part_size;
                for (x = p->x_start; x < x_end; x += p->x_part_size) {
                    part_aabb.min[0] = x;
                    part_aabb.max[0] = x + p->x_part_size;
                    // Check if obj collides partition
                    if (k3bvh_CheckCollision(&obj_aabb, &part_aabb)) {
                        p->llists[p_index]->AddTail(o);
                    }
                    p_index++;
                }
            }
        }
    }
}

K3API void k3meshObj::genBoneHierarchyMask(k3bitTracker b, uint32_t bone_id)
{
    k3bitTracker bone_hier = k3bitTrackerObj::Create(_data->_num_bones);
    bone_hier->SetAll(false);
    bone_hier->SetBit(bone_id, true);
    b->SetBit(bone_id, true);

    uint32_t i, parent;
    for (i = bone_id + 1; i < _data->_num_bones; i++) {
        parent = _data->_bones[i].parent;
        if (bone_hier->GetBit(parent)) {
            bone_hier->SetBit(i, true);
            b->SetBit(i, true);
        }
    }
}


#define K3_FBX_DEBUG 0

enum class k3fbxNodeType {
    UNKNOWN,
    OBJECTS,
    GEOMETRY,
    VERTICES,
    POLYGON_VERT_INDEX,
    LAYER_ELEMENT_NORMAL,
    LAYER_ELEMENT_TANGENT,
    LAYER_ELEMENT_UV,
    LAYER_ELEMENT_MATERIAL,
    MAPPING_TYPE,
    REFERENCE_TYPE,
    NORMALS,
    NORMAL_INDEX,
    TANGENTS,
    TANGENT_INDEX,
    UV,
    UV_INDEX,
    MATERIAL_IDS,
    //LAYER,
    //LAYER_ELEMENT,
    MODEL,
    MATERIAL,
    PROPRTIES70,
    PROP,
    TEXTURE,
    RELATIVE_FILE_NAME,
    CONNECTIONS,
    CONNECT,
    VIDEO,
    CONTENT,
    NODE_ATTRIBUTE,
    POSITION,
    UP,
    LOOK_AT,
    DEFORMER,
    INDEXES,
    WEIGHTS,
    TRANSFORM,
    TRANSFORM_LINK,
    TRANSFORM_ASSOCIATE_MODEL,
    POSE,
    NB_POSE_NODES,
    POSE_NODE,
    NODE,
    MATRIX,
    ANIMATION_STACK,
    ANIMATION_LAYER,
    ANIMATION_CURVE_NODE,
    ANIMATION_CURVE,
    KEY_TIME,
    KEY_VALUE_FLOAT
};

enum class k3fbxObjType {
    NONE,
    MESH,
    LIGHT,
    CAMERA,
    LIMB_NODE
};

enum class k3fbxProperty {
    NONE,
    LOCAL_TRANSLATION,
    LOCAL_ROTATION,
    LOCAL_SCALING,
    DIFFUSE_COLOR,
    POSITION,
    UP_VECTOR,
    INTEREST_POSITION,
    ASPECT_WIDTH,
    ASPECT_HEIGHT,
    FOVX,
    ORTHO_SCALE,
    PROJ_TYPE,
    NEAR_PLANE,
    FAR_PLANE,
    LIGHT_TYPE,
    COLOR,
    INTENSITY,
    DECAY_TYPE,
    DECAY_START,
    CAST_SHADOWS,
    D_X,
    D_Y,
    D_Z,
    VISIBILITY,
    CUSTOM_PROP
};

enum class k3fbxMapping {
    None,
    ByPoly,
    ByPolyVert,
    ByVert,
    ByEdge,
    AllSame
};

enum class k3fbxReference {
    None,
    Direct,
    ByIndex
};

enum class k3fbxDeformer {
    None,
    Skin,
    Cluster
};

enum class k3fbxAnimCurveType {
    None,
    Translation,
    Scaling,
    Rotation
};

enum class k3fbxPoseType {
    None,
    BindPose
};

struct k3fbxMeshData {
    uint64_t id;
    uint32_t start;
    uint32_t vert_offset;
    uint32_t normal_offset;
    uint32_t normal_index_offset;
    uint32_t tangent_offset;
    uint32_t tangent_index_offset;
    uint32_t uv_offset;
    uint32_t uv_index_offset;
    uint32_t material_offset;
    k3fbxMapping normal_mapping;
    k3fbxMapping tangent_mapping;
    k3fbxMapping uv_mapping;
    k3fbxMapping material_mapping;
    k3fbxReference normal_reference;
    k3fbxReference tangent_reference;
    k3fbxReference uv_reference;
};

struct k3fbxModelIndexData {
    uint32_t parent;
    uint32_t mesh;
    uint32_t material;
};

struct k3fbxLightIndexData {
    uint32_t light_node;
};

struct k3fbxLimbNodeIndexData {
    uint32_t parent;
    uint32_t cluster_index;
    uint32_t bind_pose_index;
};

union k3fbxModelIndex {
    k3fbxModelIndexData model;
    k3fbxLightIndexData light;
    k3fbxLimbNodeIndexData limb_node;
};

struct k3fbxModelData {
    uint64_t id;
    k3fbxObjType obj_type;
    k3fbxModelIndex index;
    char name[K3_FBX_MAX_NAME_LENGTH];
    float translation[3];
    float rotation[3];
    float scaling[3];
    float visibility;
};

struct k3fbxMaterialData {
    uint64_t id;
    float diffuse_color[3];
    uint32_t diffuse_texture_index;
    uint32_t normal_texture_index;
};

static const uint32_t K3_FBX_FILENAME_SIZE = 64;

struct k3fbxTextureData {
    uint64_t id;
    char filename[K3_FBX_FILENAME_SIZE];
    uint32_t file_pos;
};

struct k3fbxContentData {
    uint64_t id;
    char filename[K3_FBX_FILENAME_SIZE];
    uint32_t file_pos;
};

struct k3fbxCamera {
    uint64_t id;
    char name[K3_FBX_MAX_NAME_LENGTH];
    float translation[3];
    float look_at[3];
    float up[3];
    uint32_t proj_type;
    uint32_t res_x;
    uint32_t res_y;
    float fovx;
    float ortho_scale;
    float near_plane;
    float far_plane;
};

struct k3fbxLightNode {
    uint64_t id;
    char name[K3_FBX_MAX_NAME_LENGTH];
    uint32_t light_type;
    float color[3];
    float intensity;
    uint32_t decay_type;
    float decay_start;
    bool cast_shadows;
};

struct k3fbxSkin {
    uint64_t id;
    uint32_t mesh_index;
};
struct k3fbxClusterNode {
    uint64_t id;
    uint32_t index_start;
    uint32_t weight_start;
    uint32_t skin_index;
    char weight_type;
    uint32_t bone_index;
};

static const uint32_t K3_FBX_BONE_TO_ANIM_MULTIPLIER = 4;

struct k3fbxAnimLayer {
    uint64_t id;
    uint32_t num_anim_curve_nodes;
    char name[K3_FBX_MAX_NAME_LENGTH];
    uint32_t* anim_curve_node_id;
};

struct k3fbxAnimCurveNode {
    uint64_t id;
    k3fbxAnimCurveType curve_type;
    uint32_t anim_curve[3];
    uint32_t limb_node;
    float default_value[3];
};

struct k3fbxAnimCurve {
    uint64_t id;
    uint32_t num_key_frames;
    uint32_t time_start;
    uint32_t data_start;
};

struct k3fbxPoseNode {
    uint64_t model_id;
    float xform[16];
};

struct k3fbxCustomPropDesc {
    uint32_t num_model_custom_props;
    const char** model_custom_prop_name;
};

struct k3fbxData {
    uint32_t num_meshes;
    uint32_t num_models;
    uint32_t num_materials;
    uint32_t num_textures;
    uint32_t num_content_data;
    uint32_t num_vertices;
    uint32_t num_vertex_bytes;
    uint32_t num_indices;
    uint32_t num_normal_indices;
    uint32_t num_tangent_indices;
    uint32_t num_uv_indices;
    uint32_t num_normals;
    uint32_t num_normal_bytes;
    uint32_t num_tangents;
    uint32_t num_tangent_bytes;
    uint32_t num_uvs;
    uint32_t num_uv_bytes;
    uint32_t num_material_ids;
    uint32_t num_cameras;
    uint32_t num_light_nodes;
    uint32_t num_lights;
    uint32_t num_skins;
    uint32_t num_clusters;
    uint32_t num_cluster_indexes;
    uint32_t num_cluster_weights;
    uint32_t num_cluster_weight_bytes;
    uint32_t num_anim_layers;
    uint32_t num_anim_curve_nodes;
    uint32_t num_anim_curves;
    uint32_t num_anim_curve_time_elements;
    uint32_t num_anim_curve_data_elements;
    uint32_t num_bind_pose_nodes;
    k3fbxDeformer deformer_type;
    k3fbxPoseType pose_type;
    k3fbxReference last_normal_reference;
    k3fbxReference last_tangent_reference;
    k3fbxReference last_uv_reference;
    k3fbxMeshData* mesh;
    k3fbxModelData* model;
    k3flint32* model_custom_prop;
    k3fbxMaterialData* material;
    k3fbxTextureData* texture;
    k3fbxContentData* content_data;
    k3fbxLightNode* light_node;
    uint32_t* indices;
    uint32_t* normal_indices;
    uint32_t* tangent_indices;
    uint32_t* uv_indices;
    uint32_t* material_ids;
    k3fbxObjType node_attrib_obj;
    k3fbxCamera* camera;
    k3fbxSkin* skin;
    k3fbxClusterNode* cluster;
    k3fbxAnimLayer* anim_layer;
    k3fbxAnimCurveNode* anim_curve_node;
    k3fbxAnimCurve* anim_curve;
    k3fbxPoseNode* bind_pose_node;
    void* vertices;
    void* normals;
    void* tangents;
    void* uvs;
    uint32_t* cluster_indexes;
    void* cluster_weights;
    uint64_t* anim_curve_time;
    float* anim_curve_data;
    char vert_type;
    char norm_type;
    char tang_type;
    char uv_type;
};

struct k3fbxAttrLinkList {
    uint32_t n_index;
    uint32_t uv_index;
    float* attr;
    k3fbxAttrLinkList* next;
};

static const uint64_t CONNECT_TYPE_UNKNOWN = 0;
static const uint64_t CONNECT_TYPE_DIFFUSE_MAP = 1;
static const uint64_t CONNECT_TYPE_NORMAL_MAP = 2;
static const uint64_t CONNECT_TYPE_X_AXIS = 0x100;
static const uint64_t CONNECT_TYPE_Y_AXIS = 0x101;
static const uint64_t CONNECT_TYPE_Z_AXIS = 0x102;

void doubleToFloatArray(void* arr, uint32_t size)
{
    double* d = (double*)arr;
    float* f = (float*)arr;
    uint32_t i;
    for (i = 0; i < size; i++) {
        *f = (float)*d;
        f++;
        d++;
    }
}

uint32_t findFbxLightNode(k3fbxData* fbx, uint64_t id)
{
    uint32_t i;
    for (i = 0; i < fbx->num_light_nodes; i++) {
        if (fbx->light_node[i].id == id) return i;
    }
    return ~0x0;
}

uint32_t findFbxContentDataNode(k3fbxData* fbx, uint64_t id)
{
    uint32_t i;
    for (i = 0; i < fbx->num_content_data; i++) {
        if (fbx->content_data[i].id == id) return i;
    }
    return ~0x0;
}

uint32_t findFbxTextureNode(k3fbxData* fbx, uint64_t id)
{
    uint32_t i;
    for (i = 0; i < fbx->num_textures; i++) {
        if (fbx->texture[i].id == id) return i;
    }
    return ~0x0;
}

uint32_t findFbxMaterialNode(k3fbxData* fbx, uint64_t id)
{
    uint32_t i;
    for (i = 0; i < fbx->num_materials; i++) {
        if (fbx->material[i].id == id) return i;
    }
    return ~0x0;
}

uint32_t findFbxMeshNode(k3fbxData* fbx, uint64_t id)
{
    uint32_t i;
    for (i = 0; i < fbx->num_meshes; i++) {
        if (fbx->mesh[i].id == id) return i;
    }
    return ~0x0;
}

uint32_t findFbxModelNode(k3fbxData* fbx, uint64_t id)
{
    uint32_t i;
    for (i = 0; i < fbx->num_models; i++) {
        if (fbx->model[i].id == id) return i;
    }
    return ~0x0;
}

uint32_t findFbxSkin(k3fbxData* fbx, uint64_t id)
{
    uint32_t i;
    for (i = 0; i < fbx->num_skins; i++) {
        if (fbx->skin[i].id == id) return i;
    }
    return ~0x0;
}

uint32_t findFbxCluster(k3fbxData* fbx, uint64_t id)
{
    uint32_t i;
    for (i = 0; i < fbx->num_clusters; i++) {
        if (fbx->cluster[i].id == id) return i;
    }
    return ~0x0;
}

uint32_t findFbxAnimLayer(k3fbxData* fbx, uint64_t id)
{
    uint32_t i;
    for (i = 0; i < fbx->num_anim_layers; i++) {
        if (fbx->anim_layer[i].id == id) return i;
    }
    return ~0x0;
}

uint32_t findFbxAnimCurveNode(k3fbxData* fbx, uint64_t id)
{
    uint32_t i;
    for (i = 0; i < fbx->num_anim_curve_nodes; i++) {
        if (fbx->anim_curve_node[i].id == id) return i;
    }
    return ~0x0;
}

uint32_t findFbxAnimCurve(k3fbxData* fbx, uint64_t id)
{
    uint32_t i;
    for (i = 0; i < fbx->num_anim_curves; i++) {
        if (fbx->anim_curve[i].id == id) return i;
    }
    return ~0x0;
}

void connectFbxNode(k3fbxData* fbx, uint64_t* id)
{
    if (fbx->mesh) {
        uint32_t index0, index1;

        // Check if id0 is a mesh
        index0 = findFbxMeshNode(fbx, id[0]);
        if (index0 != ~0x0) {
            // id1 shoud be a model
            index1 = findFbxModelNode(fbx, id[1]);
            if (index1 != ~0x0) {
                fbx->model[index1].index.model.mesh = index0;
            }
            return;
        }
        // Check if id0 is a material
        index0 = findFbxMaterialNode(fbx, id[0]);
        if (index0 != ~0x0) {
            // id1 shoud be a model
            index1 = findFbxModelNode(fbx, id[1]);
            if (index1 != ~0x0) {
                fbx->model[index1].index.model.material = index0;
            }
            return;
        }
        // Check if id0 is a texture
        index0 = findFbxTextureNode(fbx, id[0]);
        if (index0 != ~0x0) {
            // id1 should be a material
            index1 = findFbxMaterialNode(fbx, id[1]);
            if (index1 != ~0x0) {
                switch (id[2]) {
                case CONNECT_TYPE_DIFFUSE_MAP:
                    fbx->material[index1].diffuse_texture_index = index0;
                    break;
                case CONNECT_TYPE_NORMAL_MAP:
                    fbx->material[index1].normal_texture_index = index0;
                    break;
                }
            }
            return;
        }
        // Check if id0 is content data
        index0 = findFbxContentDataNode(fbx, id[0]);
        if (index0 != ~0x0) {
            // id1 should be a texture
            index1 = findFbxTextureNode(fbx, id[1]);
            if (index1 != ~0x0) {
                fbx->texture[index1].file_pos = fbx->content_data[index0].file_pos;
            }
            return;
        }
        // Check if id0 is a light node
        index0 = findFbxLightNode(fbx, id[0]);
        if (index0 != ~0x0) {
            // id1 should be a model
            index1 = findFbxModelNode(fbx, id[1]);
            if (index1 != ~0x0) {
                fbx->model[index1].index.light.light_node = index0;
            }
        }
        // Check if id0 is a model, and a limb node
        index0 = findFbxModelNode(fbx, id[0]);
        if (index0 != ~0x0 && fbx->model[index0].obj_type == k3fbxObjType::LIMB_NODE) {
            // id1 should also be a model/limb node
            index1 = findFbxModelNode(fbx, id[1]);
            if (index1 != ~0x0 && fbx->model[index1].obj_type == k3fbxObjType::LIMB_NODE) {
                fbx->model[index0].index.limb_node.parent = index1;
            }
            // check if id1 is a cluster
            index1 = findFbxCluster(fbx, id[1]);
            if (index1 != ~0x0) {
                //fbx->model[index0].index.limb_node.cluster = index0;
                fbx->cluster[index1].bone_index = index0;
                fbx->model[index0].index.limb_node.cluster_index = index1;
            }
        }
        if (index0 != ~0x0 && fbx->model[index0].obj_type == k3fbxObjType::MESH) {
            // id1 should be a model/mesh
            index1 = findFbxModelNode(fbx, id[1]);
            if (index1 != ~0x0 && fbx->model[index1].obj_type == k3fbxObjType::MESH) {
                fbx->model[index0].index.model.parent = index1;
            }
        }
        // Check if id0 is a cluster
        index0 = findFbxCluster(fbx, id[0]);
        if (index0 != ~0x0) {
            // id1 should be a skin
            index1 = findFbxSkin(fbx, id[1]);
            if (index1 != ~0x0) {
                fbx->cluster[index0].skin_index = index1;
            }
        }
        // Check if id0 is a skin
        index0 = findFbxSkin(fbx, id[0]);
        if (index0 != ~0x0) {
            // id1 should be a mesh
            index1 = findFbxMeshNode(fbx, id[1]);
            if (index1 != ~0x0) {
                fbx->skin[index0].mesh_index = index1;
            }
        }
        // Check if id0 is a anim_curve_node
        index0 = findFbxAnimCurveNode(fbx, id[0]);
        if (index0 != ~0x0) {
            // check if id1 is an anim_layer
            index1 = findFbxAnimLayer(fbx, id[1]);
            if (index1 != ~0x0) {
                if (fbx->anim_layer[index1].num_anim_curve_nodes < K3_FBX_BONE_TO_ANIM_MULTIPLIER * fbx->num_clusters) {
                    fbx->anim_layer[index1].anim_curve_node_id[fbx->anim_layer[index1].num_anim_curve_nodes] = index0;
                    fbx->anim_layer[index1].num_anim_curve_nodes++;
                } else {
                    k3error::Handler("Too many anim curves in animation layer", "connectFbxNode");
                }
            } else {
                // Check if id1 is a limb node
                index1 = findFbxModelNode(fbx, id[1]);
                if (index1 != ~0x0 && fbx->model[index1].obj_type == k3fbxObjType::LIMB_NODE) {
                    fbx->anim_curve_node[index0].limb_node = index1;
                }
            }
        }
        // Check if id0 is an anim_curve
        index0 = findFbxAnimCurve(fbx, id[0]);
        if (index0 != ~0x0) {
            // id1 should be an anim_curvve_node
            index1 = findFbxAnimCurveNode(fbx, id[1]);
            if (index1 != ~0x0) {
                switch (id[2]) {
                case CONNECT_TYPE_X_AXIS:
                    fbx->anim_curve_node[index1].anim_curve[0] = index0;
                    break;
                case CONNECT_TYPE_Y_AXIS:
                    fbx->anim_curve_node[index1].anim_curve[1] = index0;
                    break;
                case CONNECT_TYPE_Z_AXIS:
                    fbx->anim_curve_node[index1].anim_curve[2] = index0;
                    break;
                }
            }
        }
    }
}

void readFbxNode(k3fbxData* fbx, k3fbxNodeType parent_node, uint32_t level, FILE* in_file, k3fbxCustomPropDesc* custom_props)
{
    static char buffer[512] = { 0 };
    static char* str = buffer;
    static char* str2 = buffer + 256;
    static int16_t* i16_arr = (int16_t*)buffer;
    static float* f32_arr = (float*)buffer;
    static double* d64_arr = (double*)buffer;
    static int64_t* i64_arr = (int64_t*)buffer;
    static int32_t* i32_arr = (int32_t*)buffer;

    if (parent_node == k3fbxNodeType::UNKNOWN) {
        // reset counts on entry
        fbx->num_meshes = 0;
        fbx->num_models = 0;
        fbx->num_materials = 0;
        fbx->num_textures = 0;
        fbx->num_content_data = 0;
        fbx->num_vertices = 0;
        fbx->num_vertex_bytes = 0;
        fbx->num_indices = 0;
        fbx->num_normal_indices = 0;
        fbx->num_tangent_indices = 0;
        fbx->num_uv_indices = 0;
        fbx->num_normals = 0;
        fbx->num_normal_bytes = 0;
        fbx->num_tangents = 0;
        fbx->num_tangent_bytes = 0;
        fbx->num_uvs = 0;
        fbx->num_uv_bytes = 0;
        fbx->num_material_ids = 0;
        fbx->num_cameras = 0;
        fbx->num_lights = 0;
        fbx->num_light_nodes = 0;
        fbx->node_attrib_obj = k3fbxObjType::NONE;
        fbx->num_skins = 0;
        fbx->num_clusters = 0;
        fbx->num_cluster_indexes = 0;
        fbx->num_cluster_weights = 0;
        fbx->num_cluster_weight_bytes = 0;
        fbx->deformer_type = k3fbxDeformer::None;
        fbx->pose_type = k3fbxPoseType::None;
        if (fbx->anim_layer) {
            uint32_t i;
            for (i = 0; i < fbx->num_anim_layers; i++) {
                fbx->anim_layer[i].num_anim_curve_nodes = 0;
            }
        }
        fbx->num_anim_layers = 0;
        fbx->num_anim_curve_nodes = 0;
        fbx->num_anim_curves = 0;
        fbx->num_anim_curve_time_elements = 0;
        fbx->num_anim_curve_data_elements = 0;
        fbx->num_bind_pose_nodes = 0;
        if(K3_FBX_DEBUG) printf("----------starting fbx parse ---------------\n");
    }

    k3fbxNodeRecord node;
    k3fbxArrayProperty arr_prop;
    char typecode;
    uint32_t bytes_remaining, uncomp_bytes_remaining;
    uint32_t i;
    z_stream zs = { 0 };

    k3fbxNodeType node_type = k3fbxNodeType::UNKNOWN;
    void* data_arr;
    uint32_t connect_index;
    uint32_t connect_params;
    uint64_t connect_id[3];
    k3fbxProperty fbx_property = k3fbxProperty::NONE;
    uint32_t fbx_custom_prop_index = 0;
    uint32_t fbx_property_argument = 0;
    uint32_t fbx_node_argument = 0;
    uint64_t node_attrib_id;
    uint32_t str_file_pos;
    bool first_uv = true;
    double mat_d[16];
    char node_attr_name[K3_FBX_MAX_NAME_LENGTH];

    while (1) {
        fread(&node, K3_FBX_NODE_RECORD_LENGTH, 1, in_file);
        if (node.end_offset == 0)
            break;
        if (node.name_len) {
            fread(str, 1, node.name_len, in_file);
            str[node.name_len] = '\0';
            if (!strncmp(str, "Objects", 8)) node_type = k3fbxNodeType::OBJECTS;
            else if (!strncmp(str, "Geometry", 9)) node_type = k3fbxNodeType::GEOMETRY;
            else if (!strncmp(str, "Vertices", 9)) node_type = k3fbxNodeType::VERTICES;
            else if (!strncmp(str, "PolygonVertexIndex", 19)) node_type = k3fbxNodeType::POLYGON_VERT_INDEX;
            else if (!strncmp(str, "LayerElementNormal", 19)) node_type = k3fbxNodeType::LAYER_ELEMENT_NORMAL;
            else if (!strncmp(str, "LayerElementTangent", 20)) node_type = k3fbxNodeType::LAYER_ELEMENT_TANGENT;
            else if (!strncmp(str, "LayerElementUV", 15) && first_uv) { node_type = k3fbxNodeType::LAYER_ELEMENT_UV; first_uv = false; } else if (!strncmp(str, "LayerElementMaterial", 21)) node_type = k3fbxNodeType::LAYER_ELEMENT_MATERIAL;
            else if (!strncmp(str, "MappingInformationType", 23)) node_type = k3fbxNodeType::MAPPING_TYPE;
            else if (!strncmp(str, "ReferenceInformationType", 25)) node_type = k3fbxNodeType::REFERENCE_TYPE;
            else if (!strncmp(str, "Normals", 8)) node_type = k3fbxNodeType::NORMALS;
            else if (!strncmp(str, "NormalIndex", 12)) node_type = k3fbxNodeType::NORMAL_INDEX;
            else if (!strncmp(str, "NormalsIndex", 13)) node_type = k3fbxNodeType::NORMAL_INDEX;
            else if (!strncmp(str, "Tangents", 9)) node_type = k3fbxNodeType::TANGENTS;
            else if (!strncmp(str, "TangentIndex", 13)) node_type = k3fbxNodeType::TANGENT_INDEX;
            else if (!strncmp(str, "TangentsIndex", 14)) node_type = k3fbxNodeType::TANGENT_INDEX;
            else if (!strncmp(str, "UVIndex", 8)) node_type = k3fbxNodeType::UV_INDEX;
            else if (!strncmp(str, "UV", 3)) node_type = k3fbxNodeType::UV;
            else if (!strncmp(str, "Model", 6)) node_type = k3fbxNodeType::MODEL;
            else if (!strncmp(str, "Materials", 10)) node_type = k3fbxNodeType::MATERIAL_IDS;
            //else if (!strncmp(str, "Layer", 6)) node_type = k3fbxNodeType::LAYER;
            //else if (!strncmp(str, "LayerElement", 13)) node_type = k3fbxNodeType::LAYER_ELEMENT;
            else if (!strncmp(str, "Material", 9)) node_type = k3fbxNodeType::MATERIAL;
            else if (!strncmp(str, "Texture", 8)) node_type = k3fbxNodeType::TEXTURE;
            else if (!strncmp(str, "RelativeFilename", 17)) node_type = k3fbxNodeType::RELATIVE_FILE_NAME;
            else if (!strncmp(str, "Properties70", 13)) node_type = k3fbxNodeType::PROPRTIES70;
            else if (!strncmp(str, "P", 2)) node_type = k3fbxNodeType::PROP;
            else if (!strncmp(str, "Connections", 12)) node_type = k3fbxNodeType::CONNECTIONS;
            else if (!strncmp(str, "C", 2)) node_type = k3fbxNodeType::CONNECT;
            else if (!strncmp(str, "Video", 6)) node_type = k3fbxNodeType::VIDEO;
            else if (!strncmp(str, "Content", 8)) node_type = k3fbxNodeType::CONTENT;
            else if (!strncmp(str, "NodeAttribute", 14)) node_type = k3fbxNodeType::NODE_ATTRIBUTE;
            else if (!strncmp(str, "Position", 9)) node_type = k3fbxNodeType::POSITION;
            else if (!strncmp(str, "Up", 3)) node_type = k3fbxNodeType::UP;
            else if (!strncmp(str, "LookAt", 7)) node_type = k3fbxNodeType::LOOK_AT;
            else if (!strncmp(str, "Deformer", 9)) node_type = k3fbxNodeType::DEFORMER;
            else if (!strncmp(str, "Indexes", 8)) node_type = k3fbxNodeType::INDEXES;
            else if (!strncmp(str, "Weights", 8)) node_type = k3fbxNodeType::WEIGHTS;
            else if (!strncmp(str, "Transform", 10)) node_type = k3fbxNodeType::TRANSFORM;
            else if (!strncmp(str, "TransformLink", 14)) node_type = k3fbxNodeType::TRANSFORM_LINK;
            else if (!strncmp(str, "TransformAssociateModel", 24)) node_type = k3fbxNodeType::TRANSFORM_ASSOCIATE_MODEL;
            else if (!strncmp(str, "Pose", 5)) node_type = k3fbxNodeType::POSE;
            else if (!strncmp(str, "NbPoseNodes", 12)) node_type = k3fbxNodeType::NB_POSE_NODES;
            else if (!strncmp(str, "PoseNode", 9)) node_type = k3fbxNodeType::POSE_NODE;
            else if (!strncmp(str, "Node", 5)) node_type = k3fbxNodeType::NODE;
            else if (!strncmp(str, "Matrix", 7)) node_type = k3fbxNodeType::MATRIX;
            else if (!strncmp(str, "AnimationStack", 15)) node_type = k3fbxNodeType::ANIMATION_STACK;
            else if (!strncmp(str, "AnimationLayer", 15)) node_type = k3fbxNodeType::ANIMATION_LAYER;
            else if (!strncmp(str, "AnimationCurveNode", 19)) node_type = k3fbxNodeType::ANIMATION_CURVE_NODE;
            else if (!strncmp(str, "AnimationCurve", 15)) node_type = k3fbxNodeType::ANIMATION_CURVE;
            else if (!strncmp(str, "KeyTime", 8)) node_type = k3fbxNodeType::KEY_TIME;
            else if (!strncmp(str, "KeyValueFloat", 14)) node_type = k3fbxNodeType::KEY_VALUE_FLOAT;
            else node_type = k3fbxNodeType::UNKNOWN;
            if (K3_FBX_DEBUG) {
                for (i = 0; i < level; i++) printf("-");
                printf("Name: %s\n", str);
            }
            if (node_type == k3fbxNodeType::UNKNOWN) {
                if (K3_FBX_DEBUG) printf("<---Ignoring--->\n");
                fseek(in_file, node.end_offset, SEEK_SET);
                continue;
            }
        }

        if (node_type == k3fbxNodeType::GEOMETRY) {
            if (fbx->mesh) {
                // if we have mesh, enter it
                fbx->mesh[fbx->num_meshes].id = 0;
                fbx->mesh[fbx->num_meshes].start = fbx->num_indices;
                fbx->mesh[fbx->num_meshes].vert_offset = fbx->num_vertices;
                fbx->mesh[fbx->num_meshes].normal_offset = fbx->num_normals;
                fbx->mesh[fbx->num_meshes].normal_index_offset = fbx->num_normal_indices;
                fbx->mesh[fbx->num_meshes].tangent_offset = fbx->num_tangents;
                fbx->mesh[fbx->num_meshes].tangent_index_offset = fbx->num_tangent_indices;
                fbx->mesh[fbx->num_meshes].uv_offset = fbx->num_uvs;
                fbx->mesh[fbx->num_meshes].uv_index_offset = fbx->num_uv_indices;
                fbx->mesh[fbx->num_meshes].material_offset = fbx->num_material_ids;
                fbx->mesh[fbx->num_meshes].normal_mapping = k3fbxMapping::None;
                fbx->mesh[fbx->num_meshes].tangent_mapping = k3fbxMapping::None;
                fbx->mesh[fbx->num_meshes].uv_mapping = k3fbxMapping::None;
                fbx->mesh[fbx->num_meshes].material_mapping = k3fbxMapping::None;
                fbx->mesh[fbx->num_meshes].normal_reference = k3fbxReference::None;
                fbx->mesh[fbx->num_meshes].tangent_reference = k3fbxReference::None;
                fbx->mesh[fbx->num_meshes].uv_reference = k3fbxReference::None;
            }
            fbx->num_meshes++;
            fbx_node_argument = 0;
        }

        if (node_type == k3fbxNodeType::MODEL) {
            if (fbx->model) {
                fbx->model[fbx->num_models].id = 0;
                fbx->model[fbx->num_models].obj_type = k3fbxObjType::NONE;
                fbx->model[fbx->num_models].index.model.parent = ~0;
                fbx->model[fbx->num_models].index.model.mesh = ~0;
                fbx->model[fbx->num_models].index.model.material = ~0;
                fbx->model[fbx->num_models].name[0] = '\0';
                fbx->model[fbx->num_models].translation[0] = 0.0f;
                fbx->model[fbx->num_models].translation[1] = 0.0f;
                fbx->model[fbx->num_models].translation[2] = 0.0f;
                fbx->model[fbx->num_models].rotation[0] = 0.0f;
                fbx->model[fbx->num_models].rotation[1] = 0.0f;
                fbx->model[fbx->num_models].rotation[2] = 0.0f;
                fbx->model[fbx->num_models].scaling[0] = 1.0f;
                fbx->model[fbx->num_models].scaling[1] = 1.0f;
                fbx->model[fbx->num_models].scaling[2] = 1.0f;
                fbx->model[fbx->num_models].visibility = 1.0f;
            }
            fbx->num_models++;
            fbx_node_argument = 0;
        }

        if (node_type == k3fbxNodeType::MATERIAL) {
            if (fbx->material) {
                fbx->material[fbx->num_materials].id = 0;
                fbx->material[fbx->num_materials].diffuse_color[0] = 1.0f;
                fbx->material[fbx->num_materials].diffuse_color[1] = 1.0f;
                fbx->material[fbx->num_materials].diffuse_color[2] = 1.0f;
                fbx->material[fbx->num_materials].diffuse_texture_index = ~0;
                fbx->material[fbx->num_materials].normal_texture_index = ~0;
            }
            fbx->num_materials++;
            fbx_node_argument = 0;
        }

        if (node_type == k3fbxNodeType::TEXTURE) {
            if (fbx->texture) {
                fbx->texture[fbx->num_textures].id = 0;
                fbx->texture[fbx->num_textures].filename[0] = '\0';
                fbx->texture[fbx->num_textures].file_pos = ~0x0;
            }
            fbx->num_textures++;
            fbx_node_argument = 0;
        }

        if(node_type == k3fbxNodeType::VIDEO) {
            if (fbx->content_data) {
                fbx->content_data[fbx->num_content_data].id = 0;
                fbx->content_data[fbx->num_content_data].filename[0] = '\0';
                fbx->content_data[fbx->num_content_data].file_pos = ~0x0;
            }
            fbx->num_content_data++;
            fbx_node_argument = 0;
        }

        if (node_type == k3fbxNodeType::DEFORMER) {
            fbx_node_argument = 0;
        }

        if (node_type == k3fbxNodeType::PROP || node_type == k3fbxNodeType::POSITION ||
            node_type == k3fbxNodeType::UP || node_type == k3fbxNodeType::LOOK_AT) {
            fbx_property = k3fbxProperty::NONE;
            fbx_property_argument = 0;
        }

        if (node_type == k3fbxNodeType::NODE_ATTRIBUTE) {
            fbx->node_attrib_obj = k3fbxObjType::NONE;
            fbx_node_argument = 0;
        }

        if (node_type == k3fbxNodeType::CONNECT) {
            connect_index = 0;
            connect_params = 0;
        }

        if (node_type == k3fbxNodeType::ANIMATION_LAYER) {
            if (fbx->anim_layer) {
                fbx->anim_layer[fbx->num_anim_layers].id = 0;
                fbx->anim_layer[fbx->num_anim_layers].name[0] = '\0';
            }
            fbx->num_anim_layers++;
            fbx_node_argument = 0;
        }

        if (node_type == k3fbxNodeType::ANIMATION_CURVE_NODE) {
            if (fbx->anim_curve_node) {
                fbx->anim_curve_node[fbx->num_anim_curve_nodes].id = 0;
                fbx->anim_curve_node[fbx->num_anim_curve_nodes].curve_type = k3fbxAnimCurveType::None;
                fbx->anim_curve_node[fbx->num_anim_curve_nodes].anim_curve[0] = ~0x0;
                fbx->anim_curve_node[fbx->num_anim_curve_nodes].anim_curve[1] = ~0x0;
                fbx->anim_curve_node[fbx->num_anim_curve_nodes].anim_curve[2] = ~0x0;
                fbx->anim_curve_node[fbx->num_anim_curve_nodes].limb_node = ~0x0;
                fbx->anim_curve_node[fbx->num_anim_curve_nodes].default_value[0] = 0.0f;
                fbx->anim_curve_node[fbx->num_anim_curve_nodes].default_value[1] = 0.0f;
                fbx->anim_curve_node[fbx->num_anim_curve_nodes].default_value[2] = 0.0f;
            }
            fbx->num_anim_curve_nodes++;
            fbx_node_argument = 0;
        }

        if (node_type == k3fbxNodeType::ANIMATION_CURVE) {
            if (fbx->anim_curve) {
                fbx->anim_curve[fbx->num_anim_curves].id = 0;
                fbx->anim_curve[fbx->num_anim_curves].num_key_frames = 0;
                fbx->anim_curve[fbx->num_anim_curves].time_start = ~0x0;
                fbx->anim_curve[fbx->num_anim_curves].data_start = ~0x0;
            }
            fbx->num_anim_curves++;
            fbx_node_argument = 0;
        }

        if (node_type == k3fbxNodeType::POSE) {
            fbx_node_argument = 0;
        }

        if (node_type == k3fbxNodeType::POSE_NODE && parent_node == k3fbxNodeType::POSE && fbx->pose_type == k3fbxPoseType::BindPose) {
            if (fbx->bind_pose_node) {
                fbx->bind_pose_node[fbx->num_bind_pose_nodes].model_id = 0;
                k3m4_SetIdentity(fbx->bind_pose_node[fbx->num_bind_pose_nodes].xform);
            }
            fbx->num_bind_pose_nodes++;
            fbx_node_argument = 0;
        }

        if (node_type == k3fbxNodeType::NODE) {
            fbx_node_argument = 0;
        }

        for (i = 0; i < node.num_properties; i++) {
            fread(&typecode, 1, 1, in_file);
            switch (typecode) {
            case K3_FBX_TYPECODE_SINT16:
                fread(i16_arr, sizeof(int16_t), 1, in_file);
                if (K3_FBX_DEBUG) printf("Y: %d\n", *i16_arr);
                break;
            case K3_FBX_TYPECODE_BOOL:
                fread(str, sizeof(char), 1, in_file);
                if (K3_FBX_DEBUG) printf("C: %s\n", (*str != 0) ? "true" : "false");
                break;
            case K3_FBX_TYPECODE_SINT32:
                fread(i32_arr, sizeof(int32_t), 1, in_file);
                if (K3_FBX_DEBUG) printf("I: %d\n", *i32_arr);
                switch (node_type) {
                case k3fbxNodeType::PROP:
                    switch (fbx_property) {
                    case k3fbxProperty::LIGHT_TYPE:
                        if (fbx->node_attrib_obj == k3fbxObjType::LIGHT && fbx->light_node && fbx_property_argument < 1) {
                            fbx->light_node[fbx->num_light_nodes - 1].light_type = *i32_arr;
                            fbx_property_argument++;
                        }
                        break;
                    case k3fbxProperty::DECAY_TYPE:
                        if (fbx->node_attrib_obj == k3fbxObjType::LIGHT && fbx->light_node && fbx_property_argument < 1) {
                            fbx->light_node[fbx->num_light_nodes - 1].decay_type = *i32_arr;
                            fbx_property_argument++;
                        }
                        break;
                    case k3fbxProperty::CAST_SHADOWS:
                        if (fbx->node_attrib_obj == k3fbxObjType::LIGHT && fbx->light_node && fbx_property_argument < 1) {
                            fbx->light_node[fbx->num_light_nodes - 1].cast_shadows = (*i32_arr != 0);
                            fbx_property_argument++;
                        }
                        break;
                    case k3fbxProperty::PROJ_TYPE:
                        if (fbx->node_attrib_obj == k3fbxObjType::CAMERA && fbx->camera && fbx_property_argument < 1) {
                            fbx->camera[fbx->num_cameras - 1].proj_type = *i32_arr;
                            fbx_property_argument++;
                        }
                        break;
                    case k3fbxProperty::VISIBILITY:
                        if (fbx->model && fbx_property_argument < 1) {
                            fbx->model[fbx->num_models - 1].visibility = *i32_arr;
                            fbx_property_argument++;
                        }
                        break;
                    case k3fbxProperty::CUSTOM_PROP:
                        if (fbx->model && fbx_property_argument < 1) {
                            fbx->model_custom_prop[(fbx->num_models - 1) * custom_props->num_model_custom_props + fbx_custom_prop_index].i = *i32_arr;
                            fbx_property_argument++;
                        }
                        break;
                    }
                    break;
                }
                break;
            case K3_FBX_TYPECODE_FLOAT:
                fread(f32_arr, sizeof(float), 1, in_file);
                if (K3_FBX_DEBUG) printf("F: %0.4f\n", *f32_arr);
                switch (node_type) {
                case k3fbxNodeType::POSITION:
                    if (fbx->node_attrib_obj == k3fbxObjType::CAMERA && fbx->camera && fbx_property_argument < 3) {
                        fbx->camera[fbx->num_cameras - 1].translation[fbx_property_argument] = *f32_arr;
                        fbx_property_argument++;
                    }
                    break;
                case k3fbxNodeType::UP:
                    if (fbx->node_attrib_obj == k3fbxObjType::CAMERA && fbx->camera && fbx_property_argument < 3) {
                        fbx->camera[fbx->num_cameras - 1].up[fbx_property_argument] = *f32_arr;
                        fbx_property_argument++;
                    }
                    break;
                case k3fbxNodeType::LOOK_AT:
                    if (fbx->node_attrib_obj == k3fbxObjType::CAMERA && fbx->camera && fbx_property_argument < 3) {
                        fbx->camera[fbx->num_cameras - 1].look_at[fbx_property_argument] = *f32_arr;
                        fbx_property_argument++;
                    }
                    break;
                case k3fbxNodeType::PROP:
                    switch (fbx_property) {
                    case k3fbxProperty::LOCAL_TRANSLATION:
                        if (fbx->model && fbx_property_argument < 3) {
                            fbx->model[fbx->num_models - 1].translation[fbx_property_argument] = *f32_arr;
                            fbx_property_argument++;
                        }
                        break;
                    case k3fbxProperty::LOCAL_ROTATION:
                        if (fbx->model && fbx_property_argument < 3) {
                            fbx->model[fbx->num_models - 1].rotation[fbx_property_argument] = *f32_arr;
                            fbx_property_argument++;
                        }
                        break;
                    case k3fbxProperty::LOCAL_SCALING:
                        if (fbx->model && fbx_property_argument < 3) {
                            fbx->model[fbx->num_models - 1].scaling[fbx_property_argument] = *f32_arr;
                            fbx_property_argument++;
                        }
                        break;
                    case k3fbxProperty::DIFFUSE_COLOR:
                        if (fbx->material && fbx_property_argument < 3) {
                            fbx->material[fbx->num_materials - 1].diffuse_color[fbx_property_argument] = *f32_arr;
                            fbx_property_argument++;
                        }
                        break;
                    //case k3fbxProperty::POSITION:
                    //    if (fbx->node_attrib_obj == k3fbxObjType::CAMERA && fbx->camera && fbx_property_argument < 3) {
                    //        fbx->camera[fbx->num_cameras - 1].translation[fbx_property_argument] = *f32_arr;
                    //        fbx_property_argument++;
                    //    }
                    //    break;
                    //case k3fbxProperty::UP_VECTOR:
                    //    if (fbx->node_attrib_obj == k3fbxObjType::CAMERA && fbx->camera && fbx_property_argument < 3) {
                    //        fbx->camera[fbx->num_cameras - 1].up[fbx_property_argument] = *f32_arr;
                    //        fbx_property_argument++;
                    //    }
                    //    break;
                    //case k3fbxProperty::INTEREST_POSITION:
                    //    if (fbx->node_attrib_obj == k3fbxObjType::CAMERA && fbx->camera && fbx_property_argument < 3) {
                    //        fbx->camera[fbx->num_cameras - 1].look_at[fbx_property_argument] = *f32_arr;
                    //        fbx_property_argument++;
                    //    }
                    //    break;
                    case k3fbxProperty::ASPECT_WIDTH:
                        if (fbx->node_attrib_obj == k3fbxObjType::CAMERA && fbx->camera && fbx_property_argument < 1) {
                            fbx->camera[fbx->num_cameras - 1].res_x = (uint32_t)*f32_arr;
                            fbx_property_argument++;
                        }
                        break;
                    case k3fbxProperty::ASPECT_HEIGHT:
                        if (fbx->node_attrib_obj == k3fbxObjType::CAMERA && fbx->camera && fbx_property_argument < 1) {
                            fbx->camera[fbx->num_cameras - 1].res_y = (uint32_t)*f32_arr;
                            fbx_property_argument++;
                        }
                        break;
                    case k3fbxProperty::FOVX:
                        if (fbx->node_attrib_obj == k3fbxObjType::CAMERA && fbx->camera && fbx_property_argument < 1) {
                            fbx->camera[fbx->num_cameras - 1].fovx = *f32_arr;
                            fbx_property_argument++;
                        }
                        break;
                    case k3fbxProperty::ORTHO_SCALE:
                        if (fbx->node_attrib_obj == k3fbxObjType::CAMERA && fbx->camera && fbx_property_argument < 1) {
                            fbx->camera[fbx->num_cameras - 1].ortho_scale = *f32_arr;
                            fbx_property_argument++;
                        }
                        break;
                    case k3fbxProperty::NEAR_PLANE:
                        if (fbx->node_attrib_obj == k3fbxObjType::CAMERA && fbx->camera && fbx_property_argument < 1) {
                            fbx->camera[fbx->num_cameras - 1].near_plane = *f32_arr;
                            fbx_property_argument++;
                        }
                        break;
                    case k3fbxProperty::FAR_PLANE:
                        if (fbx->node_attrib_obj == k3fbxObjType::CAMERA && fbx->camera && fbx_property_argument < 1) {
                            fbx->camera[fbx->num_cameras - 1].far_plane = *f32_arr;
                            fbx_property_argument++;
                        }
                        break;
                    case k3fbxProperty::COLOR:
                        if (fbx->node_attrib_obj == k3fbxObjType::LIGHT && fbx->light_node && fbx_property_argument < 3) {
                            fbx->light_node[fbx->num_light_nodes - 1].color[fbx_property_argument] = *f32_arr;
                            fbx_property_argument++;
                        }
                        break;
                    case k3fbxProperty::INTENSITY:
                        if (fbx->node_attrib_obj == k3fbxObjType::LIGHT && fbx->light_node && fbx_property_argument < 1) {
                            fbx->light_node[fbx->num_light_nodes - 1].intensity = *f32_arr;
                            fbx_property_argument++;
                        }
                        break;
                    case k3fbxProperty::DECAY_START:
                        if (fbx->node_attrib_obj == k3fbxObjType::LIGHT && fbx->light_node && fbx_property_argument < 1) {
                            fbx->light_node[fbx->num_light_nodes - 1].decay_start = *f32_arr;
                            fbx_property_argument++;
                        }
                        break;
                    case k3fbxProperty::D_X:
                        if (fbx->anim_curve_node && fbx_property_argument < 1) {
                            fbx->anim_curve_node[fbx->num_anim_curve_nodes - 1].default_value[0] = *f32_arr;
                            fbx_property_argument++;
                        }
                        break;
                    case k3fbxProperty::D_Y:
                        if (fbx->anim_curve_node && fbx_property_argument < 1) {
                            fbx->anim_curve_node[fbx->num_anim_curve_nodes - 1].default_value[1] = *f32_arr;
                            fbx_property_argument++;
                        }
                        break;
                    case k3fbxProperty::D_Z:
                        if (fbx->anim_curve_node && fbx_property_argument < 1) {
                            fbx->anim_curve_node[fbx->num_anim_curve_nodes - 1].default_value[2] = *f32_arr;
                            fbx_property_argument++;
                        }
                        break;
                    case k3fbxProperty::VISIBILITY:
                        if (fbx->model && fbx_property_argument < 1) {
                            fbx->model[fbx->num_models - 1].visibility = *f32_arr;
                            fbx_property_argument++;
                        }
                        break;
                    case k3fbxProperty::CUSTOM_PROP:
                        if (fbx->model && fbx_property_argument < 1) {
                            fbx->model_custom_prop[(fbx->num_models - 1) * custom_props->num_model_custom_props + fbx_custom_prop_index].f = *f32_arr;
                            fbx_property_argument++;
                        }
                        break;
                    }
                    break;
                }
                break;
            case K3_FBX_TYPECODE_DOUBLE:
                fread(d64_arr, sizeof(double), 1, in_file);
                if (K3_FBX_DEBUG) printf("D: %0.4f\n", *d64_arr);
                switch (node_type) {
                case k3fbxNodeType::POSITION:
                    if (fbx->node_attrib_obj == k3fbxObjType::CAMERA && fbx->camera && fbx_property_argument < 3) {
                        fbx->camera[fbx->num_cameras - 1].translation[fbx_property_argument] = (float)*d64_arr;
                        fbx_property_argument++;
                    }
                    break;
                case k3fbxNodeType::UP:
                    if (fbx->node_attrib_obj == k3fbxObjType::CAMERA && fbx->camera && fbx_property_argument < 3) {
                        fbx->camera[fbx->num_cameras - 1].up[fbx_property_argument] = (float)*d64_arr;
                        fbx_property_argument++;
                    }
                    break;
                case k3fbxNodeType::LOOK_AT:
                    if (fbx->node_attrib_obj == k3fbxObjType::CAMERA && fbx->camera && fbx_property_argument < 3) {
                        fbx->camera[fbx->num_cameras - 1].look_at[fbx_property_argument] = (float)*d64_arr;
                        fbx_property_argument++;
                    }
                    break;
                case k3fbxNodeType::PROP:
                    switch (fbx_property) {
                    case k3fbxProperty::LOCAL_TRANSLATION:
                        if (fbx->model && fbx_property_argument < 3) {
                            fbx->model[fbx->num_models - 1].translation[fbx_property_argument] = (float)*d64_arr;
                            fbx_property_argument++;
                        }
                        break;
                    case k3fbxProperty::LOCAL_ROTATION:
                        if (fbx->model && fbx_property_argument < 3) {
                            fbx->model[fbx->num_models - 1].rotation[fbx_property_argument] = (float)*d64_arr;
                            fbx_property_argument++;
                        }
                        break;
                    case k3fbxProperty::LOCAL_SCALING:
                        if (fbx->model && fbx_property_argument < 3) {
                            fbx->model[fbx->num_models - 1].scaling[fbx_property_argument] = (float)*d64_arr;
                            fbx_property_argument++;
                        }
                        break;
                    case k3fbxProperty::DIFFUSE_COLOR:
                        if (fbx->material && fbx_property_argument < 3) {
                            fbx->material[fbx->num_materials - 1].diffuse_color[fbx_property_argument] = (float)*d64_arr;
                            fbx_property_argument++;
                        }
                        break;
                    //case k3fbxProperty::POSITION:
                    //    if (fbx->node_attrib_obj == k3fbxObjType::CAMERA && fbx->camera && fbx_property_argument < 3) {
                    //        fbx->camera[fbx->num_cameras - 1].translation[fbx_property_argument] = (float)*d64_arr;
                    //        fbx_property_argument++;
                    //    }
                    //    break;
                    //case k3fbxProperty::UP_VECTOR:
                    //    if (fbx->node_attrib_obj == k3fbxObjType::CAMERA && fbx->camera && fbx_property_argument < 3) {
                    //        fbx->camera[fbx->num_cameras - 1].up[fbx_property_argument] = (float)*d64_arr;
                    //        fbx_property_argument++;
                    //    }
                    //    break;
                    //case k3fbxProperty::INTEREST_POSITION:
                    //    if (fbx->node_attrib_obj == k3fbxObjType::CAMERA && fbx->camera && fbx_property_argument < 3) {
                    //        fbx->camera[fbx->num_cameras - 1].look_at[fbx_property_argument] = (float)*d64_arr;
                    //        fbx_property_argument++;
                    //    }
                    //    break;
                    case k3fbxProperty::ASPECT_WIDTH:
                        if (fbx->node_attrib_obj == k3fbxObjType::CAMERA && fbx->camera && fbx_property_argument < 1) {
                            fbx->camera[fbx->num_cameras - 1].res_x = (uint32_t)*d64_arr;
                            fbx_property_argument++;
                        }
                        break;
                    case k3fbxProperty::ASPECT_HEIGHT:
                        if (fbx->node_attrib_obj == k3fbxObjType::CAMERA && fbx->camera && fbx_property_argument < 1) {
                            fbx->camera[fbx->num_cameras - 1].res_y = (uint32_t)*d64_arr;
                            fbx_property_argument++;
                        }
                        break;
                    case k3fbxProperty::FOVX:
                        if (fbx->node_attrib_obj == k3fbxObjType::CAMERA && fbx->camera && fbx_property_argument < 1) {
                            fbx->camera[fbx->num_cameras - 1].fovx = (float)*d64_arr;
                            fbx_property_argument++;
                        }
                        break;
                    case k3fbxProperty::ORTHO_SCALE:
                        if (fbx->node_attrib_obj == k3fbxObjType::CAMERA && fbx->camera && fbx_property_argument < 1) {
                            fbx->camera[fbx->num_cameras - 1].ortho_scale = (float)*d64_arr;
                            fbx_property_argument++;
                        }
                        break;
                    case k3fbxProperty::NEAR_PLANE:
                        if (fbx->node_attrib_obj == k3fbxObjType::CAMERA && fbx->camera && fbx_property_argument < 1) {
                            fbx->camera[fbx->num_cameras - 1].near_plane = (float)*d64_arr;
                            fbx_property_argument++;
                        }
                        break;
                    case k3fbxProperty::FAR_PLANE:
                        if (fbx->node_attrib_obj == k3fbxObjType::CAMERA && fbx->camera && fbx_property_argument < 1) {
                            fbx->camera[fbx->num_cameras - 1].far_plane = (float)*d64_arr;
                            fbx_property_argument++;
                        }
                        break;
                    case k3fbxProperty::COLOR:
                        if (fbx->node_attrib_obj == k3fbxObjType::LIGHT && fbx->light_node && fbx_property_argument < 3) {
                            fbx->light_node[fbx->num_light_nodes - 1].color[fbx_property_argument] = (float)*d64_arr;
                            fbx_property_argument++;
                        }
                        break;
                    case k3fbxProperty::INTENSITY:
                        if (fbx->node_attrib_obj == k3fbxObjType::LIGHT && fbx->light_node && fbx_property_argument < 1) {
                            fbx->light_node[fbx->num_light_nodes - 1].intensity = (float)*d64_arr;
                            fbx_property_argument++;
                        }
                        break;
                    case k3fbxProperty::DECAY_START:
                        if (fbx->node_attrib_obj == k3fbxObjType::LIGHT && fbx->light_node && fbx_property_argument < 1) {
                            fbx->light_node[fbx->num_light_nodes - 1].decay_start = (float)*d64_arr;
                            fbx_property_argument++;
                        }
                        break;
                    case k3fbxProperty::D_X:
                        if (fbx->anim_curve_node && fbx_property_argument < 1) {
                            fbx->anim_curve_node[fbx->num_anim_curve_nodes - 1].default_value[0] = (float)*d64_arr;
                            fbx_property_argument++;
                        }
                        break;
                    case k3fbxProperty::D_Y:
                        if (fbx->anim_curve_node && fbx_property_argument < 1) {
                            fbx->anim_curve_node[fbx->num_anim_curve_nodes - 1].default_value[1] = (float)*d64_arr;
                            fbx_property_argument++;
                        }
                        break;
                    case k3fbxProperty::D_Z:
                        if (fbx->anim_curve_node && fbx_property_argument < 1) {
                            fbx->anim_curve_node[fbx->num_anim_curve_nodes - 1].default_value[2] = (float)*d64_arr;
                            fbx_property_argument++;
                        }
                        break;
                    case k3fbxProperty::VISIBILITY:
                        if (fbx->model && fbx_property_argument < 1) {
                            fbx->model[fbx->num_models - 1].visibility = (float)*d64_arr;
                            fbx_property_argument++;
                        }
                        break;
                    }
                    break;
                }
                break;
            case K3_FBX_TYPECODE_SINT64:
                fread(i64_arr, sizeof(int64_t), 1, in_file);
                if (K3_FBX_DEBUG) printf("L: %lld\n", *i64_arr);
                switch (node_type) {
                case k3fbxNodeType::GEOMETRY:
                    if (fbx_node_argument == 0) {
                        if (fbx->mesh) fbx->mesh[fbx->num_meshes - 1].id = *i64_arr;
                        fbx_node_argument++;
                    }
                    break;
                case k3fbxNodeType::MODEL:
                    if (fbx_node_argument == 0) {
                        if (fbx->model) fbx->model[fbx->num_models - 1].id = *i64_arr;
                        fbx_node_argument++;
                    }
                    break;
                case k3fbxNodeType::MATERIAL:
                    if (fbx_node_argument == 0) {
                        if (fbx->material) fbx->material[fbx->num_materials - 1].id = *i64_arr;
                        fbx_node_argument++;
                    }
                    break;
                case k3fbxNodeType::TEXTURE:
                    if (fbx_node_argument == 0) {
                        if (fbx->texture) fbx->texture[fbx->num_textures - 1].id = *i64_arr;
                        fbx_node_argument++;
                    }
                    break;
                case k3fbxNodeType::VIDEO:
                    if (fbx_node_argument == 0) {
                        if (fbx->content_data) fbx->content_data[fbx->num_content_data - 1].id = *i64_arr;
                        fbx_node_argument++;
                    }
                    break;
                case k3fbxNodeType::NODE_ATTRIBUTE:
                case k3fbxNodeType::DEFORMER:
                    if (fbx_node_argument == 0) {
                        node_attrib_id = *i64_arr;
                        fbx_node_argument++;
                    }
                    break;
                case k3fbxNodeType::CONNECT:
                    if (connect_index < 2) {
                        connect_id[connect_index] = *i64_arr;
                        connect_index++;
                        if (connect_index == connect_params) connectFbxNode(fbx, connect_id);
                    }
                    break;
                case k3fbxNodeType::ANIMATION_LAYER:
                    if (fbx_node_argument == 0) {
                        if (fbx->anim_layer) fbx->anim_layer[fbx->num_anim_layers - 1].id = *i64_arr;
                        fbx_node_argument++;
                    }
                    break;
                case k3fbxNodeType::ANIMATION_CURVE_NODE:
                    if (fbx_node_argument == 0) {
                        if (fbx->anim_curve_node) fbx->anim_curve_node[fbx->num_anim_curve_nodes - 1].id = *i64_arr;
                        fbx_node_argument++;
                    }
                    break;
                case k3fbxNodeType::ANIMATION_CURVE:
                    if (fbx_node_argument == 0) {
                        if (fbx->anim_curve) fbx->anim_curve[fbx->num_anim_curves - 1].id = *i64_arr;
                        fbx_node_argument++;
                    }
                    break;
                case k3fbxNodeType::POSE:
                    if (fbx_node_argument == 0) {
                        fbx_node_argument++;
                    }
                    break;
                case k3fbxNodeType::NODE:
                    if (fbx_node_argument == 0 && parent_node == k3fbxNodeType::POSE_NODE && fbx->pose_type == k3fbxPoseType::BindPose) {
                        if (fbx->bind_pose_node) fbx->bind_pose_node[fbx->num_bind_pose_nodes - 1].model_id = *i64_arr;
                        fbx_node_argument++;
                    }
                }
                break;
            case K3_FBX_TYPECODE_FLOAT_ARRAY:
            case K3_FBX_TYPECODE_DOUBLE_ARRAY:
            case K3_FBX_TYPECODE_SINT64_ARRAY:
            case K3_FBX_TYPECODE_SINT32_ARRAY:
            case K3_FBX_TYPECODE_BOOL_ARRAY:
                fread(&arr_prop, K3_FBX_ARRAY_PROPERTY_LENGTH, 1, in_file);
                uncomp_bytes_remaining = arr_prop.array_length;
                switch (typecode) {
                case K3_FBX_TYPECODE_FLOAT_ARRAY:
                    uncomp_bytes_remaining *= sizeof(float);
                    break;
                case K3_FBX_TYPECODE_DOUBLE_ARRAY:
                    uncomp_bytes_remaining *= sizeof(double);
                    break;
                case K3_FBX_TYPECODE_SINT64_ARRAY:
                    uncomp_bytes_remaining *= sizeof(int64_t);
                    break;
                case K3_FBX_TYPECODE_SINT32_ARRAY:
                    uncomp_bytes_remaining *= sizeof(int32_t);
                    break;
                }
                if (arr_prop.encoding) {
                    bytes_remaining = arr_prop.compressed_length;
                } else {
                    bytes_remaining = uncomp_bytes_remaining;
                }
                data_arr = NULL;
                switch (node_type) {
                case k3fbxNodeType::VERTICES:
                    fbx->vert_type = typecode;
                    if (fbx->vertices) data_arr = (char*)fbx->vertices + fbx->num_vertex_bytes;
                    fbx->num_vertex_bytes += ((typecode == 'd') ? 8 : 4) * arr_prop.array_length;
                    fbx->num_vertices += (arr_prop.array_length) / 3;
                    break;
                case k3fbxNodeType::POLYGON_VERT_INDEX:
                    if (fbx->indices) data_arr = fbx->indices + fbx->num_indices;
                    fbx->num_indices += arr_prop.array_length;
                    break;
                case k3fbxNodeType::NORMAL_INDEX:
                    if (fbx->last_normal_reference == k3fbxReference::ByIndex) {
                        if (fbx->normal_indices) data_arr = fbx->normal_indices + fbx->num_normal_indices;
                        fbx->num_normal_indices += arr_prop.array_length;
                    }
                    break;
                case k3fbxNodeType::TANGENT_INDEX:
                    if (fbx->last_tangent_reference == k3fbxReference::ByIndex) {
                        if (fbx->tangent_indices) data_arr = fbx->tangent_indices + fbx->num_tangent_indices;
                        fbx->num_tangent_indices += arr_prop.array_length;
                    }
                    break;
                case k3fbxNodeType::UV_INDEX:
                    if (fbx->last_uv_reference == k3fbxReference::ByIndex) {
                        if (fbx->uv_indices) data_arr = fbx->uv_indices + fbx->num_uv_indices;
                        fbx->num_uv_indices += arr_prop.array_length;
                    }
                    break;
                case k3fbxNodeType::NORMALS:
                    fbx->norm_type = typecode;
                    if (fbx->normals) data_arr = (char*)fbx->normals + fbx->num_normal_bytes;
                    fbx->num_normal_bytes += ((typecode == 'd') ? 8 : 4) * arr_prop.array_length;
                    fbx->num_normals += (arr_prop.array_length) / 3;
                    break;
                case k3fbxNodeType::TANGENTS:
                    fbx->tang_type = typecode;
                    if (fbx->tangents) data_arr = (char*)fbx->tangents + fbx->num_tangent_bytes;
                    fbx->num_tangent_bytes += ((typecode == 'd') ? 8 : 4) * arr_prop.array_length;
                    fbx->num_tangents += (arr_prop.array_length) / 3;
                    break;
                case k3fbxNodeType::UV:
                    fbx->uv_type = typecode;
                    if (fbx->uvs) data_arr = (char*)fbx->uvs + fbx->num_uv_bytes;
                    fbx->num_uv_bytes += ((typecode == 'd') ? 8 : 4) * arr_prop.array_length;
                    fbx->num_uvs += (arr_prop.array_length) / 2;
                    break;
                case k3fbxNodeType::MATERIAL_IDS:
                    if (fbx->material_ids) data_arr = fbx->material_ids + fbx->num_material_ids;
                    fbx->num_material_ids += arr_prop.array_length;
                    break;
                case k3fbxNodeType::INDEXES:
                    if (parent_node == k3fbxNodeType::DEFORMER && fbx->deformer_type == k3fbxDeformer::Cluster) {
                        if (fbx->cluster_indexes) data_arr = fbx->cluster_indexes + fbx->num_cluster_indexes;
                        fbx->num_cluster_indexes += arr_prop.array_length;
                    }
                    break;
                case k3fbxNodeType::WEIGHTS:
                    if (parent_node == k3fbxNodeType::DEFORMER && fbx->deformer_type == k3fbxDeformer::Cluster) {
                        if (fbx->cluster_weights) data_arr = (char*)fbx->cluster_weights + fbx->num_cluster_weight_bytes;
                        fbx->num_cluster_weight_bytes += ((typecode == 'd') ? 8 : 4) * arr_prop.array_length;
                        fbx->num_cluster_weights += arr_prop.array_length;
                        if (fbx->cluster) {
                            fbx->cluster[fbx->num_clusters - 1].weight_type = typecode;
                        }
                    }
                    break;
                case k3fbxNodeType::KEY_TIME:
                    if (parent_node == k3fbxNodeType::ANIMATION_CURVE) {
                        if (fbx->anim_curve_time) data_arr = fbx->anim_curve_time + fbx->num_anim_curve_time_elements;
                        if(fbx->anim_curve) {
                            if (typecode != K3_FBX_TYPECODE_SINT64_ARRAY) {
                                k3error::Handler("Invalid key time type array", "readFbxNode");
                            }
                            fbx->anim_curve[fbx->num_anim_curves - 1].time_start = fbx->num_anim_curve_time_elements;
                            fbx->anim_curve[fbx->num_anim_curves - 1].num_key_frames = arr_prop.array_length;
                        }
                        fbx->num_anim_curve_time_elements += arr_prop.array_length;
                    }
                    break;
                case k3fbxNodeType::KEY_VALUE_FLOAT:
                    if (parent_node == k3fbxNodeType::ANIMATION_CURVE) {
                        if (fbx->anim_curve_data) data_arr = fbx->anim_curve_data + fbx->num_anim_curve_data_elements;
                        if (fbx->anim_curve) {
                            if (typecode != K3_FBX_TYPECODE_FLOAT_ARRAY) {
                                k3error::Handler("Invalid key value type array", "readFbxNode");
                            }
                            fbx->anim_curve[fbx->num_anim_curves - 1].data_start = fbx->num_anim_curve_data_elements;
                        }
                        fbx->num_anim_curve_data_elements += arr_prop.array_length;
                    }
                    break;
                case k3fbxNodeType::MATRIX:
                    if (parent_node == k3fbxNodeType::POSE_NODE && fbx->pose_type == k3fbxPoseType::BindPose) {
                        if (fbx->bind_pose_node) {
                            if (typecode == 'd') {
                                data_arr = mat_d;
                            } else {
                                data_arr = fbx->bind_pose_node[fbx->num_bind_pose_nodes - 1].xform;
                            }
                        }
                        if (arr_prop.array_length > 16) {
                            k3error::Handler("Matrix length too long", "readFbxNode");
                        }
                    }
                    break;
                }
                if (data_arr == NULL) {
                    fseek(in_file, bytes_remaining, SEEK_CUR);
                } else if (arr_prop.encoding) {
                    zs.zalloc = Z_NULL;
                    zs.zfree = Z_NULL;
                    zs.opaque = Z_NULL;
                    inflateInit(&zs);
                    zs.avail_out = uncomp_bytes_remaining;
                    zs.next_out = (Bytef*)data_arr;
                    while (bytes_remaining) {
                        zs.avail_in = (bytes_remaining > 256) ? 256 : bytes_remaining;
                        zs.next_in = (Bytef*)str;
                        fread(str, 1, zs.avail_in, in_file);
                        bytes_remaining -= zs.avail_in;
                        inflate(&zs, Z_NO_FLUSH);
                    }
                    inflateEnd(&zs);
                    //uint32_t end_comp_block = ftell(in_file) + arr_prop.compressed_length;
                    //readZBlock(str, in_file);
                    //fseek(in_file, end_comp_block, SEEK_SET);
                } else {
                    fread(data_arr, 1, bytes_remaining, in_file);
                }
                // if a matrix is a double, convert to float, and copy it to the destination
                if (node_type == k3fbxNodeType::MATRIX &&
                    parent_node == k3fbxNodeType::POSE_NODE &&
                    fbx->pose_type == k3fbxPoseType::BindPose &&
                    fbx->bind_pose_node && typecode == 'd') {
                    doubleToFloatArray(mat_d, arr_prop.array_length);
                    memcpy(fbx->bind_pose_node[fbx->num_bind_pose_nodes - 1].xform, mat_d, 16 * sizeof(float));
                }
                break;
            case K3_FBX_TYPECODE_STRING:
            case K3_FBX_TYPECODE_RAW:
                fread(&bytes_remaining, sizeof(uint32_t), 1, in_file);
                str[0] = '\0';
                str_file_pos = ftell(in_file);
                while (bytes_remaining) {
                    if (bytes_remaining >= 256) {
                        //fread(str, 1, 256, in_file);
                        //bytes_remaining -= 256;
                        fseek(in_file, bytes_remaining, SEEK_CUR);
                        bytes_remaining = 0;
                    } else {
                        fread(str, 1, bytes_remaining, in_file);
                        str[bytes_remaining] = '\0';
                        bytes_remaining = 0;
                    }
                }
                if (K3_FBX_DEBUG) {
                    printf("%s\n", str);
                }
                if (node_type == k3fbxNodeType::MAPPING_TYPE) {
                    k3fbxMapping mapping = k3fbxMapping::ByVert;
                    if (!strncmp(str, "ByPolygon", 10)) mapping = k3fbxMapping::ByPoly;
                    else if (!strncmp(str, "ByPolygonVertex", 16)) mapping = k3fbxMapping::ByPolyVert;
                    else if (!strncmp(str, "ByVertex", 9)) mapping = k3fbxMapping::ByVert;
                    else if (!strncmp(str, "ByVertice", 10)) mapping = k3fbxMapping::ByVert;
                    else if (!strncmp(str, "ByEdge", 7)) mapping = k3fbxMapping::ByEdge;
                    else if (!strncmp(str, "AllSame", 8)) mapping = k3fbxMapping::AllSame;
                    if (parent_node == k3fbxNodeType::LAYER_ELEMENT_NORMAL) {
                        if (fbx->mesh) fbx->mesh[fbx->num_meshes - 1].normal_mapping = mapping;
                    } else if (parent_node == k3fbxNodeType::LAYER_ELEMENT_TANGENT) {
                        if (fbx->mesh) fbx->mesh[fbx->num_meshes - 1].tangent_mapping = mapping;
                    } else if (parent_node == k3fbxNodeType::LAYER_ELEMENT_UV) {
                        if(fbx->mesh) fbx->mesh[fbx->num_meshes - 1].uv_mapping = mapping;
                    } else if (parent_node == k3fbxNodeType::LAYER_ELEMENT_MATERIAL) {
                        if (fbx->mesh) fbx->mesh[fbx->num_meshes - 1].material_mapping = mapping;
                    }
                } else if (node_type == k3fbxNodeType::REFERENCE_TYPE) {
                    k3fbxReference reference = k3fbxReference::Direct;
                    if (!strncmp(str, "Direct", 7)) reference = k3fbxReference::Direct;
                    else if (!strncmp(str, "IndexToDirect", 14)) reference = k3fbxReference::ByIndex;
                    if (parent_node == k3fbxNodeType::LAYER_ELEMENT_NORMAL) {
                        fbx->last_normal_reference = reference;
                        if (fbx->num_meshes && fbx->mesh) fbx->mesh[fbx->num_meshes - 1].normal_reference = reference;
                    } else if(parent_node == k3fbxNodeType::LAYER_ELEMENT_TANGENT) {
                        fbx->last_tangent_reference = reference;
                        if (fbx->num_meshes && fbx->mesh) fbx->mesh[fbx->num_meshes - 1].tangent_reference = reference;
                    } else if (parent_node == k3fbxNodeType::LAYER_ELEMENT_UV) {
                        fbx->last_uv_reference = reference;
                        if (fbx->num_meshes && fbx->mesh) fbx->mesh[fbx->num_meshes - 1].uv_reference = reference;
                    }
                } else if (node_type == k3fbxNodeType::RELATIVE_FILE_NAME) {
                    if (parent_node == k3fbxNodeType::TEXTURE && fbx->texture) {
                        strncpy(fbx->texture[fbx->num_textures - 1].filename, str, K3_FBX_FILENAME_SIZE);
                    } else if (parent_node == k3fbxNodeType::VIDEO && fbx->content_data) {
                        strncpy(fbx->content_data[fbx->num_content_data - 1].filename, str, K3_FBX_FILENAME_SIZE);
                    }
                } else if (node_type == k3fbxNodeType::PROP) {
                    if (fbx_property == k3fbxProperty::NONE) {
                        if (!strncmp(str, "Lcl Translation", 16)) {
                            fbx_property = k3fbxProperty::LOCAL_TRANSLATION;
                        } else if (!strncmp(str, "Lcl Rotation", 13)) {
                            fbx_property = k3fbxProperty::LOCAL_ROTATION;
                        } else if (!strncmp(str, "Lcl Scaling", 12)) {
                            fbx_property = k3fbxProperty::LOCAL_SCALING;
                        } else if (!strncmp(str, "DiffuseColor", 13)) {
                            fbx_property = k3fbxProperty::DIFFUSE_COLOR;
                        } else if (!strncmp(str, "Position", 9)) {
                            fbx_property = k3fbxProperty::POSITION;
                        } else if (!strncmp(str, "UpVector", 9)) {
                            fbx_property = k3fbxProperty::UP_VECTOR;
                        } else if (!strncmp(str, "InterestPosition", 17)) {
                            fbx_property = k3fbxProperty::INTEREST_POSITION;
                        } else if (!strncmp(str, "AspectWidth", 12)) {
                            fbx_property = k3fbxProperty::ASPECT_WIDTH;
                        } else if (!strncmp(str, "AspectHeight", 13)) {
                            fbx_property = k3fbxProperty::ASPECT_HEIGHT;
                        } else if (!strncmp(str, "FieldOfViewX", 13)) {
                            fbx_property = k3fbxProperty::FOVX;
                        } else if (!strncmp(str, "CameraProjectionType", 21)) {
                            fbx_property = k3fbxProperty::PROJ_TYPE;
                        } else if (!strncmp(str, "OrthoZoom", 10)) {
                            fbx_property = k3fbxProperty::ORTHO_SCALE;
                        } else if (!strncmp(str, "NearPlane", 10)) {
                            fbx_property = k3fbxProperty::NEAR_PLANE;
                        } else if (!strncmp(str, "FarPlane", 9)) {
                            fbx_property = k3fbxProperty::FAR_PLANE;
                        } else if (!strncmp(str, "LightType", 10)) {
                            fbx_property = k3fbxProperty::LIGHT_TYPE;
                        } else if (!strncmp(str, "Color", 6)) {
                            fbx_property = k3fbxProperty::COLOR;
                        } else if (!strncmp(str, "Intensity", 10)) {
                            fbx_property = k3fbxProperty::INTENSITY;
                        } else if (!strncmp(str, "DecayType", 10)) {
                            fbx_property = k3fbxProperty::DECAY_TYPE;
                        } else if (!strncmp(str, "DecayStart", 11)) {
                            fbx_property = k3fbxProperty::DECAY_START;
                        } else if (!strncmp(str, "CastShadows", 12)) {
                            fbx_property = k3fbxProperty::CAST_SHADOWS;
                        } else if (!strncmp(str, "d|X", 4)) {
                            fbx_property = k3fbxProperty::D_X;
                        } else if (!strncmp(str, "d|Y", 4)) {
                            fbx_property = k3fbxProperty::D_Y;
                        } else if (!strncmp(str, "d|Z", 4)) {
                            fbx_property = k3fbxProperty::D_Z;
                        } else if (!strncmp(str, "Visibility", 11)) {
                            fbx_property = k3fbxProperty::VISIBILITY;
                        } else {
                            uint32_t i;
                            fbx_property = k3fbxProperty::NONE;
                            for (i = 0; i < custom_props->num_model_custom_props && fbx_property != k3fbxProperty::CUSTOM_PROP; i++) {
                                if (!strncmp(str, custom_props->model_custom_prop_name[i], K3_FBX_MAX_NAME_LENGTH)) {
                                    fbx_property = k3fbxProperty::CUSTOM_PROP;
                                    fbx_custom_prop_index = i;
                                }
                            }
                            if (K3_FBX_DEBUG) {
                                printf("%s\n", str);
                            }
                        }
                    }
                } else if (node_type == k3fbxNodeType::CONNECT) {
                    if (connect_params == 0) {
                        connect_params = (str[1] == 'P') ? 3 : 2;
                    } else {
                        if (!strncmp(str, "DiffuseColor", 13)) {
                            connect_id[connect_index] = CONNECT_TYPE_DIFFUSE_MAP;
                        } else if (!strncmp(str, "NormalMap", 10)) {
                            connect_id[connect_index] = CONNECT_TYPE_NORMAL_MAP;
                        } else if (!strncmp(str, "d|X", 4)) {
                            connect_id[connect_index] = CONNECT_TYPE_X_AXIS;
                        } else if (!strncmp(str, "d|Y", 4)) {
                            connect_id[connect_index] = CONNECT_TYPE_Y_AXIS;
                        } else if (!strncmp(str, "d|Z", 4)) {
                            connect_id[connect_index] = CONNECT_TYPE_Z_AXIS;
                        } else {
                            connect_id[connect_index] = CONNECT_TYPE_UNKNOWN;
                        }
                        connect_index++;
                        if (connect_index == connect_params) connectFbxNode(fbx, connect_id);
                    }
                } else if(node_type == k3fbxNodeType::NODE_ATTRIBUTE) {
                    if (fbx_node_argument == 1) {
                        strncpy(node_attr_name, str, K3_FBX_MAX_NAME_LENGTH);
                    } else if (fbx_node_argument == 2) {
                        // third argument is the type
                        if (!strncmp(str, "Camera", 7)) {
                            if (fbx->camera) {
                                fbx->camera[fbx->num_cameras].id = node_attrib_id;
                                strncpy(fbx->camera[fbx->num_cameras].name, node_attr_name, K3_FBX_MAX_NAME_LENGTH);
                                fbx->camera[fbx->num_cameras].translation[0] = 1.0f;
                                fbx->camera[fbx->num_cameras].translation[1] = 1.0f;
                                fbx->camera[fbx->num_cameras].translation[2] = 1.0f;
                                fbx->camera[fbx->num_cameras].look_at[0] = 0.0f;
                                fbx->camera[fbx->num_cameras].look_at[1] = 0.0f;
                                fbx->camera[fbx->num_cameras].look_at[2] = 0.0f;
                                fbx->camera[fbx->num_cameras].up[0] = 0.0f;
                                fbx->camera[fbx->num_cameras].up[1] = 0.0f;
                                fbx->camera[fbx->num_cameras].up[2] = 1.0f;
                                fbx->camera[fbx->num_cameras].fovx = 40.0f;
                                fbx->camera[fbx->num_cameras].ortho_scale = 1.0f;
                                fbx->camera[fbx->num_cameras].proj_type = K3_FBX_PROJECTION_PERSPECTIVE;
                                fbx->camera[fbx->num_cameras].res_x = 640;
                                fbx->camera[fbx->num_cameras].res_y = 480;
                                fbx->camera[fbx->num_cameras].near_plane = 0.1f;
                                fbx->camera[fbx->num_cameras].far_plane = 1000.0f;
                            }
                            fbx->num_cameras++;
                            fbx->node_attrib_obj = k3fbxObjType::CAMERA;
                        } else if (!strncmp(str, "Light", 6)) {
                            if (fbx->light_node) {
                                fbx->light_node[fbx->num_light_nodes].id = node_attrib_id;
                                strncpy(fbx->light_node[fbx->num_light_nodes].name, node_attr_name, K3_FBX_MAX_NAME_LENGTH);
                                fbx->light_node[fbx->num_light_nodes].color[0] = 1.0f;
                                fbx->light_node[fbx->num_light_nodes].color[1] = 1.0f;
                                fbx->light_node[fbx->num_light_nodes].color[2] = 1.0f;
                                fbx->light_node[fbx->num_light_nodes].intensity = 1000.0f;
                                fbx->light_node[fbx->num_light_nodes].light_type = k3lightBufferData::POINT;
                                fbx->light_node[fbx->num_light_nodes].decay_start = 0.0f;
                                fbx->light_node[fbx->num_light_nodes].decay_type = k3lightBufferData::DECAY_QUADRATIC;
                                fbx->light_node[fbx->num_light_nodes].cast_shadows = false;
                            }
                            fbx->num_light_nodes++;
                            fbx->node_attrib_obj = k3fbxObjType::LIGHT;
                        }
                    }
                } else if( node_type == k3fbxNodeType::CONTENT) {
                    if (parent_node == k3fbxNodeType::VIDEO && fbx->content_data) {
                        fbx->content_data[fbx->num_content_data - 1].file_pos = str_file_pos;
                    }
                } else if (node_type == k3fbxNodeType::MODEL) {
                    if (fbx_node_argument == 1) {
                        if (fbx->model) {
                            strncpy(fbx->model[fbx->num_models - 1].name, str, K3_FBX_MAX_NAME_LENGTH);
                        }
                    } else if (fbx_node_argument == 2) {
                        if (!strncmp(str, "Light", 6)) {
                            fbx->num_lights++;
                            if (fbx->model) fbx->model[fbx->num_models - 1].obj_type = k3fbxObjType::LIGHT;
                        } else if (!strncmp(str, "Camera", 7)) {
                            if (fbx->model) fbx->model[fbx->num_models - 1].obj_type = k3fbxObjType::CAMERA;
                        } else if (!strncmp(str, "Mesh", 5)) {
                            if (fbx->model) fbx->model[fbx->num_models - 1].obj_type = k3fbxObjType::MESH;
                        } else if (!strncmp(str, "LimbNode", 9)) {
                            if (fbx->model) {
                                fbx->model[fbx->num_models - 1].obj_type = k3fbxObjType::LIMB_NODE;
                                fbx->model[fbx->num_models - 1].index.limb_node.parent = ~0x0;  // initialize parent to all 1's, indicating the root
                                fbx->model[fbx->num_models - 1].index.limb_node.cluster_index = ~0x0;
                                fbx->model[fbx->num_models - 1].index.limb_node.bind_pose_index = ~0x0;
                            }
                        } else {
                            if (fbx->model) fbx->model[fbx->num_models - 1].obj_type = k3fbxObjType::NONE;
                        }
                    }
                } else if (node_type == k3fbxNodeType::DEFORMER) {
                    if (fbx_node_argument == 2) {
                        if (!strncmp(str, "Cluster", 8)) {
                            if (fbx->cluster) {
                                fbx->cluster[fbx->num_clusters].id = node_attrib_id;
                                fbx->cluster[fbx->num_clusters].index_start = fbx->num_cluster_indexes;
                                fbx->cluster[fbx->num_clusters].weight_start = fbx->num_cluster_weight_bytes;
                            }
                            fbx->num_clusters++;
                            fbx->deformer_type = k3fbxDeformer::Cluster;
                        } else if (!strncmp(str, "Skin", 5)) {
                            if (fbx->skin) {
                                fbx->skin[fbx->num_skins].id = node_attrib_id;
                                fbx->skin[fbx->num_skins].mesh_index = ~0;
                            }
                            fbx->num_skins++;
                            fbx->deformer_type = k3fbxDeformer::Skin;
                        } else {
                            fbx->deformer_type = k3fbxDeformer::None;
                        }
                    }
                } else if (node_type == k3fbxNodeType::POSE) {
                    if (fbx_node_argument == 2) {
                        if (!strncmp(str, "BindPose", 9)) {
                            fbx->pose_type = k3fbxPoseType::BindPose;
                        } else {
                            fbx->pose_type = k3fbxPoseType::None;
                        }
                    }
                } else if (node_type == k3fbxNodeType::ANIMATION_LAYER) {
                    if (fbx_node_argument == 1 && fbx->anim_layer) {
                        // Find the last instance of '|'
                        char* anim_name = strrchr(str, '|');
                        // if found, use the characters following it; otherwise, sue the whole name
                        anim_name = (anim_name) ? (anim_name + 1) : str;
                        strncpy(fbx->anim_layer[fbx->num_anim_layers - 1].name, anim_name, K3_FBX_MAX_NAME_LENGTH);
                    }
                } else if (node_type == k3fbxNodeType::ANIMATION_CURVE_NODE) {
                    if (fbx_node_argument == 1 && fbx->anim_curve_node) {
                        switch (str[0]) {
                        case 'T': fbx->anim_curve_node[fbx->num_anim_curve_nodes - 1].curve_type = k3fbxAnimCurveType::Translation; break;
                        case 'S': fbx->anim_curve_node[fbx->num_anim_curve_nodes - 1].curve_type = k3fbxAnimCurveType::Scaling; break;
                        case 'R': fbx->anim_curve_node[fbx->num_anim_curve_nodes - 1].curve_type = k3fbxAnimCurveType::Rotation; break;
                        default: fbx->anim_curve_node[fbx->num_anim_curve_nodes - 1].curve_type = k3fbxAnimCurveType::None; break;
                        }
                    }
                }
                if (fbx_node_argument) fbx_node_argument++;
                break;
            }
        }

        uint32_t byte_pos = ftell(in_file);
        if (node.end_offset - byte_pos > 0) {
            readFbxNode(fbx, node_type, level + 1, in_file, custom_props);
            fseek(in_file, node.end_offset, SEEK_SET);
        }
    }
    if (parent_node == k3fbxNodeType::UNKNOWN) {
        uint32_t model_index;
        if (fbx->bind_pose_node) {
            for (i = 0; i < fbx->num_bind_pose_nodes; i++) {
                model_index = findFbxModelNode(fbx, fbx->bind_pose_node[i].model_id);
                if (fbx->model[model_index].obj_type == k3fbxObjType::LIMB_NODE) {
                    fbx->model[model_index].index.limb_node.bind_pose_index = i;
                }
            }
        }
    }
}

// Sort mesh models; list must be large enogh to contain all models
void insertFbxModelToSortedList(uint32_t model_id, k3fbxData* fbx, uint32_t* sorted_model_id)
{
    if (model_id >= fbx->num_models) return;
    if (sorted_model_id[model_id] != ~0x0) return;
    if (fbx->model[model_id].obj_type != k3fbxObjType::MESH) return;
    uint32_t parent_model_id = fbx->model[model_id].index.model.parent;
    if (parent_model_id < fbx->num_models) {
        // Make sure parent model has been sorted
        insertFbxModelToSortedList(parent_model_id, fbx, sorted_model_id);
        // insert child just after parent
        sorted_model_id[model_id] = sorted_model_id[parent_model_id] + 1;
    } else {
        // if there is no parent, insert t first location
        sorted_model_id[model_id] = 0;
    }
    uint32_t m;
    for (m = 0; m < fbx->num_models; m++) {
        if (sorted_model_id[m] != ~0x0 && sorted_model_id[m] >= sorted_model_id[model_id] && m != model_id) {
            sorted_model_id[m]++;
        }
    }
}

void insertFbxBoneToSortedList(uint32_t bone_id, k3fbxData* fbx, uint32_t* sorted_bone_id)
{
    if (bone_id >= fbx->num_clusters) return;
    if (sorted_bone_id[bone_id] != ~0x0) return;
    uint32_t bone_model_index = fbx->cluster[bone_id].bone_index;
    uint32_t parent_bone_index = fbx->model[bone_model_index].index.limb_node.parent;
    if (parent_bone_index < fbx->num_models) {
        uint32_t parent_bone_id = fbx->model[parent_bone_index].index.limb_node.cluster_index;
        // Make sure parent has been sorted
        insertFbxBoneToSortedList(parent_bone_id, fbx, sorted_bone_id);
        // insert child just after parent
        sorted_bone_id[bone_id] = sorted_bone_id[parent_bone_id] + 1;
    } else {
        // if there is no parent, insert to first location
        sorted_bone_id[bone_id] = 0;
    }
    uint32_t b;
    for (b = 0; b < fbx->num_clusters; b++) {
        if (sorted_bone_id[b] != ~0x0 && sorted_bone_id[b] >= sorted_bone_id[bone_id] && b != bone_id) {
            sorted_bone_id[b]++;
        }
    }
}

K3API k3mesh k3gfxObj::CreateMesh(k3meshDesc* desc)
{
    FILE* in_file;
    fopen_s(&in_file, desc->name, "rb");
    if (in_file == NULL) {
        k3error::Handler("Could not load mesh file", "k3gfxObj::CreateMesh");
        return NULL;
    }

    char str[K3_FBX_KEYWORD_LEN] = { 0 };
    uint16_t keyword_tail;
    fread(str, K3_FBX_KEYWORD_LEN, 1, in_file);
    fread(&keyword_tail, sizeof(uint16_t), 1, in_file);
    if (strncmp(str, K3_FBX_KEYWORD, K3_FBX_KEYWORD_LEN) || keyword_tail != K3_FBX_KEYWORD_TAIL) {
        fclose(in_file);
        k3error::Handler("Mesh unknown file format", "k3gfxObj::CreateMesh");
        return NULL;
    }

    uint32_t version;
    fread(&version, sizeof(uint32_t), 1, in_file);
    if (K3_FBX_DEBUG) printf("FBX version: %1.2f\n", version / 1000.0f);

    k3fbxData fbx = { 0 };
    uint32_t node_start = ftell(in_file);
    k3fbxCustomPropDesc custom_props;
    custom_props.num_model_custom_props = desc->num_custom_model_props;
    custom_props.model_custom_prop_name = desc->custom_model_prop_name;
    readFbxNode(&fbx, k3fbxNodeType::UNKNOWN, 0, in_file, &custom_props);
    fbx.mesh = new k3fbxMeshData[fbx.num_meshes];
    fbx.model = new k3fbxModelData[fbx.num_models];
    if (custom_props.num_model_custom_props) {
        fbx.model_custom_prop = new k3flint32[fbx.num_models * custom_props.num_model_custom_props];
        memset(fbx.model_custom_prop, 0, fbx.num_models * custom_props.num_model_custom_props * sizeof(k3flint32));
    }
    if (fbx.num_materials) fbx.material = new k3fbxMaterialData[fbx.num_materials];
    if (fbx.num_textures) fbx.texture = new k3fbxTextureData[fbx.num_textures];
    if (fbx.num_content_data) fbx.content_data = new k3fbxContentData[fbx.num_content_data];
    fbx.vertices = new char[fbx.num_vertex_bytes];
    fbx.indices = new uint32_t[fbx.num_indices];
    if (fbx.num_normal_indices) fbx.normal_indices = new uint32_t[fbx.num_normal_indices];
    if (fbx.num_tangent_indices) fbx.tangent_indices = new uint32_t[fbx.num_tangent_indices];
    if (fbx.num_uv_indices) fbx.uv_indices = new uint32_t[fbx.num_uv_indices];
    if (fbx.num_material_ids) fbx.material_ids = new uint32_t[fbx.num_material_ids];
    if (fbx.num_cameras) fbx.camera = new k3fbxCamera[fbx.num_cameras];
    if (fbx.num_light_nodes) fbx.light_node = new k3fbxLightNode[fbx.num_light_nodes];
    if (fbx.num_skins) fbx.skin = new k3fbxSkin[fbx.num_skins];
    if (fbx.num_clusters) fbx.cluster = new k3fbxClusterNode[fbx.num_clusters];
    if (fbx.num_cluster_indexes) fbx.cluster_indexes = new uint32_t[fbx.num_cluster_indexes];
    if (fbx.num_cluster_weight_bytes) fbx.cluster_weights = new char[fbx.num_cluster_weight_bytes];
    if (fbx.num_anim_layers) {
        fbx.anim_layer = new k3fbxAnimLayer[fbx.num_anim_layers];
        uint32_t i;
        for (i = 0; i < fbx.num_anim_layers; i++) {
            fbx.anim_layer[i].anim_curve_node_id = new uint32_t[K3_FBX_BONE_TO_ANIM_MULTIPLIER * fbx.num_clusters];
        }
    }
    if (fbx.num_anim_curve_nodes) fbx.anim_curve_node = new k3fbxAnimCurveNode[fbx.num_anim_curve_nodes];
    if (fbx.num_anim_curves) fbx.anim_curve = new k3fbxAnimCurve[fbx.num_anim_curves];
    if (fbx.num_anim_curve_time_elements) fbx.anim_curve_time = new uint64_t[fbx.num_anim_curve_time_elements];
    if (fbx.num_anim_curve_data_elements) fbx.anim_curve_data = new float[fbx.num_anim_curve_data_elements];
    if (fbx.num_tangent_bytes) fbx.tangents = new char[fbx.num_tangent_bytes];
    if (fbx.num_bind_pose_nodes) fbx.bind_pose_node = new k3fbxPoseNode[fbx.num_bind_pose_nodes];
    fbx.normals = new char[fbx.num_normal_bytes];
    fbx.uvs = new char[fbx.num_uv_bytes];
    fseek(in_file, node_start, SEEK_SET);
    readFbxNode(&fbx, k3fbxNodeType::UNKNOWN, 0, in_file, &custom_props);

    // convert all double arrays to float arrays
    uint32_t num_verts = fbx.num_vertex_bytes / sizeof(float);
    if (fbx.vert_type == K3_FBX_TYPECODE_DOUBLE_ARRAY) {
        num_verts = fbx.num_vertex_bytes / sizeof(double);
        doubleToFloatArray(fbx.vertices, num_verts);
    }
    if (fbx.norm_type == K3_FBX_TYPECODE_DOUBLE_ARRAY) doubleToFloatArray(fbx.normals, fbx.num_normal_bytes / sizeof(double));
    if (fbx.tangents && fbx.tang_type == K3_FBX_TYPECODE_DOUBLE_ARRAY) doubleToFloatArray(fbx.tangents, fbx.num_tangent_bytes / sizeof(double));
    if (fbx.uv_type == K3_FBX_TYPECODE_DOUBLE_ARRAY) doubleToFloatArray(fbx.uvs, fbx.num_uv_bytes / sizeof(double));

    k3mesh mesh = new k3meshObj;
    k3meshImpl* mesh_impl = mesh->getImpl();
    mesh_impl->_num_meshes = fbx.num_meshes;
    mesh_impl->_mesh_start = new uint32_t[fbx.num_meshes];

    uint32_t num_tris = 0;
    uint32_t i, o = 0;
    for (i = 0; i < fbx.num_indices; i++) {
        if (o < fbx.num_meshes && i >= fbx.mesh[o].start) {
            mesh_impl->_mesh_start[o] = num_tris;
            o++;
        }
        num_tris++;
        if (fbx.indices[i] & 0x80000000) num_tris -= 2;
    }
    mesh_impl->_num_tris = num_tris;

    mesh_impl->_num_textures = fbx.num_textures;
    if (fbx.num_textures) {
        static const uint32_t NUM_UP_BUF = 4;
        uint64_t f = 0;
        uint32_t v = 0;
        k3uploadImage up_image[NUM_UP_BUF];
        k3fence up_image_fence = CreateFence();
        for (i = 0; i < NUM_UP_BUF; i++) {
            up_image[i] = CreateUploadImage();
        }

        mesh_impl->_textures = new k3surf[fbx.num_textures];
        k3resourceDesc rdesc;
        k3viewDesc vdesc = { 0 };
        k3resource img_res;
        for (i = 0; i < fbx.num_textures; i++) {
            if(f >= NUM_UP_BUF) {
                up_image_fence->WaitGpuFence(f - NUM_UP_BUF);
            }

            uint32_t content_start_pos = fbx.texture[i].file_pos;
            if (content_start_pos == ~0x0) {
                k3imageObj::LoadFromFile(up_image[v], fbx.texture[i].filename);
            } else {
                fseek(in_file, content_start_pos, SEEK_SET);
                k3imageObj::LoadFromFileHandle(up_image[v], in_file);
            }
            up_image[v]->GetDesc(&rdesc);
            vdesc.view_index = desc->view_index++;
            mesh_impl->_textures[i] = CreateSurface(&rdesc, NULL, &vdesc, NULL);

            desc->cmd_buf->Reset();
            img_res = mesh_impl->_textures[i]->GetResource();
            desc->cmd_buf->TransitionResource(img_res, k3resourceState::COPY_DEST);
            desc->cmd_buf->UploadImage(up_image[v], img_res);
            desc->cmd_buf->TransitionResource(img_res, k3resourceState::SHADER_RESOURCE);
            desc->cmd_buf->Close();
            SubmitCmdBuf(desc->cmd_buf);

            f = up_image_fence->SetGpuFence(k3gpuQueue::GRAPHICS);
            v++;
            if (v >= NUM_UP_BUF) v = 0;
        }
        WaitGpuIdle();
    }

    fclose(in_file);

    mesh_impl->_lights = new k3light[fbx.num_lights];
    mesh_impl->_num_lights = 0;
    for (i = 0; i < fbx.num_models && mesh_impl->_num_lights < fbx.num_lights; i++) {
        if (fbx.model[i].obj_type == k3fbxObjType::LIGHT) {
            uint32_t light_node_index = fbx.model[i].index.light.light_node;
            if (light_node_index != ~0x0) {
                k3fbxLightNode* light_node = &(fbx.light_node[light_node_index]);
                strncpy(mesh_impl->_lights[mesh_impl->_num_lights].name, light_node->name, K3_FBX_MAX_NAME_LENGTH);
                mesh_impl->_lights[mesh_impl->_num_lights].position[0] = fbx.model[i].translation[0];
                mesh_impl->_lights[mesh_impl->_num_lights].position[1] = fbx.model[i].translation[1];
                mesh_impl->_lights[mesh_impl->_num_lights].position[2] = fbx.model[i].translation[2];
                mesh_impl->_lights[mesh_impl->_num_lights].light_type = light_node->light_type;
                mesh_impl->_lights[mesh_impl->_num_lights].color[0] = light_node->color[0];
                mesh_impl->_lights[mesh_impl->_num_lights].color[1] = light_node->color[1];
                mesh_impl->_lights[mesh_impl->_num_lights].color[2] = light_node->color[2];
                mesh_impl->_lights[mesh_impl->_num_lights].intensity = light_node->intensity;
                mesh_impl->_lights[mesh_impl->_num_lights].decay_type = light_node->decay_type;
                mesh_impl->_lights[mesh_impl->_num_lights].decay_start = light_node->decay_start;
                mesh_impl->_lights[mesh_impl->_num_lights].cast_shadows = light_node->cast_shadows;
                mesh_impl->_num_lights++;
            }

        }
    }

    mesh_impl->_num_cameras = fbx.num_cameras;
    mesh_impl->_cameras = new k3camera[mesh_impl->_num_cameras];
    for (i = 0; i < mesh_impl->_num_cameras; i++) {
        strncpy(mesh_impl->_cameras[i].name, fbx.camera[i].name, K3_FBX_MAX_NAME_LENGTH);
        mesh_impl->_cameras[i].translation[0] = fbx.camera[i].translation[0];
        mesh_impl->_cameras[i].translation[1] = fbx.camera[i].translation[1];
        mesh_impl->_cameras[i].translation[2] = fbx.camera[i].translation[2];
        mesh_impl->_cameras[i].look_at[0] = fbx.camera[i].look_at[1];
        mesh_impl->_cameras[i].look_at[1] = -fbx.camera[i].look_at[0];
        mesh_impl->_cameras[i].look_at[2] = fbx.camera[i].look_at[2];
        mesh_impl->_cameras[i].up[0] = fbx.camera[i].up[0];
        mesh_impl->_cameras[i].up[1] = fbx.camera[i].up[1];
        mesh_impl->_cameras[i].up[2] = fbx.camera[i].up[2];

        // Set the look at position properly
        k3v3_Cross(mesh_impl->_cameras[i].look_at, mesh_impl->_cameras[i].look_at, mesh_impl->_cameras[i].up);
        k3v3_Cross(mesh_impl->_cameras[i].look_at, mesh_impl->_cameras[i].up, mesh_impl->_cameras[i].look_at);
        k3v3_Add(mesh_impl->_cameras[i].look_at, mesh_impl->_cameras[i].translation, mesh_impl->_cameras[i].look_at);

        mesh_impl->_cameras[i].proj_type = (fbx.camera[i].proj_type == K3_FBX_PROJECTION_ORTHOGRAPHIC) ? k3projType::ORTHOGRAPIC : k3projType::PERSPECTIVE;
        mesh_impl->_cameras[i].res_x = fbx.camera[i].res_x;
        mesh_impl->_cameras[i].res_y = fbx.camera[i].res_y;
        float aspect = fbx.camera[i].res_y / (float)fbx.camera[i].res_x;
        mesh_impl->_cameras[i].fovy = aspect * fbx.camera[i].fovx;
        mesh_impl->_cameras[i].ortho_scale = fbx.camera[i].ortho_scale;
        mesh_impl->_cameras[i].near_plane = fbx.camera[i].near_plane;
        mesh_impl->_cameras[i].far_plane = fbx.camera[i].far_plane;
    }

    mesh_impl->_num_models = 0;  // determine which models have actual geometries...those are the only valid ones
    mesh_impl->_num_empties = 0; // determine thte empties as well
    for (i = 0; i < fbx.num_models; i++) {
        uint32_t mesh_index = fbx.model[i].index.model.mesh;
        if (mesh_index != ~0 && fbx.model[i].obj_type == k3fbxObjType::MESH) mesh_impl->_num_models++;
        if (fbx.model[i].obj_type == k3fbxObjType::NONE) mesh_impl->_num_empties++;
    }

    uint32_t* sorted_model_id = new uint32_t[fbx.num_models];
    for (i = 0; i < fbx.num_models; i++) {
        sorted_model_id[i] = ~0x0;
    }
    for (i = 0; i < fbx.num_models; i++) {
        insertFbxModelToSortedList(i, &fbx, sorted_model_id);
    }
    mesh_impl->_model = new k3meshModel[mesh_impl->_num_models];
    mesh_impl->_num_models = 0;  // reset the count, and count again, but this time with allocated entries which are filled in
    float mat[16];
    float x_axis[3] = { 1.0f, 0.0f, 0.0f };
    float y_axis[3] = { 0.0f, 1.0f, 0.0f };
    float z_axis[3] = { 0.0f, 0.0f, 1.0f };
    uint32_t max_mesh_prims = 0;
    for (i = 0; i < fbx.num_models; i++) {
        uint32_t mesh_index = fbx.model[i].index.model.mesh;
        uint32_t mesh_start, num_mesh_prims;
        if (mesh_index != ~0 && fbx.model[i].obj_type == k3fbxObjType::MESH) {
            uint32_t dest_index = sorted_model_id[i];
            // this is a valid model
            mesh_start = mesh_impl->_mesh_start[mesh_index];
            num_mesh_prims = (mesh_index < mesh_impl->_num_meshes - 1) ? mesh_impl->_mesh_start[mesh_index + 1] : num_tris;
            num_mesh_prims -= mesh_start;
            if (num_mesh_prims > max_mesh_prims) max_mesh_prims = num_mesh_prims;
            strncpy(mesh_impl->_model[dest_index].name, fbx.model[i].name, K3_FBX_MAX_NAME_LENGTH);
            mesh_impl->_model[dest_index].mesh_index = mesh_index;
            mesh_impl->_model[dest_index].prim_start = mesh_start;
            mesh_impl->_model[dest_index].num_prims = num_mesh_prims;
            mesh_impl->_model[dest_index].diffuse_color[0] = 1.0f;
            mesh_impl->_model[dest_index].diffuse_color[1] = 1.0f;
            mesh_impl->_model[dest_index].diffuse_color[2] = 1.0f;
            mesh_impl->_model[dest_index].diffuse_map_index = ~0;
            mesh_impl->_model[dest_index].normal_map_index = ~0;
            mesh_impl->_model[dest_index].visibility = fbx.model[i].visibility;
            // Set initial model rotation and position
            k3m4_SetIdentity(mesh_impl->_model[dest_index].world_xform);
            mesh_impl->_model[dest_index].world_xform[0] = fbx.model[i].scaling[0];
            mesh_impl->_model[dest_index].world_xform[5] = fbx.model[i].scaling[1];
            mesh_impl->_model[dest_index].world_xform[10] = fbx.model[i].scaling[2];
            k3m4_SetRotation(mat, -deg2rad(fbx.model[i].rotation[0]), x_axis);
            k3m4_Mul(mesh_impl->_model[dest_index].world_xform, mat, mesh_impl->_model[dest_index].world_xform);
            k3m4_SetRotation(mat, -deg2rad(fbx.model[i].rotation[1]), y_axis);
            k3m4_Mul(mesh_impl->_model[dest_index].world_xform, mat, mesh_impl->_model[dest_index].world_xform);
            k3m4_SetRotation(mat, -deg2rad(fbx.model[i].rotation[2]), z_axis);
            k3m4_Mul(mesh_impl->_model[dest_index].world_xform, mat, mesh_impl->_model[dest_index].world_xform);
            k3m4_SetIdentity(mat);
            mat[3] = fbx.model[i].translation[0];
            mat[7] = fbx.model[i].translation[1];
            mat[11] = fbx.model[i].translation[2];
            k3m4_Mul(mesh_impl->_model[dest_index].world_xform, mat, mesh_impl->_model[dest_index].world_xform);
            if (fbx.model[i].index.model.parent < fbx.num_models) {
                mesh_impl->_model[dest_index].parent = sorted_model_id[fbx.model[i].index.model.parent];
            } else {
                mesh_impl->_model[dest_index].parent = ~0x0;
            }
            uint32_t material_index = fbx.model[i].index.model.material;
            if (material_index != ~0) {
                mesh_impl->_model[dest_index].diffuse_color[0] = fbx.material[material_index].diffuse_color[0];
                mesh_impl->_model[dest_index].diffuse_color[1] = fbx.material[material_index].diffuse_color[1];
                mesh_impl->_model[dest_index].diffuse_color[2] = fbx.material[material_index].diffuse_color[2];
                uint32_t texture_index = fbx.material[material_index].diffuse_texture_index;
                if (texture_index != ~0) {
                    mesh_impl->_model[dest_index].diffuse_map_index = texture_index;
                }
                texture_index = fbx.material[material_index].normal_texture_index;
                if (texture_index != ~0) {
                    mesh_impl->_model[dest_index].normal_map_index = texture_index;
                }
            }
            mesh_impl->_num_models++;
        }
    }
    mesh_impl->_num_model_custom_props = custom_props.num_model_custom_props;
    if (mesh_impl->_num_model_custom_props) {
        mesh_impl->_model_custom_props = fbx.model_custom_prop;
    } else {
        mesh_impl->_model_custom_props = NULL;
    }
    delete[] sorted_model_id;
    // Transform by the mdel parent, if there is any
    for (i = 0; i < mesh_impl->_num_models; i++) {
        uint32_t parent = mesh_impl->_model[i].parent;
        if (parent < mesh_impl->_num_models) {
            k3m4_Mul(mesh_impl->_model[i].world_xform, mesh_impl->_model[parent].world_xform, mesh_impl->_model[i].world_xform);
        }
    }

    mesh_impl->_empties = new k3emptyModel[mesh_impl->_num_empties];
    mesh_impl->_num_empties = 0;
    for (i = 0; i < fbx.num_models; i++) {
        if (fbx.model[i].obj_type == k3fbxObjType::NONE) {
            strncpy(mesh_impl->_empties[mesh_impl->_num_empties].name, fbx.model[i].name, K3_FBX_MAX_NAME_LENGTH);
            // Set initial model rotation and position
            k3m4_SetIdentity(mesh_impl->_empties[mesh_impl->_num_empties].world_xform);
            mesh_impl->_empties[mesh_impl->_num_empties].world_xform[0] = fbx.model[i].scaling[0];
            mesh_impl->_empties[mesh_impl->_num_empties].world_xform[5] = fbx.model[i].scaling[1];
            mesh_impl->_empties[mesh_impl->_num_empties].world_xform[10] = fbx.model[i].scaling[2];
            k3m4_SetRotation(mat, -deg2rad(fbx.model[i].rotation[0]), x_axis);
            k3m4_Mul(mesh_impl->_empties[mesh_impl->_num_empties].world_xform, mat, mesh_impl->_empties[mesh_impl->_num_empties].world_xform);
            k3m4_SetRotation(mat, -deg2rad(fbx.model[i].rotation[1]), y_axis);
            k3m4_Mul(mesh_impl->_empties[mesh_impl->_num_empties].world_xform, mat, mesh_impl->_empties[mesh_impl->_num_empties].world_xform);
            k3m4_SetRotation(mat, -deg2rad(fbx.model[i].rotation[2]), z_axis);
            k3m4_Mul(mesh_impl->_empties[mesh_impl->_num_empties].world_xform, mat, mesh_impl->_empties[mesh_impl->_num_empties].world_xform);
            k3m4_SetIdentity(mat);
            mat[3] = fbx.model[i].translation[0];
            mat[7] = fbx.model[i].translation[1];
            mat[11] = fbx.model[i].translation[2];
            k3m4_Mul(mesh_impl->_empties[mesh_impl->_num_empties].world_xform, mat, mesh_impl->_empties[mesh_impl->_num_empties].world_xform);

            mesh_impl->_num_empties++;
        }
    }

    bool use_ib = (fbx.num_tangent_indices == 0) && (fbx.num_normal_indices == 0) && (fbx.num_uv_indices == 0);
    for (i = 0; i < fbx.num_meshes; i++) {
        use_ib = use_ib && (fbx.mesh[i].normal_mapping == k3fbxMapping::ByVert);
        use_ib = use_ib && (fbx.mesh[i].tangent_mapping == k3fbxMapping::ByVert);
        use_ib = use_ib && (fbx.mesh[i].uv_mapping == k3fbxMapping::ByVert);
        use_ib = use_ib && (fbx.mesh[i].material_mapping == k3fbxMapping::AllSame);
    }
    if (!use_ib) num_verts = 3 * num_tris;

    bool use_skin = (fbx.num_skins > 0) && (fbx.num_clusters > 0);
    uint32_t vert_size = (use_skin) ? 19 : 11;
    mesh_impl->_num_bones = (use_skin) ? fbx.num_clusters : 0;

    uint32_t geom_size = (vert_size * num_verts * sizeof(float)) + ((use_ib) ? 3 * num_tris * sizeof(uint32_t) : 0);
    uint32_t lbuf_size = mesh_impl->_num_lights * sizeof(k3lightBufferData);
    uint32_t up_buf_size = geom_size + lbuf_size;
    float* verts = (float*)desc->up_buf->MapForWrite(up_buf_size);
    float* attrs = verts + 3 * num_verts;
    float* skin_f = verts + 11 * num_verts;
    uint32_t* skin_i = (uint32_t*)(skin_f);
    float* bones = verts + (geom_size + lbuf_size) / sizeof(float);

    float* alloc_skin_f = (use_ib) ? skin_f : new float[8 * num_verts];
    uint32_t* alloc_skin_i = (uint32_t*)alloc_skin_f;

    mesh_impl->_num_verts = num_verts;
    mesh_impl->_geom_data = new float[geom_size];

    k3bufferDesc bdesc = { 0 };
    bdesc.size = 3 * num_verts * sizeof(float);
    bdesc.stride = 3 * sizeof(float);
    bdesc.format = k3fmt::RGB32_FLOAT;
    mesh_impl->_vb = CreateBuffer(&bdesc);

    bdesc.size = 8 * num_verts * sizeof(float);
    bdesc.stride = 8 * sizeof(float);
    bdesc.format = k3fmt::UNKNOWN;
    bdesc.view_index = desc->view_index++;
    bdesc.shader_resource = true;
    mesh_impl->_ab = CreateBuffer(&bdesc);

    if (use_skin) {
        bdesc.size = 8 * num_verts * sizeof(float);
        bdesc.stride = 8 * sizeof(float);
        bdesc.format = k3fmt::UNKNOWN;
        bdesc.view_index = desc->view_index++;
        bdesc.shader_resource = true;
        mesh_impl->_sb = CreateBuffer(&bdesc);

        // clear the skin data
        memset(alloc_skin_i, 0, 8 * num_verts * sizeof(float));
    }

    uint32_t num_lights = mesh_impl->_num_lights;
    if (num_lights) {
        bdesc.size = lbuf_size;
        bdesc.stride = sizeof(k3lightBufferData);
        bdesc.format = k3fmt::UNKNOWN;
        bdesc.view_index = desc->view_index++;
        bdesc.shader_resource = true;
        mesh_impl->_lb = CreateBuffer(&bdesc);

        k3lightBufferData* lb_data = (k3lightBufferData*)(verts + geom_size / sizeof(float));

        for (i = 0; i < num_lights; i++) {
            lb_data[i].color[0] = mesh_impl->_lights[i].color[0];
            lb_data[i].color[1] = mesh_impl->_lights[i].color[1];
            lb_data[i].color[2] = mesh_impl->_lights[i].color[2];
            lb_data[i].intensity = mesh_impl->_lights[i].intensity;
            lb_data[i].position[0] = mesh_impl->_lights[i].position[0];
            lb_data[i].position[1] = mesh_impl->_lights[i].position[1];
            lb_data[i].position[2] = mesh_impl->_lights[i].position[2];
            lb_data[i].decay_start = mesh_impl->_lights[i].decay_start;
            lb_data[i].decay_type = mesh_impl->_lights[i].decay_type;
            lb_data[i].light_type = mesh_impl->_lights[i].light_type;
            lb_data[i].cast_shadows = mesh_impl->_lights[i].cast_shadows;
            lb_data[i].spot_angle = 45.0f;
        }
    }

    if (use_skin) {
        uint32_t b, bone_id;
        mesh_impl->_bones = new k3bone[mesh_impl->_num_bones];

        // Remap bone ids so that parents are always before child
        uint32_t* sorted_bone_id = new uint32_t[mesh_impl->_num_bones];
        // first initialize sorted list to ~0x0, which is the null value
        for (bone_id = 0; bone_id < mesh_impl->_num_bones; bone_id++) {
            sorted_bone_id[bone_id] = ~0x0;
        }
        // Then insert each bone, into the the sorted list, taking care that parents are before the children
        for (bone_id = 0; bone_id < mesh_impl->_num_bones; bone_id++) {
            insertFbxBoneToSortedList(bone_id, &fbx, sorted_bone_id);
        }

        // Loop through all the bones
        for (b = 0; b < mesh_impl->_num_bones; b++) {
            bone_id = sorted_bone_id[b];

            // Find the skin it belongs to
            uint32_t skin_id = fbx.cluster[b].skin_index;
            if (skin_id < fbx.num_skins) {
                // then find the corresponding mesh
                uint32_t w_start = fbx.cluster[b].weight_start;
                uint32_t i_start = fbx.cluster[b].index_start;
                float* weights = (float*)((char*)fbx.cluster_weights + w_start);
                uint32_t* cl_indexes = fbx.cluster_indexes + i_start;
                uint32_t num_cl_indexes = (b == mesh_impl->_num_bones - 1) ? fbx.num_cluster_indexes : fbx.cluster[b + 1].index_start;
                num_cl_indexes -= i_start;
                if (fbx.cluster[b].weight_type == K3_FBX_TYPECODE_DOUBLE_ARRAY) {
                    doubleToFloatArray(weights, num_cl_indexes);
                    fbx.cluster[b].weight_type = K3_FBX_TYPECODE_FLOAT_ARRAY;
                }
                uint32_t mesh_id = fbx.skin[skin_id].mesh_index;
                if (mesh_id < fbx.num_meshes) {
                    uint32_t v_id, v_start = fbx.mesh[mesh_id].vert_offset;
                    uint32_t i, w;
                    for (i = 0; i < num_cl_indexes; i++) {
                        float temp_weight, cur_weight = weights[i];
                        uint32_t temp_bone, cur_bone = bone_id;
                        v_id = v_start + cl_indexes[i];
                        for (w = 0; w < 4; w++) {
                            if (alloc_skin_f[8 * v_id + 4 + w] < cur_weight) {
                                temp_weight = cur_weight;
                                temp_bone = cur_bone;
                                cur_weight = alloc_skin_f[8 * v_id + 4 + w];
                                cur_bone = alloc_skin_i[8 * v_id + w];
                                alloc_skin_f[8 * v_id + 4 + w] = temp_weight;
                                alloc_skin_i[8 * v_id + w] = temp_bone;
                            }
                        }
                    }
                }
            }
            // Find the bone model index
            uint32_t bone_model_index = fbx.cluster[b].bone_index;
            uint32_t parent_bone_index = fbx.model[bone_model_index].index.limb_node.parent;
            uint32_t bind_pose_index = fbx.model[bone_model_index].index.limb_node.bind_pose_index;
            strncpy(mesh_impl->_bones[bone_id].name, fbx.model[bone_model_index].name, K3_FBX_MAX_NAME_LENGTH);
            mesh_impl->_bones[bone_id].position[0] = fbx.model[bone_model_index].translation[0];
            mesh_impl->_bones[bone_id].position[1] = fbx.model[bone_model_index].translation[1];
            mesh_impl->_bones[bone_id].position[2] = fbx.model[bone_model_index].translation[2];
            mesh_impl->_bones[bone_id].scaling[0] = fbx.model[bone_model_index].scaling[0];
            mesh_impl->_bones[bone_id].scaling[1] = fbx.model[bone_model_index].scaling[1];
            mesh_impl->_bones[bone_id].scaling[2] = fbx.model[bone_model_index].scaling[2];
            if (bind_pose_index < fbx.num_bind_pose_nodes) {
                memcpy(mesh_impl->_bones[bone_id].inv_bind_pose, fbx.bind_pose_node[bind_pose_index].xform, 16*sizeof(float));
                // TODO: Shold copy and transpose in one step
                k3m4_Transpose(mesh_impl->_bones[bone_id].inv_bind_pose);
                k3m4_Inverse(mesh_impl->_bones[bone_id].inv_bind_pose);
            } else {
                // No bind pose found, so assume identity
                k3m4_SetIdentity(mesh_impl->_bones[bone_id].inv_bind_pose);
            }
            if (parent_bone_index < fbx.num_models) {
                uint32_t parent_bone_id = fbx.model[parent_bone_index].index.limb_node.cluster_index;
                uint32_t sorted_parent_bone_id = sorted_bone_id[parent_bone_id];
                mesh_impl->_bones[bone_id].parent = sorted_parent_bone_id;
                if (mesh_impl->_bones[bone_id].parent > bone_id) {
                    k3error::Handler("Parent bone after child", "k3gfxObj::CreateMesh");
                }
            } else {
                mesh_impl->_bones[bone_id].parent = ~0x0;
            }
            float rot_mat[16];
            k3m4_SetRotation(rot_mat, -deg2rad(fbx.model[bone_model_index].rotation[0]), x_axis);
            k3m4_SetRotation(mat, -deg2rad(fbx.model[bone_model_index].rotation[1]), y_axis);
            k3m4_Mul(rot_mat, mat, rot_mat);
            k3m4_SetRotation(mat, -deg2rad(fbx.model[bone_model_index].rotation[2]), z_axis);
            k3m4_Mul(rot_mat, mat, rot_mat);
            k3m4_MatToQuat(mesh_impl->_bones[bone_id].rot_quat, rot_mat);
        }
        // Normalize skin weights
        uint32_t v_id;
        for (v_id = 0; v_id < num_verts; v_id++) {
            if (*(alloc_skin_f + 8 * v_id + 4) >= 0.1f) {
                k3v4_Normalize(alloc_skin_f + 8 * v_id + 4);
            }
        }

        // load animations
        mesh_impl->_num_anims = fbx.num_anim_layers;
        if (mesh_impl->_num_anims) {
            mesh_impl->_anim = new k3anim[mesh_impl->_num_anims];
            uint32_t anim_id, curve_node_index, curve_node_id, curve_id, axis;
            uint32_t k, cur_time, cur_curve_index, dest_index, dest_stride;
            uint64_t* curve_time_ptr;
            //uint32_t curve_time;
            //uint32_t last_curve_time;
            float* dest;
            float* default_value;
            //float cur_value, interp, next_value;
            for (anim_id = 0; anim_id < mesh_impl->_num_anims; anim_id++) {
                mesh_impl->_anim[anim_id].num_keyframes = 0;
                mesh_impl->_anim[anim_id].keyframe_delta_msec = 0;
                for (curve_node_index = 0; curve_node_index < fbx.anim_layer[anim_id].num_anim_curve_nodes; curve_node_index++) {
                    curve_node_id = fbx.anim_layer[anim_id].anim_curve_node_id[curve_node_index];
                    for (axis = 0; axis < 3; axis++) {
                        curve_id = fbx.anim_curve_node[curve_node_id].anim_curve[axis];
                        if (curve_id < fbx.num_anim_curves && fbx.anim_curve[curve_id].num_key_frames > mesh_impl->_anim[anim_id].num_keyframes) {
                            mesh_impl->_anim[anim_id].num_keyframes = fbx.anim_curve[curve_id].num_key_frames;
                            uint64_t* end_time_ptr = fbx.anim_curve_time + fbx.anim_curve[curve_id].time_start + fbx.anim_curve[curve_id].num_key_frames - 1;
                            mesh_impl->_anim[anim_id].keyframe_delta_msec = (uint32_t)(*end_time_ptr / K3_FBX_TICKS_PER_MSEC);
                        }
                    }
                }
                if (mesh_impl->_anim[anim_id].num_keyframes == 0) mesh_impl->_anim[anim_id].num_keyframes = 1;
                mesh_impl->_anim[anim_id].keyframe_delta_msec /= mesh_impl->_anim[anim_id].num_keyframes;
                strncpy(mesh_impl->_anim[anim_id].name, fbx.anim_layer[anim_id].name, K3_FBX_MAX_NAME_LENGTH);
                mesh_impl->_anim[anim_id].bone_data = new k3boneData[mesh_impl->_anim[anim_id].num_keyframes * mesh_impl->_num_bones];
                mesh_impl->_anim[anim_id].bone_flag = new uint32_t[mesh_impl->_num_bones];
                memset(mesh_impl->_anim[anim_id].bone_data, 0, mesh_impl->_anim[anim_id].num_keyframes* mesh_impl->_num_bones * sizeof(k3boneData));
                memset(mesh_impl->_anim[anim_id].bone_flag, 0, mesh_impl->_num_bones * sizeof(uint32_t));
                for (bone_id = 0; bone_id < mesh_impl->_num_bones; bone_id++) {
                    for (k = 0; k < mesh_impl->_anim[anim_id].num_keyframes; k++) {
                        for (axis = 0; axis < 3; axis++) {
                            // initialize scale to 1.0
                            mesh_impl->_anim[anim_id].bone_data[k * mesh_impl->_num_bones + bone_id].scaling[axis] = 1.0f;
                        }
                    }
                }
                dest_stride = (sizeof(k3boneData) / sizeof(float)) * mesh_impl->_num_bones;
                for (curve_node_index = 0; curve_node_index < fbx.anim_layer[anim_id].num_anim_curve_nodes; curve_node_index++) {
                    curve_node_id = fbx.anim_layer[anim_id].anim_curve_node_id[curve_node_index];
                    dest_index = fbx.anim_curve_node[curve_node_id].limb_node;
                    if ((fbx.anim_curve_node[curve_node_id].curve_type != k3fbxAnimCurveType::None) && (dest_index < fbx.num_models)) {
                        dest_index = fbx.model[dest_index].index.limb_node.cluster_index;
                        dest_index = sorted_bone_id[dest_index];
                        default_value = fbx.anim_curve_node[curve_node_id].default_value;
                        switch (fbx.anim_curve_node[curve_node_id].curve_type) {
                        case k3fbxAnimCurveType::Translation:
                            dest = mesh_impl->_anim[anim_id].bone_data[dest_index].position;
                            break;
                        case k3fbxAnimCurveType::Scaling:
                            dest = mesh_impl->_anim[anim_id].bone_data[dest_index].scaling;
                            break;
                        case k3fbxAnimCurveType::Rotation:
                            dest = mesh_impl->_anim[anim_id].bone_data[dest_index].rot_quat;
                            break;
                        }
                        for (axis = 0; axis < 3; axis++) {
                            curve_id = fbx.anim_curve_node[curve_node_id].anim_curve[axis];
                            if (curve_id < fbx.num_anim_curves) {
                                cur_time = 0;
                                cur_curve_index = 0;
                                curve_time_ptr = fbx.anim_curve_time + fbx.anim_curve[curve_id].time_start;

                                //cur_value = (fbx.anim_curve_node[curve_node_id].curve_type == k3fbxAnimCurveType::Scaling) ? 1.0f : 0.0f;
                                //next_value = fbx.anim_curve_data[fbx.anim_curve[curve_id].data_start];
                                //curve_time = (uint32_t)(*curve_time_ptr / K3_FBX_TICKS_PER_MSEC);
                                //last_curve_time = 0;
                                //for (k = 0; k < mesh_impl->_anim[anim_id].num_keyframes; k++) {
                                //    if (cur_time >= curve_time) {
                                //        cur_value = next_value;
                                //        cur_curve_index++;
                                //        curve_time_ptr++;
                                //        cur_time -= curve_time;
                                //        last_curve_time = (uint32_t)(*(curve_time_ptr - 1) / K3_FBX_TICKS_PER_MSEC);
                                //        if (cur_curve_index < fbx.anim_curve[curve_id].num_key_frames) {
                                //            curve_time = (uint32_t)(*curve_time_ptr / K3_FBX_TICKS_PER_MSEC) - last_curve_time;
                                //            next_value = fbx.anim_curve_data[fbx.anim_curve[curve_id].data_start + cur_curve_index];
                                //        } else {
                                //            curve_time = mesh_impl->_anim[anim_id].keyframe_delta_msec * mesh_impl->_anim[anim_id].num_keyframes - last_curve_time;
                                //            next_value = (fbx.anim_curve_node[curve_node_id].curve_type == k3fbxAnimCurveType::Scaling) ? 1.0f : 0.0f;
                                //        }
                                //    }
                                //    interp = cur_time / (float)curve_time;
                                //    *(dest + dest_stride * k) = (1.0f - interp) * cur_value + interp * next_value;
                                //
                                //    cur_time += mesh_impl->_anim[anim_id].keyframe_delta_msec;
                                //}

                                for (k = 0; k < mesh_impl->_anim[anim_id].num_keyframes; k++) {
                                    *(dest + dest_stride * k) = fbx.anim_curve_data[fbx.anim_curve[curve_id].data_start + cur_curve_index];

                                    if ( fabsf(*(dest + dest_stride * k) - default_value[axis]) >= 0.001f) {
                                        // if the curve has some non-zero motion, then set the bone to be morphed
                                        mesh_impl->_anim[anim_id].bone_flag[dest_index] |= K3_BONE_FLAG_MORPH;
                                    }

                                    if (cur_curve_index < fbx.anim_curve[curve_id].num_key_frames - 1) {
                                        // TODO: check if an increment is needed
                                        cur_curve_index++;
                                    }
                                    cur_time += mesh_impl->_anim[anim_id].keyframe_delta_msec;
                                }
                            }
                            dest++;
                        }
                    }
                }
                // convert rotations from euler angles to quaternions
                for (k = 0; k < (mesh_impl->_anim[anim_id].num_keyframes * mesh_impl->_num_bones); k++) {
                    dest = mesh_impl->_anim[anim_id].bone_data[k].rot_quat;
                    dest[0] = deg2rad(dest[0]);
                    dest[1] = deg2rad(dest[1]);
                    dest[2] = deg2rad(dest[2]);
                    k3v4_SetQuatEuler(dest, dest);
                }
            }
        }

        delete[] sorted_bone_id;
    }

    float* fbx_verts = (float*)fbx.vertices;
    float* fbx_normals = (float*)fbx.normals;
    float* fbx_tangents = (float*)fbx.tangents;
    float* fbx_uvs = (float*)fbx.uvs;
    uint32_t ipos = 0, poly_start;
    uint32_t poly = 0;
    // these restart per object
    uint32_t local_ipos = 0, local_poly_start;
    uint32_t local_poly = 0;
    uint32_t v_index, n_index, t_index, uv_index;
    bool end_poly;
    uint32_t i0, i1, i2, j;
    float bitangent[3];
    o = 0;
    if(use_ib) {
        uint32_t* indices = (uint32_t*)(verts + vert_size * num_verts);
        poly_start = ipos;
        for (i = 0; i < num_verts; i++) {
            verts[3 * i + 0] = fbx_verts[3 * i + 0];
            verts[3 * i + 1] = fbx_verts[3 * i + 1];
            verts[3 * i + 2] = fbx_verts[3 * i + 2];
            attrs[8 * i + 0] = fbx_normals[3 * i + 0];
            attrs[8 * i + 1] = fbx_normals[3 * i + 1];
            attrs[8 * i + 2] = fbx_normals[3 * i + 2];
            attrs[8 * i + 3] = fbx_uvs[2 * i + 0];
            attrs[8 * i + 4] = 1.0f - fbx_uvs[2 * i + 1];
            if (fbx.tangents) {
                attrs[8 * i + 5] = fbx_tangents[3 * i + 0];
                attrs[8 * i + 6] = fbx_tangents[3 * i + 1];
                attrs[8 * i + 7] = fbx_tangents[3 * i + 0];
            } else {
                attrs[8 * i + 5] = 1.0f;
                attrs[8 * i + 6] = 0.0f;
                attrs[8 * i + 7] = 0.0f;
            }
        }
        for (i = 0; i < num_tris; i++) {
            if (o + 1 < fbx.num_meshes && i >= mesh_impl->_mesh_start[o + 1]) o++;
            indices[3 * i + 0] = fbx.indices[poly_start];
            indices[3 * i + 1] = fbx.indices[ipos + 1];
            indices[3 * i + 2] = fbx.indices[ipos + 2];
            if (indices[3 * i + 2] & 0x80000000) {
                indices[3 * i + 2] = ~indices[3 * i + 2];
                ipos += 3;
                poly_start = ipos;
            } else {
                ipos++;
            }
            indices[3 * i + 0] += fbx.mesh[o].vert_offset;
            indices[3 * i + 1] += fbx.mesh[o].vert_offset;
            indices[3 * i + 2] += fbx.mesh[o].vert_offset;
            if (fbx.tangents == NULL) {
                i0 = indices[3 * i + 0];
                i1 = indices[3 * i + 1];
                i2 = indices[3 * i + 2];
                k3v3_SetTangentBitangent(NULL, bitangent, verts + 3 * i0, verts + 3 * i1, verts + 3 * i2, attrs + 8 * i0 + 3, attrs + 8 * i1 + 3, attrs + 8 * i2 + 3);
                k3v3_Cross(attrs + 8 * i0 + 5, attrs + 8 * i0 + 0, bitangent);
                k3v3_Normalize(attrs + 8 * i0 + 5);
                k3v3_SetTangentBitangent(NULL, bitangent, verts + 3 * i1, verts + 3 * i2, verts + 3 * i0, attrs + 8 * i1 + 3, attrs + 8 * i2 + 3, attrs + 8 * i0 + 3);
                k3v3_Cross(attrs + 8 * i1 + 5, attrs + 8 * i1 + 0, bitangent);
                k3v3_Normalize(attrs + 8 * i1 + 5);
                k3v3_SetTangentBitangent(NULL, bitangent, verts + 3 * i2, verts + 3 * i0, verts + 3 * i1, attrs + 8 * i2 + 3, attrs + 8 * i0 + 3, attrs + 8 * i1 + 3);
                k3v3_Cross(attrs + 8 * i2 + 5, attrs + 8 * i2 + 0, bitangent);
                k3v3_Normalize(attrs + 8 * i2 + 5);
            }
        }
    } else {
        poly_start = ipos;
        local_poly_start = local_ipos;
        uint32_t obj_v_index[3];
        uint32_t obj_n_index[3];
        uint32_t obj_t_index[3];
        uint32_t obj_uv_index[3];
        k3fbxAttrLinkList** attr_ll = new k3fbxAttrLinkList*[3 * max_mesh_prims];
        k3fbxAttrLinkList* attr_nodes = new k3fbxAttrLinkList[3 * max_mesh_prims];
        k3fbxAttrLinkList* alloc_attr_node = attr_nodes;
        k3fbxAttrLinkList* cur_attr_node;
        memset(attr_ll, 0, 3 * max_mesh_prims * sizeof(k3fbxAttrLinkList*));
        for (i = 0; i < num_tris; i++) {
            if (o + 1 < fbx.num_meshes && i >= mesh_impl->_mesh_start[o + 1]) {
                local_ipos = 0;
                local_poly_start = 0;
                local_poly = 0;
                o++;
                alloc_attr_node = attr_nodes;
                memset(attr_ll, 0, 3 * max_mesh_prims * sizeof(k3fbxAttrLinkList*));
            }
            v_index = fbx.indices[poly_start];
            obj_v_index[0] = v_index;
            v_index += fbx.mesh[o].vert_offset;
            verts[9 * i + 0] = fbx_verts[3 * v_index + 0];
            verts[9 * i + 1] = fbx_verts[3 * v_index + 1];
            verts[9 * i + 2] = fbx_verts[3 * v_index + 2];
            if (use_skin) {
                memcpy(skin_f + 8 * (3 * i + 0), alloc_skin_f + 8 * v_index, 8 * sizeof(float));
            }
            switch (fbx.mesh[o].normal_mapping) {
            case k3fbxMapping::None: n_index = 0; break;
            case k3fbxMapping::ByPoly: n_index = local_poly; break;
            case k3fbxMapping::ByPolyVert: n_index = local_poly_start; break;
            case k3fbxMapping::ByVert: n_index = fbx.indices[poly_start]; break;
            case k3fbxMapping::AllSame: n_index = 0; break;
            }
            if (fbx.mesh[o].normal_reference == k3fbxReference::ByIndex) {
                n_index += fbx.mesh[o].normal_index_offset;
                n_index = fbx.normal_indices[n_index];
            }
            obj_n_index[0] = n_index;
            n_index += fbx.mesh[o].normal_offset;
            attrs[24 * i + 0] = fbx_normals[3 * n_index + 0];
            attrs[24 * i + 1] = fbx_normals[3 * n_index + 1];
            attrs[24 * i + 2] = fbx_normals[3 * n_index + 2];
            if (fbx.tangents) {
                switch (fbx.mesh[o].tangent_mapping) {
                case k3fbxMapping::None: t_index = 0; break;
                case k3fbxMapping::ByPoly: t_index = local_poly; break;
                case k3fbxMapping::ByPolyVert: t_index = local_poly_start; break;
                case k3fbxMapping::ByVert: t_index = fbx.indices[poly_start]; break;
                case k3fbxMapping::AllSame: t_index = 0; break;
                }
                if (fbx.mesh[o].tangent_reference == k3fbxReference::ByIndex) {
                    t_index += fbx.mesh[o].tangent_index_offset;
                    t_index = fbx.tangent_indices[t_index];
                }
                obj_t_index[0] = t_index;
                t_index += fbx.mesh[o].tangent_offset;
                attrs[24 * i + 5] = fbx_tangents[3 * t_index + 0];
                attrs[24 * i + 6] = fbx_tangents[3 * t_index + 1];
                attrs[24 * i + 7] = fbx_tangents[3 * t_index + 2];
            } else {
                attrs[24 * i + 5] = 1.0f;
                attrs[24 * i + 6] = 0.0f;
                attrs[24 * i + 7] = 0.0f;
            }
            switch (fbx.mesh[o].uv_mapping) {
            case k3fbxMapping::None: uv_index = 0; break;
            case k3fbxMapping::ByPoly: uv_index = local_poly; break;
            case k3fbxMapping::ByPolyVert: uv_index = local_poly_start; break;
            case k3fbxMapping::ByVert: uv_index = fbx.indices[poly_start]; break;
            case k3fbxMapping::AllSame: uv_index = 0; break;
            }
            if (fbx.mesh[o].uv_reference == k3fbxReference::ByIndex) {
                uv_index += fbx.mesh[o].uv_index_offset;
                uv_index = fbx.uv_indices[uv_index];
            }
            obj_uv_index[0] = uv_index;
            uv_index += fbx.mesh[o].uv_offset;
            attrs[24 * i + 3] = fbx_uvs[2 * uv_index + 0];
            attrs[24 * i + 4] = 1.0f - fbx_uvs[2 * uv_index + 1];
            v_index = fbx.indices[ipos + 1];
            obj_v_index[1] = v_index;
            v_index += fbx.mesh[o].vert_offset;
            verts[9 * i + 3] = fbx_verts[3 * v_index + 0];
            verts[9 * i + 4] = fbx_verts[3 * v_index + 1];
            verts[9 * i + 5] = fbx_verts[3 * v_index + 2];
            if (use_skin) {
                memcpy(skin_f + 8 * (3 * i + 1), alloc_skin_f + 8 * v_index, 8 * sizeof(float));
            }
            switch (fbx.mesh[o].normal_mapping) {
            case k3fbxMapping::None: n_index = 0; break;
            case k3fbxMapping::ByPoly: n_index = local_poly; break;
            case k3fbxMapping::ByPolyVert: n_index = local_ipos + 1; break;
            case k3fbxMapping::ByVert: n_index = fbx.indices[ipos + 1]; break;
            case k3fbxMapping::AllSame: n_index = 0; break;
            }
            if (fbx.mesh[o].normal_reference == k3fbxReference::ByIndex) {
                n_index += fbx.mesh[o].normal_index_offset;
                n_index = fbx.normal_indices[n_index];
            }
            obj_n_index[1] = n_index;
            n_index += fbx.mesh[o].normal_offset;
            attrs[24 * i + 8] = fbx_normals[3 * n_index + 0];
            attrs[24 * i + 9] = fbx_normals[3 * n_index + 1];
            attrs[24 * i + 10] = fbx_normals[3 * n_index + 2];
            if (fbx.tangents) {
                switch (fbx.mesh[o].tangent_mapping) {
                case k3fbxMapping::None: t_index = 0; break;
                case k3fbxMapping::ByPoly: t_index = local_poly; break;
                case k3fbxMapping::ByPolyVert: t_index = local_ipos + 1; break;
                case k3fbxMapping::ByVert: t_index = fbx.indices[ipos + 1]; break;
                case k3fbxMapping::AllSame: t_index = 0; break;
                }
                if (fbx.mesh[o].tangent_reference == k3fbxReference::ByIndex) {
                    t_index += fbx.mesh[o].tangent_index_offset;
                    t_index = fbx.tangent_indices[t_index];
                }
                obj_t_index[1] = t_index;
                t_index += fbx.mesh[o].tangent_offset;
                attrs[24 * i + 13] = fbx_tangents[3 * t_index + 0];
                attrs[24 * i + 14] = fbx_tangents[3 * t_index + 1];
                attrs[24 * i + 15] = fbx_tangents[3 * t_index + 2];
            } else {
                attrs[24 * i + 13] = 1.0f;
                attrs[24 * i + 14] = 0.0f;
                attrs[24 * i + 15] = 0.0f;
            }
            switch (fbx.mesh[o].uv_mapping) {
            case k3fbxMapping::None: uv_index = 0; break;
            case k3fbxMapping::ByPoly: uv_index = local_poly; break;
            case k3fbxMapping::ByPolyVert: uv_index = local_ipos + 1; break;
            case k3fbxMapping::ByVert: uv_index = fbx.indices[ipos + 1]; break;
            case k3fbxMapping::AllSame: uv_index = 0; break;
            }
            if (fbx.mesh[o].uv_reference == k3fbxReference::ByIndex) {
                uv_index += fbx.mesh[o].uv_index_offset;
                uv_index = fbx.uv_indices[uv_index];
            }
            obj_uv_index[1] = uv_index;
            uv_index += fbx.mesh[o].uv_offset;
            attrs[24 * i + 11] = fbx_uvs[2 * uv_index + 0];
            attrs[24 * i + 12] = 1.0f - fbx_uvs[2 * uv_index + 1];
            v_index = fbx.indices[ipos + 2];
            end_poly = (v_index & 0x80000000) ? true : false;
            if (end_poly) v_index = ~v_index;
            obj_v_index[2] = v_index;
            v_index += fbx.mesh[o].vert_offset;
            verts[9 * i + 6] = fbx_verts[3 * v_index + 0];
            verts[9 * i + 7] = fbx_verts[3 * v_index + 1];
            verts[9 * i + 8] = fbx_verts[3 * v_index + 2];
            if (use_skin) {
                memcpy(skin_f + 8 * (3 * i + 2), alloc_skin_f + 8 * v_index, 8 * sizeof(float));
            }
            switch (fbx.mesh[o].normal_mapping) {
            case k3fbxMapping::None: n_index = 0; break;
            case k3fbxMapping::ByPoly: n_index = local_poly; break;
            case k3fbxMapping::ByPolyVert: n_index = local_ipos + 2; break;
            case k3fbxMapping::ByVert: n_index = (end_poly) ? ~fbx.indices[ipos + 2] : fbx.indices[ipos + 2]; break;
            case k3fbxMapping::AllSame: n_index = 0; break;
            }
            if (fbx.mesh[o].normal_reference == k3fbxReference::ByIndex) {
                n_index += fbx.mesh[o].normal_index_offset;
                n_index = fbx.normal_indices[n_index];
            }
            obj_n_index[2] = n_index;
            n_index += fbx.mesh[o].normal_offset;
            attrs[24 * i + 16] = fbx_normals[3 * n_index + 0];
            attrs[24 * i + 17] = fbx_normals[3 * n_index + 1];
            attrs[24 * i + 18] = fbx_normals[3 * n_index + 2];
            if (fbx.tangents) {
                switch (fbx.mesh[o].tangent_mapping) {
                case k3fbxMapping::None: t_index = 0; break;
                case k3fbxMapping::ByPoly: t_index = local_poly; break;
                case k3fbxMapping::ByPolyVert: t_index = local_ipos + 2; break;
                case k3fbxMapping::ByVert: t_index = (end_poly) ? ~fbx.indices[ipos + 2] : fbx.indices[ipos + 2]; break;
                case k3fbxMapping::AllSame: t_index = 0; break;
                }
                if (fbx.mesh[o].tangent_reference == k3fbxReference::ByIndex) {
                    t_index += fbx.mesh[o].tangent_index_offset;
                    t_index = fbx.tangent_indices[t_index];
                }
                obj_t_index[2] = t_index;
                t_index += fbx.mesh[o].tangent_offset;
                attrs[24 * i + 21] = fbx_tangents[3 * t_index + 0];
                attrs[24 * i + 22] = fbx_tangents[3 * t_index + 1];
                attrs[24 * i + 23] = fbx_tangents[3 * t_index + 2];
            } else {
                attrs[24 * i + 21] = 1.0f;
                attrs[24 * i + 22] = 0.0f;
                attrs[24 * i + 23] = 0.0f;
            }
            switch (fbx.mesh[o].uv_mapping) {
            case k3fbxMapping::None: uv_index = 0; break;
            case k3fbxMapping::ByPoly: uv_index = local_poly; break;
            case k3fbxMapping::ByPolyVert: uv_index = local_ipos + 2; break;
            case k3fbxMapping::ByVert: uv_index = (end_poly) ? ~fbx.indices[ipos + 2] : fbx.indices[ipos + 2]; break;
            case k3fbxMapping::AllSame: uv_index = 0; break;
            }
            if (fbx.mesh[o].uv_reference == k3fbxReference::ByIndex) {
                uv_index += fbx.mesh[o].uv_index_offset;
                uv_index = fbx.uv_indices[uv_index];
            }
            obj_uv_index[2] = uv_index;
            uv_index += fbx.mesh[o].uv_offset;
            attrs[24 * i + 19] = fbx_uvs[2 * uv_index + 0];
            attrs[24 * i + 20] = 1.0f - fbx_uvs[2 * uv_index + 1];
            if (end_poly) {
                ipos += 3;
                local_ipos += 3;
                poly_start = ipos;
                local_poly_start = local_ipos;
                poly++;
                local_poly++;
            } else {
                ipos++;
                local_ipos++;
            }
            
            if (fbx.tangents == NULL) {
                for (j = 0; j < 3; j++) {
                    for (cur_attr_node = attr_ll[obj_v_index[j]]; cur_attr_node != NULL; cur_attr_node = cur_attr_node->next) {
                        if (cur_attr_node->n_index == obj_n_index[j] && cur_attr_node->uv_index == obj_uv_index[j]) {
                            break;
                        }
                    }
                    if (cur_attr_node) {
                        memcpy(attrs + 8 * (3 * i + j) + 5, cur_attr_node->attr, 3 * sizeof(float));
                    } else {
                        i0 = 3 * i + j;
                        i1 = 3 * i + ((j > 1) ? j - 2 : j + 1);
                        i2 = 3 * i + ((j > 0) ? j - 1 : j + 2);
                        k3v3_SetTangentBitangent(NULL, bitangent, verts + 3 * i0, verts + 3 * i1, verts + 3 * i2, attrs + 8 * i0 + 3, attrs + 8 * i1 + 3, attrs + 8 * i2 + 3);
                        k3v3_Cross(attrs + 8 * i0 + 5, attrs + 8 * i0 + 0, bitangent);
                        k3v3_Normalize(attrs + 8 * i0 + 5);
                        alloc_attr_node->n_index = obj_n_index[j];
                        alloc_attr_node->uv_index = obj_uv_index[j];
                        alloc_attr_node->attr = attrs + 8 * i0 + 5;
                        alloc_attr_node->next = attr_ll[obj_v_index[j]];
                        attr_ll[obj_v_index[j]] = alloc_attr_node;
                        alloc_attr_node++;
                    }
                }
            }

        }
        delete[] attr_ll;
        delete[] attr_nodes;
    }

    memcpy(mesh_impl->_geom_data, verts, geom_size);

    desc->up_buf->Unmap();

    k3resource buf_res;
    desc->cmd_buf->Reset();
    buf_res = mesh_impl->_vb->GetResource();
    desc->cmd_buf->TransitionResource(buf_res, k3resourceState::COPY_DEST);
    desc->cmd_buf->UploadBufferSrcRange(desc->up_buf, buf_res, 0, 3 * num_verts * sizeof(float));
    desc->cmd_buf->TransitionResource(buf_res, k3resourceState::COMMON);
    buf_res = mesh_impl->_ab->GetResource();
    desc->cmd_buf->TransitionResource(buf_res, k3resourceState::COPY_DEST);
    desc->cmd_buf->UploadBufferSrcRange(desc->up_buf, buf_res, 3 * num_verts * sizeof(float), 8 * num_verts * sizeof(float));
    desc->cmd_buf->TransitionResource(buf_res, k3resourceState::COMMON);
    if (use_skin) {
        buf_res = mesh_impl->_sb->GetResource();
        desc->cmd_buf->TransitionResource(buf_res, k3resourceState::COPY_DEST);
        desc->cmd_buf->UploadBufferSrcRange(desc->up_buf, buf_res, 11 * num_verts * sizeof(float), 8 * num_verts * sizeof(float));
        desc->cmd_buf->TransitionResource(buf_res, k3resourceState::COMMON);
    }
    if (lbuf_size) {
        buf_res = mesh_impl->_lb->GetResource();
        desc->cmd_buf->TransitionResource(buf_res, k3resourceState::COPY_DEST);
        desc->cmd_buf->UploadBufferSrcRange(desc->up_buf, buf_res, geom_size, lbuf_size);
        desc->cmd_buf->TransitionResource(buf_res, k3resourceState::COMMON);
    }
    desc->cmd_buf->Close();
    SubmitCmdBuf(desc->cmd_buf);

    delete[] fbx.mesh;
    delete[] fbx.model;
    // Don't delete custom prop - it's directly transferred to the mesh_impl object
    //if (fbx.model_custom_prop) delete[] fbx.model_custom_prop;
    if (fbx.light_node) delete[] fbx.light_node;
    if (fbx.camera) delete[] fbx.camera;
    if (fbx.material) delete[] fbx.material;
    if (fbx.texture) delete[] fbx.texture;
    if (fbx.content_data) delete[] fbx.content_data;
    delete[] fbx.vertices;
    if (fbx.uv_indices) delete[] fbx.uv_indices;
    if (fbx.tangent_indices) delete[] fbx.tangent_indices;
    if (fbx.normal_indices) delete[] fbx.normal_indices;
    if (fbx.material_ids) delete[] fbx.material_ids;
    if (fbx.skin) delete[] fbx.skin;
    if (fbx.cluster) delete[] fbx.cluster;
    if (fbx.cluster_indexes) delete[] fbx.cluster_indexes;
    if (fbx.cluster_weights) delete[] fbx.cluster_weights;
    if (!use_ib) delete[] alloc_skin_f;
    if (fbx.anim_layer) {
        uint32_t i;
        for (i = 0; i < fbx.num_anim_layers; i++) {
            if (fbx.anim_layer[i].anim_curve_node_id) delete[] fbx.anim_layer[i].anim_curve_node_id;
        }
        delete[] fbx.anim_layer;
    }
    if (fbx.anim_curve_node) delete[] fbx.anim_curve_node;
    if (fbx.anim_curve) delete[] fbx.anim_curve;
    if (fbx.anim_curve_time) delete[] fbx.anim_curve_time;
    if (fbx.anim_curve_data) delete[] fbx.anim_curve_data;
    if (fbx.tangents) delete[] fbx.tangents;
    if (fbx.bind_pose_node) delete[] fbx.bind_pose_node;
    delete[] fbx.indices;
    delete[] fbx.normals;
    delete[] fbx.uvs;

    return mesh;
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
