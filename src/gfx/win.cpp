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
// mesh class

k3meshImpl::k3meshImpl()
{
    _num_meshes = 0;
    _num_models = 0;
    _num_tris = 0;
    _num_textures = 0;
    _num_cameras = 0;
    _num_lights = 0;
    _mesh_start = NULL;
    _model = NULL;
    _textures = NULL;
    _cameras = NULL;
    _lights = NULL;
    _ib = NULL;
    _vb = NULL;
    _ab = NULL;
    _lb = NULL;
}

k3meshImpl::~k3meshImpl()
{
    if (_mesh_start) {
        delete[] _mesh_start;
        _mesh_start = NULL;
    }
    if (_model) {
        delete[] _model;
        _model = NULL;
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

K3API uint32_t k3meshObj::getNumCameras()
{
    return _data->_num_cameras;
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

K3API float* k3meshObj::getCameraPerspective(float* d, uint32_t camera, bool left_handed, bool dx_style, bool reverse_z)
{
    if (camera > _data->_num_cameras) return d;
    k3camera* cam = _data->_cameras + camera;
    float aspect = cam->res_x / (float)cam->res_y;
    k3m4_SetPerspectiveFov(d, deg2rad(cam->fovy), aspect, cam->near_plane, cam->far_plane, left_handed, dx_style, reverse_z);
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


#define K3_FBX_DEBUG 0

enum class k3fbxNodeType {
    UNKNOWN,
    OBJECTS,
    GEOMETRY,
    VERTICES,
    POLYGON_VERT_INDEX,
    LAYER_ELEMENT_NORMAL,
    LAYER_ELEMENT_UV,
    LAYER_ELEMENT_MATERIAL,
    MAPPING_TYPE,
    REFERENCE_TYPE,
    NORMALS,
    NORMAL_INDEX,
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
    LOOK_AT
};

enum class k3fbxObjType {
    NONE,
    MESH,
    LIGHT,
    CAMERA
};

enum class k3fbxProperty {
    NONE,
    LOCAL_TRANSLATION,
    LOCAL_ROTATION,
    DIFFUSE_COLOR,
    POSITION,
    UP_VECTOR,
    INTEREST_POSITION,
    ASPECT_WIDTH,
    ASPECT_HEIGHT,
    FOVX,
    NEAR_PLANE,
    FAR_PLANE,
    LIGHT_TYPE,
    COLOR,
    INTENSITY,
    DECAY_TYPE,
    DECAY_START,
    CAST_SHADOWS
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

struct k3fbxMeshData {
    uint64_t id;
    uint32_t start;
    uint32_t vert_offset;
    uint32_t normal_offset;
    uint32_t normal_index_offset;
    uint32_t uv_offset;
    uint32_t uv_index_offset;
    uint32_t material_offset;
    k3fbxMapping normal_mapping;
    k3fbxMapping uv_mapping;
    k3fbxMapping material_mapping;
    k3fbxReference normal_reference;
    k3fbxReference uv_reference;
};

struct k3fbxModelData {
    uint64_t id;
    k3fbxObjType obj_type;
    uint32_t mesh_index;
    uint32_t material_index;  // used for light node index for lights
    float translation[3];
    float rotation[3];
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
    float translation[3];
    float look_at[3];
    float up[3];
    uint32_t res_x;
    uint32_t res_y;
    float fovx;
    float near_plane;
    float far_plane;
};

struct k3fbxLightNode {
    uint64_t id;
    uint32_t light_type;
    float color[3];
    float intensity;
    uint32_t decay_type;
    float decay_start;
    bool cast_shadows;
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
    uint32_t num_uv_indices;
    uint32_t num_normals;
    uint32_t num_normal_bytes;
    uint32_t num_uvs;
    uint32_t num_uv_bytes;
    uint32_t num_material_ids;
    uint32_t num_cameras;
    uint32_t num_light_nodes;
    uint32_t num_lights;
    k3fbxReference last_normal_reference;
    k3fbxReference last_uv_reference;
    k3fbxMeshData* mesh;
    k3fbxModelData* model;
    k3fbxMaterialData* material;
    k3fbxTextureData* texture;
    k3fbxContentData* content_data;
    k3fbxLightNode* light_node;
    uint32_t* indices;
    uint32_t* normal_indices;
    uint32_t* uv_indices;
    uint32_t* material_ids;
    k3fbxObjType node_attrib_obj;
    k3fbxCamera* camera;
    void* vertices;
    void* normals;
    void* uvs;
    char vert_type;
    char norm_type;
    char uv_type;
};

struct k3fbxAttrLinkList {
    uint32_t n_index;
    uint32_t uv_index;
    float* attr;
    k3fbxAttrLinkList* next;
};

static const uint64_t MAP_TYPE_DIFFUSE = 0;
static const uint64_t MAP_TYPE_NORMAL = 1;

uint32_t findFbxLightNode(k3fbxData* fbx, uint64_t id)
{
    uint32_t i;
    for (i = 0; i < fbx->num_light_nodes; i++) {
        if (fbx->light_node[i].id == id) break;
    }
    return (i < fbx->num_light_nodes) ? i : ~0x0;
}

uint32_t findFbxContentDataNode(k3fbxData* fbx, uint64_t id)
{
    uint32_t i;
    for (i = 0; i < fbx->num_content_data; i++) {
        if (fbx->content_data[i].id == id) break;
    }
    return (i < fbx->num_content_data) ? i : ~0x0;
}

uint32_t findFbxTextureNode(k3fbxData* fbx, uint64_t id)
{
    uint32_t i;
    for (i = 0; i < fbx->num_textures; i++) {
        if (fbx->texture[i].id == id) break;
    }
    return (i < fbx->num_textures) ? i : ~0x0;
}

uint32_t findFbxMaterialNode(k3fbxData* fbx, uint64_t id)
{
    uint32_t i;
    for (i = 0; i < fbx->num_materials; i++) {
        if (fbx->material[i].id == id) break;
    }
    return (i < fbx->num_materials) ? i : ~0x0;
}

uint32_t findFbxMeshNode(k3fbxData* fbx, uint64_t id)
{
    uint32_t i;
    for (i = 0; i < fbx->num_meshes; i++) {
        if (fbx->mesh[i].id == id) break;
    }
    return (i < fbx->num_meshes) ? i : ~0x0;
}

uint32_t findFbxModelNode(k3fbxData* fbx, uint64_t id)
{
    uint32_t i;
    for (i = 0; i < fbx->num_models; i++) {
        if (fbx->model[i].id == id) break;
    }
    return (i < fbx->num_models) ? i : ~0x0;
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
                fbx->model[index1].mesh_index = index0;
            }
            return;
        }
        // Check if id0 is a material
        index0 = findFbxMaterialNode(fbx, id[0]);
        if (index0 != ~0x0) {
            // id1 shoud be a model
            index1 = findFbxModelNode(fbx, id[1]);
            if (index1 != ~0x0) {
                fbx->model[index1].material_index = index0;
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
                case MAP_TYPE_DIFFUSE:
                    fbx->material[index1].diffuse_texture_index = index0;
                    break;
                case MAP_TYPE_NORMAL:
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
                fbx->model[index1].material_index = index0;
            }
        }
    }
}

void readFbxNode(k3fbxData* fbx, k3fbxNodeType parent_node, uint32_t level, FILE* in_file)
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
        fbx->num_uv_indices = 0;
        fbx->num_normals = 0;
        fbx->num_normal_bytes = 0;
        fbx->num_uvs = 0;
        fbx->num_uv_bytes = 0;
        fbx->num_material_ids = 0;
        fbx->num_cameras = 0;
        fbx->num_lights = 0;
        fbx->num_light_nodes = 0;
        fbx->node_attrib_obj = k3fbxObjType::NONE;
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
    uint32_t fbx_property_argument = 0;
    uint32_t fbx_node_argument = 0;
    uint64_t node_attrib_id;
    uint32_t str_file_pos;

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
            else if (!strncmp(str, "LayerElementUV", 15)) node_type = k3fbxNodeType::LAYER_ELEMENT_UV;
            else if (!strncmp(str, "LayerElementMaterial", 21)) node_type = k3fbxNodeType::LAYER_ELEMENT_MATERIAL;
            else if (!strncmp(str, "MappingInformationType", 23)) node_type = k3fbxNodeType::MAPPING_TYPE;
            else if (!strncmp(str, "ReferenceInformationType", 25)) node_type = k3fbxNodeType::REFERENCE_TYPE;
            else if (!strncmp(str, "Normals", 8)) node_type = k3fbxNodeType::NORMALS;
            else if (!strncmp(str, "NormalIndex", 12)) node_type = k3fbxNodeType::NORMAL_INDEX;
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
                fbx->mesh[fbx->num_meshes].uv_offset = fbx->num_uvs;
                fbx->mesh[fbx->num_meshes].uv_index_offset = fbx->num_uv_indices;
                fbx->mesh[fbx->num_meshes].material_offset = fbx->num_material_ids;
                fbx->mesh[fbx->num_meshes].normal_mapping = k3fbxMapping::None;
                fbx->mesh[fbx->num_meshes].uv_mapping = k3fbxMapping::None;
                fbx->mesh[fbx->num_meshes].material_mapping = k3fbxMapping::None;
                fbx->mesh[fbx->num_meshes].normal_reference = k3fbxReference::None;
                fbx->mesh[fbx->num_meshes].uv_reference = k3fbxReference::None;
            }
            fbx->num_meshes++;
            fbx_node_argument = 0;
        }

        if (node_type == k3fbxNodeType::MODEL) {
            if (fbx->model) {
                fbx->model[fbx->num_models].id = 0;
                fbx->model[fbx->num_models].obj_type = k3fbxObjType::NONE;
                fbx->model[fbx->num_models].mesh_index = ~0;
                fbx->model[fbx->num_models].material_index = ~0;
                fbx->model[fbx->num_models].translation[0] = 0.0f;
                fbx->model[fbx->num_models].translation[1] = 0.0f;
                fbx->model[fbx->num_models].translation[2] = 0.0f;
                fbx->model[fbx->num_models].rotation[0] = 0.0f;
                fbx->model[fbx->num_models].rotation[1] = 0.0f;
                fbx->model[fbx->num_models].rotation[2] = 0.0f;
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
                case k3fbxNodeType::UV:
                    fbx->uv_type = typecode;
                    if (fbx->uvs) data_arr = (char*)fbx->uvs + fbx->num_uv_bytes;
                    fbx->num_uv_bytes += ((typecode == 'd') ? 8 : 4) * arr_prop.array_length;
                    fbx->num_uvs += (arr_prop.array_length) / 2;
                    break;
                case k3fbxNodeType::MATERIAL_IDS:
                    if (fbx->material_ids) data_arr = fbx->material_ids + fbx->num_material_ids;
                    fbx->num_material_ids += arr_prop.array_length;
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
                    else if (!strncmp(str, "ByEdge", 7)) mapping = k3fbxMapping::ByEdge;
                    else if (!strncmp(str, "AllSame", 8)) mapping = k3fbxMapping::AllSame;
                    if (parent_node == k3fbxNodeType::LAYER_ELEMENT_NORMAL) {
                        if(fbx->mesh) fbx->mesh[fbx->num_meshes -1].normal_mapping = mapping;
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
                        } else if (K3_FBX_DEBUG) {
                            printf("%s\n", str);
                        }
                    }
                } else if (node_type == k3fbxNodeType::CONNECT) {
                    if (connect_params == 0) {
                        connect_params = (str[1] == 'P') ? 3 : 2;
                    } else {
                        if (!strncmp(str, "DiffuseColor", 13)) {
                            connect_id[connect_index] = MAP_TYPE_DIFFUSE;
                        } else if (!strncmp(str, "NormalMap", 10)) {
                            connect_id[connect_index] = MAP_TYPE_NORMAL;
                        }
                        connect_index++;
                        if (connect_index == connect_params) connectFbxNode(fbx, connect_id);
                    }
                } else if(node_type == k3fbxNodeType::NODE_ATTRIBUTE) {
                    if (fbx_node_argument == 2) {
                        // third argument is the type
                        if (!strncmp(str, "Camera", 7)) {
                            if (fbx->camera) {
                                fbx->camera[fbx->num_cameras].id = node_attrib_id;
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
                    if (fbx_node_argument == 2) {
                        if (!strncmp(str, "Light", 6)) {
                            fbx->num_lights++;
                            if (fbx->model) fbx->model[fbx->num_models - 1].obj_type = k3fbxObjType::LIGHT;
                        } else if (!strncmp(str, "Camera", 7)) {
                            if (fbx->model) fbx->model[fbx->num_models - 1].obj_type = k3fbxObjType::CAMERA;
                        } else if (!strncmp(str, "Mesh", 5)) {
                            if (fbx->model) fbx->model[fbx->num_models - 1].obj_type = k3fbxObjType::MESH;
                        } else {
                            if (fbx->model) fbx->model[fbx->num_models - 1].obj_type = k3fbxObjType::NONE;
                        }
                    }
                }
                if (fbx_node_argument) fbx_node_argument++;
                break;
            }
        }

        uint32_t byte_pos = ftell(in_file);
        if (node.end_offset - byte_pos > 0) {
            readFbxNode(fbx, node_type, level + 1, in_file);
            fseek(in_file, node.end_offset, SEEK_SET);
        }
    }
}

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
    readFbxNode(&fbx, k3fbxNodeType::UNKNOWN, 0, in_file);
    fbx.mesh = new k3fbxMeshData[fbx.num_meshes];
    fbx.model = new k3fbxModelData[fbx.num_models];
    if (fbx.num_materials) fbx.material = new k3fbxMaterialData[fbx.num_materials];
    if (fbx.num_textures) fbx.texture = new k3fbxTextureData[fbx.num_textures];
    if (fbx.num_content_data) fbx.content_data = new k3fbxContentData[fbx.num_content_data];
    fbx.vertices = new char[fbx.num_vertex_bytes];
    fbx.indices = new uint32_t[fbx.num_indices];
    if (fbx.num_normal_indices) fbx.normal_indices = new uint32_t[fbx.num_normal_indices];
    if (fbx.num_uv_indices) fbx.uv_indices = new uint32_t[fbx.num_uv_indices];
    if (fbx.num_material_ids) fbx.material_ids = new uint32_t[fbx.num_material_ids];
    if (fbx.num_cameras) fbx.camera = new k3fbxCamera[fbx.num_cameras];
    if (fbx.num_light_nodes) fbx.light_node = new k3fbxLightNode[fbx.num_light_nodes];
    fbx.normals = new char[fbx.num_normal_bytes];
    fbx.uvs = new char[fbx.num_uv_bytes];
    fseek(in_file, node_start, SEEK_SET);
    readFbxNode(&fbx, k3fbxNodeType::UNKNOWN, 0, in_file);

    // convert all double arrays to float arrays
    uint32_t num_verts = fbx.num_vertex_bytes / sizeof(float);
    if (fbx.vert_type == K3_FBX_TYPECODE_DOUBLE_ARRAY) {
        num_verts = fbx.num_vertex_bytes / sizeof(double);
        doubleToFloatArray(fbx.vertices, num_verts);
    }
    if (fbx.norm_type == K3_FBX_TYPECODE_DOUBLE_ARRAY) doubleToFloatArray(fbx.normals, fbx.num_normal_bytes / sizeof(double));
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
            uint32_t light_node_index = fbx.model[i].material_index;
            if (light_node_index != ~0x0) {
                k3fbxLightNode* light_node = &(fbx.light_node[light_node_index]);
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

        mesh_impl->_cameras[i].res_x = fbx.camera[i].res_x;
        mesh_impl->_cameras[i].res_y = fbx.camera[i].res_y;
        float aspect = fbx.camera[i].res_y / (float)fbx.camera[i].res_x;
        mesh_impl->_cameras[i].fovy = aspect * fbx.camera[i].fovx;
        mesh_impl->_cameras[i].near_plane = fbx.camera[i].near_plane;
        mesh_impl->_cameras[i].far_plane = fbx.camera[i].far_plane;
    }

    mesh_impl->_num_models = 0;  // determine which models have actual geometries...those are the only valid ones
    for (i = 0; i < fbx.num_models; i++) {
        uint32_t mesh_index = fbx.model[i].mesh_index;
        if (mesh_index != ~0 && fbx.model[i].obj_type == k3fbxObjType::MESH) mesh_impl->_num_models++;
    }

    mesh_impl->_model = new k3meshModel[mesh_impl->_num_models];
    mesh_impl->_num_models = 0;  // reset the count, and count again, but this time with allocated entries which are filled in
    float mat[16];
    float x_axis[3] = { 1.0f, 0.0f, 0.0f };
    float y_axis[3] = { 0.0f, 1.0f, 0.0f };
    float z_axis[3] = { 0.0f, 0.0f, 1.0f };
    uint32_t max_mesh_prims = 0;
    for (i = 0; i < fbx.num_models; i++) {
        uint32_t mesh_index = fbx.model[i].mesh_index;
        uint32_t mesh_start, num_mesh_prims;
        if (mesh_index != ~0 && fbx.model[i].obj_type == k3fbxObjType::MESH) {
            // this is a valid model
            mesh_start = mesh_impl->_mesh_start[mesh_index];
            num_mesh_prims = (mesh_index < mesh_impl->_num_meshes - 1) ? mesh_impl->_mesh_start[mesh_index + 1] : num_tris;
            num_mesh_prims -= mesh_start;
            if (num_mesh_prims > max_mesh_prims) max_mesh_prims = num_mesh_prims;
            mesh_impl->_model[mesh_impl->_num_models].mesh_index = mesh_index;
            mesh_impl->_model[mesh_impl->_num_models].prim_start = mesh_start;
            mesh_impl->_model[mesh_impl->_num_models].num_prims = num_mesh_prims;
            mesh_impl->_model[mesh_impl->_num_models].diffuse_color[0] = 1.0f;
            mesh_impl->_model[mesh_impl->_num_models].diffuse_color[1] = 1.0f;
            mesh_impl->_model[mesh_impl->_num_models].diffuse_color[2] = 1.0f;
            mesh_impl->_model[mesh_impl->_num_models].diffuse_map_index = ~0;
            mesh_impl->_model[mesh_impl->_num_models].normal_map_index = ~0;
            // Set initial model rotation and position
            k3m4_SetRotation(mesh_impl->_model[mesh_impl->_num_models].world_xform, -deg2rad(fbx.model[i].rotation[0]), x_axis);
            k3m4_SetRotation(mat, -deg2rad(fbx.model[i].rotation[1]), y_axis);
            k3m4_Mul(mesh_impl->_model[mesh_impl->_num_models].world_xform, mat, mesh_impl->_model[mesh_impl->_num_models].world_xform);
            k3m4_SetRotation(mat, -deg2rad(fbx.model[i].rotation[2]), z_axis);
            k3m4_Mul(mesh_impl->_model[mesh_impl->_num_models].world_xform, mat, mesh_impl->_model[mesh_impl->_num_models].world_xform);
            k3m4_SetIdentity(mat);
            mat[3] = fbx.model[i].translation[0];
            mat[7] = fbx.model[i].translation[1];
            mat[11] = fbx.model[i].translation[2];
            k3m4_Mul(mesh_impl->_model[mesh_impl->_num_models].world_xform, mat, mesh_impl->_model[mesh_impl->_num_models].world_xform);
            uint32_t material_index = fbx.model[i].material_index;
            if (material_index != ~0) {
                mesh_impl->_model[mesh_impl->_num_models].diffuse_color[0] = fbx.material[material_index].diffuse_color[0];
                mesh_impl->_model[mesh_impl->_num_models].diffuse_color[1] = fbx.material[material_index].diffuse_color[1];
                mesh_impl->_model[mesh_impl->_num_models].diffuse_color[2] = fbx.material[material_index].diffuse_color[2];
                uint32_t texture_index = fbx.material[material_index].diffuse_texture_index;
                if (texture_index != ~0) {
                    mesh_impl->_model[mesh_impl->_num_models].diffuse_map_index = texture_index;
                }
                texture_index = fbx.material[material_index].normal_texture_index;
                if (texture_index != ~0) {
                    mesh_impl->_model[mesh_impl->_num_models].normal_map_index = texture_index;
                }
            }
            mesh_impl->_num_models++;
        }
    }

    bool use_ib = (fbx.num_normal_indices == 0) && (fbx.num_uv_indices == 0);
    for (i = 0; i < fbx.num_meshes; i++) {
        use_ib = use_ib && (fbx.mesh[i].normal_mapping == k3fbxMapping::ByVert);
        use_ib = use_ib && (fbx.mesh[i].uv_mapping == k3fbxMapping::ByVert);
        use_ib = use_ib && (fbx.mesh[i].material_mapping == k3fbxMapping::AllSame);
    }
    if (!use_ib) num_verts = 3 * num_tris;


    uint32_t geom_size = (11 * num_verts * sizeof(float)) + ((use_ib) ? 3 * num_tris * sizeof(uint32_t) : 0);
    uint32_t lbuf_size = mesh_impl->_num_lights * sizeof(k3lightBufferData);
    uint32_t up_buf_size = geom_size + lbuf_size;
    float* verts = (float*)desc->up_buf->MapForWrite(up_buf_size);
    float* attrs = verts + 3 * num_verts;

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

    float* fbx_verts = (float*)fbx.vertices;
    float* fbx_normals = (float*)fbx.normals;
    float* fbx_uvs = (float*)fbx.uvs;
    uint32_t ipos = 0, poly_start;
    uint32_t poly = 0;
    // these restart per object
    uint32_t local_ipos = 0, local_poly_start;
    uint32_t local_poly = 0;
    uint32_t v_index, n_index, uv_index;
    bool end_poly;
    uint32_t i0, i1, i2, j;
    float bitangent[3];
    o = 0;
    if(use_ib) {
        uint32_t* indices = (uint32_t*)(verts + 11 * num_verts);
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
            // TODO: compute tangents
            attrs[8 * i + 5] = 1.0f;
            attrs[8 * i + 6] = 0.0f;
            attrs[8 * i + 7] = 0.0f;
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
    } else {
        poly_start = ipos;
        local_poly_start = local_ipos;
        uint32_t obj_v_index[3];
        uint32_t obj_n_index[3];
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
            attrs[24 * i + 5] = 1.0f;
            attrs[24 * i + 6] = 0.0f;
            attrs[24 * i + 7] = 0.0f;
            v_index = fbx.indices[ipos + 1];
            obj_v_index[1] = v_index;
            v_index += fbx.mesh[o].vert_offset;
            verts[9 * i + 3] = fbx_verts[3 * v_index + 0];
            verts[9 * i + 4] = fbx_verts[3 * v_index + 1];
            verts[9 * i + 5] = fbx_verts[3 * v_index + 2];
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
            attrs[24 * i + 13] = 1.0f;
            attrs[24 * i + 14] = 0.0f;
            attrs[24 * i + 15] = 0.0f;
            v_index = fbx.indices[ipos + 2];
            end_poly = (v_index & 0x80000000) ? true : false;
            if (end_poly) v_index = ~v_index;
            obj_v_index[2] = v_index;
            v_index += fbx.mesh[o].vert_offset;
            verts[9 * i + 6] = fbx_verts[3 * v_index + 0];
            verts[9 * i + 7] = fbx_verts[3 * v_index + 1];
            verts[9 * i + 8] = fbx_verts[3 * v_index + 2];
            switch (fbx.mesh[o].normal_mapping) {
            case k3fbxMapping::None: n_index = 0; break;
            case k3fbxMapping::ByPoly: n_index = local_poly; break;
            case k3fbxMapping::ByPolyVert: n_index = local_ipos + 2; break;
            case k3fbxMapping::ByVert: n_index = fbx.indices[ipos + 2]; break;
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
            switch (fbx.mesh[o].uv_mapping) {
            case k3fbxMapping::None: uv_index = 0; break;
            case k3fbxMapping::ByPoly: uv_index = local_poly; break;
            case k3fbxMapping::ByPolyVert: uv_index = local_ipos + 2; break;
            case k3fbxMapping::ByVert: uv_index = fbx.indices[ipos + 2]; break;
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
            attrs[24 * i + 21] = 1.0f;
            attrs[24 * i + 22] = 0.0f;
            attrs[24 * i + 23] = 0.0f;
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
        delete[] attr_ll;
        delete[] attr_nodes;
    }

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
    if (fbx.light_node) delete[] fbx.light_node;
    if (fbx.camera) delete[] fbx.camera;
    if (fbx.material) delete[] fbx.material;
    if (fbx.texture) delete[] fbx.texture;
    if (fbx.content_data) delete[] fbx.content_data;
    delete[] fbx.vertices;
    if (fbx.uv_indices) delete[] fbx.uv_indices;
    if (fbx.normal_indices) delete[] fbx.normal_indices;
    if (fbx.material_ids) delete[] fbx.material_ids;
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
