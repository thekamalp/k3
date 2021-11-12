// k3 graphics library
// image class
// Date: 10/10/2021

#include "k3internal.h"
#include "ddshandler.h"

uint32_t k3imageObj::_num_file_handlers = 1;
k3image_file_handler_t* k3imageObj::_fh[MAX_FILE_HANDLERS] = { &k3DDSHandler };

K3API uint32_t k3imageObj::AddImageFileHandler(k3image_file_handler_t* fh)
{
    if (_num_file_handlers >= k3imageObj::MAX_FILE_HANDLERS) {
        k3error::Handler("Too many file handlers", "AddImageFileHandler");
        return ~0;
    }
    _fh[_num_file_handlers] = fh;
    _num_file_handlers++;
    return _num_file_handlers - 1;
}

K3API void k3imageObj::RemoveImageFileHandler(k3image_file_handler_t* fh)
{
    bool found = false;
    uint32_t i;
    for (i = 0; i < _num_file_handlers; i++) {
        if (!found && fh == _fh[i]) found = true;
        if (found && i < _num_file_handlers - 1) _fh[i] = _fh[i + 1];
    }
    if (found) _num_file_handlers--;
    else {
        k3error::Handler("FIle handler not found", "RemoveImageFileHandler");
    }
}

K3API k3image k3imageObj::Create()
{
    k3image img = new k3imageObj;
    return img;
}

K3API void k3imageObj::ReformatFromImage(k3image img, k3image src,
    uint32_t dest_width, uint32_t dest_height, uint32_t dest_depth,
    k3fmt dest_format, const float* transform,
    k3texAddr x_addr_mode, k3texAddr y_addr_mode, k3texAddr z_addr_mode)
{
    uint32_t src_width = src->GetWidth();
    uint32_t src_height = src->GetHeight();
    uint32_t src_depth = src->GetDepth();
    uint32_t src_pitch = src->GetPitch();
    uint32_t src_slice_pitch = src->GetSlicePitch();
    k3fmt src_format = src->GetFormat();
    const void* src_data = src->MapForRead();

    if (src_data) {
        k3imageObj::ReformatFromMemory(img, src_width, src_height, src_depth, src_pitch, src_slice_pitch, src_format, src_data,
            dest_width, dest_height, dest_depth, dest_format, transform, x_addr_mode, y_addr_mode, z_addr_mode);
    }

    src->Unmap();
}

K3API void k3imageObj::ReformatFromFile(k3image img, const char* file_name,
    uint32_t dest_width, uint32_t dest_height, uint32_t dest_depth,
    k3fmt dest_format, const float* transform,
    k3texAddr x_addr_mode, k3texAddr y_addr_mode, k3texAddr z_addr_mode)
{
    uint32_t src_width, src_height, src_depth;
    k3fmt src_format = k3fmt::UNKNOWN;
    uint8_t* src_data_byte_ptr = NULL;
    void* src_data;
    uint32_t fhi;

    FILE* file_handle;
    fopen_s(&file_handle,  file_name, "rb");
    if (file_handle == NULL) {
        k3error::Handler("File not found", "ReformatFromFile");
    } else {
        for (fhi = 0; fhi < _num_file_handlers; fhi++) {
            _fh[fhi]->LoadHeaderInfo(file_handle, &src_width, &src_height, &src_depth, &src_format);
            if (src_format != k3fmt::UNKNOWN) break;
        }
        if (src_format != k3fmt::UNKNOWN) {
            if (dest_width == 0) dest_width = src_width;
            if (dest_height == 0) dest_height = src_height;
            if (dest_depth == 0) dest_depth = src_depth;
            if (dest_format == k3fmt::UNKNOWN) dest_format = src_format;
            uint32_t src_format_size = k3imageObj::GetFormatSize(src_format);
            img->SetDimensions(dest_width, dest_height, dest_depth, dest_format);
            uint32_t dest_pitch = img->GetPitch();
            uint32_t dest_slice_pitch = img->GetSlicePitch();
            uint32_t src_pitch = (src_width == dest_width) ? dest_pitch : src_width * src_format_size;
            uint32_t src_slice_pitch = (src_pitch == dest_pitch && src_height == dest_height) ? dest_slice_pitch : src_pitch * src_height;
            
            uint32_t src_image_size = src_depth * src_slice_pitch;
            uint32_t dest_image_size = dest_depth * dest_slice_pitch;
            bool inplace = (src_width == dest_width && src_height == dest_height &&
                src_depth == dest_depth && src_image_size == dest_image_size && transform == NULL &&
                src_pitch == dest_pitch && src_slice_pitch == dest_slice_pitch);

            if (inplace) {
                src_data = img->MapForWrite();
            } else {
                src_data_byte_ptr = new uint8_t[src_image_size];
                src_data = static_cast<void*>(src_data_byte_ptr);
            } // if( inplace )

            _fh[fhi]->LoadData(file_handle, src_pitch, src_slice_pitch, src_data);

            if (inplace) {
                if (src_format != dest_format) k3imageObj::ReformatBuffer(src_width, src_height, src_depth,
                    src_pitch, src_slice_pitch,
                    src_format, src_data,
                    dest_width, dest_height, dest_depth,
                    dest_pitch, dest_slice_pitch,
                    dest_format, src_data, transform,
                    x_addr_mode, y_addr_mode, z_addr_mode);
                img->Unmap();
            } else {
                k3imageObj::ReformatFromMemory(img, src_width, src_height, src_depth, src_pitch, src_slice_pitch, src_format, src_data,
                    dest_width, dest_height, dest_depth, dest_format, transform,
                    x_addr_mode, y_addr_mode, z_addr_mode);
                delete[] src_data_byte_ptr;
            } // if( inplace )
        } // if (src_format != k3fmt::UNKNOWN)
    }
}

K3API void k3imageObj::ReformatFromMemory(k3image img, uint32_t src_width, uint32_t src_height, uint32_t src_depth,
    uint32_t src_pitch, uint32_t src_slice_pitch,
    k3fmt src_format, const void* src_data,
    uint32_t dest_width, uint32_t dest_height, uint32_t dest_depth,
    k3fmt dest_format, const float* transform,
    k3texAddr x_addr_mode, k3texAddr y_addr_mode, k3texAddr z_addr_mode)
{
    if (dest_width == 0)  dest_width = src_width;
    if (dest_height == 0) dest_height = src_height;
    if (dest_depth == 0)  dest_depth = src_depth;
    if (dest_format == k3fmt::UNKNOWN) dest_format = src_format;

    if (img == NULL) {
        k3error::Handler("No image passed in", "ReformatFromMemory");
        return;
    }
    img->SetDimensions(dest_width, dest_height, dest_depth, dest_format);
    uint32_t dest_pitch = img->GetPitch();
    uint32_t dest_slice_pitch = img->GetSlicePitch();

    if (src_data) {

        void* dest_data = img->MapForWrite();

        if (dest_data == NULL) {
            return;
        }

        k3imageObj::ReformatBuffer(src_width, src_height, src_depth, src_pitch, src_slice_pitch, src_format, src_data,
            dest_width, dest_height, dest_depth, dest_pitch, dest_slice_pitch, dest_format, dest_data,
            transform, x_addr_mode, y_addr_mode, z_addr_mode);
        img->Unmap();
    }
}

k3imageObj::k3imageObj()
{
    _data = new k3imageImpl;
    memset(_data, 0, sizeof(k3imageImpl));
}

k3imageObj::~k3imageObj()
{
    if (_data->_image_data) {
        delete _data->_image_data;
        _data->_image_data = NULL;
    }
    delete _data;
    _data = NULL;
}

K3API void k3imageObj::SetDimensions(uint32_t width, uint32_t height, uint32_t depth, k3fmt format)
{
    _data->_width = width;
    _data->_height = height;
    _data->_depth = depth;
    _data->_fmt = format;
}

K3API uint32_t k3imageObj::GetWidth() const
{
    return _data->_width;
}

K3API uint32_t k3imageObj::GetHeight() const
{
    return _data->_height;
}

K3API uint32_t k3imageObj::GetDepth() const
{
    return _data->_depth;
}

K3API k3fmt k3imageObj::GetFormat() const
{
    return _data->_fmt;
}

K3API uint32_t k3imageObj::GetPitch() const
{
    uint32_t pitch_pad = _data->_pitch_pad;
    if (pitch_pad == 0) pitch_pad = 1;
    uint32_t format_size = GetFormatSize(_data->_fmt);
    uint32_t block_size = GetFormatBlockSize(_data->_fmt);
    uint32_t pitch = (_data->_width + block_size - 1) / block_size;
    pitch *= format_size;
    pitch += pitch_pad - 1;
    pitch /= pitch_pad;
    pitch *= pitch_pad;
    return pitch;
}

K3API uint32_t k3imageObj::GetSlicePitch() const
{
    uint32_t slice_pitch_pad = _data->_slice_pitch_pad;
    if (slice_pitch_pad == 0) slice_pitch_pad = 1;
    uint32_t pitch = GetPitch();
    uint32_t block_size = GetFormatBlockSize(_data->_fmt);
    uint32_t height = (_data->_height + block_size - 1) / block_size;
    uint32_t slice_pitch = pitch * height;
    slice_pitch += slice_pitch_pad - 1;
    slice_pitch /= slice_pitch_pad;
    slice_pitch *= slice_pitch_pad;
    return slice_pitch;
}


K3API void k3imageObj::SaveToFile(const char* file_name, uint32_t fh_index)
{
    k3image_file_handler_t* fh = _fh[fh_index];

    if (fh == NULL) {
        k3error::Handler("Illegal file handler index", "SaveToFile");
        return;
    }

    FILE* file_handle;
    fopen_s(&file_handle, file_name, "wb");
    if (file_handle == NULL) {
        k3error::Handler("Could not open file", "SaveToFile");
        return;
    }

    const void* data = MapForRead();
    if (data == NULL) {
        k3error::Handler("Could not map image for read", "SaveToFile");
        fclose(file_handle);
        return;
    }
    uint32_t pitch = GetPitch();
    uint32_t slice_pitch = GetSlicePitch();
    fh->SaveData(file_handle, _data->_width, _data->_height, _data->_depth, pitch, slice_pitch, _data->_fmt, data);
    fclose(file_handle);
}

K3API const void* k3imageObj::MapForRead()
{
    return _data->_image_data;
}

K3API void* k3imageObj::MapForWrite()
{
    uint32_t size = _data->_depth * GetSlicePitch();
    if (_data->_image_data == NULL) {
        _data->_size = 0;
    }
    if (_data->_size < size) {
        if (_data->_image_data) delete[] _data->_image_data;
        _data->_size = size;
        _data->_image_data = static_cast<void*>(new uint8_t[_data->_size]);
    }
    return _data->_image_data;
}

K3API void k3imageObj::Unmap()
{ }

K3API void k3imageObj::SampleImage(float* color,
    float x, float y, float z,
    k3texAddr x_addr_mode, k3texAddr y_addr_mode, k3texAddr z_addr_mode)
{
    const void* data = MapForRead();
    uint32_t pitch = GetPitch();
    uint32_t slice_pitch = GetSlicePitch();
    SampleBuffer(x, y, z, _data->_width, _data->_height, _data->_depth, pitch, slice_pitch, _data->_fmt,
        data, color, x_addr_mode, y_addr_mode, z_addr_mode);
    Unmap();
}

void k3imageObj::InterpolateFloat(uint32_t num_channels, const float* src0, const float* src1, uint32_t levels, float* out)
{
    uint32_t i, c;
    float w1, w2;

    for (i = 0; i < levels; i++) {
        w2 = (i + 1) / static_cast<float>(levels + 1);
        w1 = 1.0f - w2;
        for (c = 0; c < num_channels; c++) {
            out[num_channels * i + c] = (w1 * src0[c]) + (w2 * src1[c]);
        }
    }
}

void k3imageObj::DecompressDXT1Palette(const k3DXT1Block* src, float* palette, bool allow_alpha)
{
    ConvertToFloat4(k3fmt::B5G6R5_UNORM, &(src->color0), &(palette[0]));
    ConvertToFloat4(k3fmt::B5G6R5_UNORM, &(src->color1), &(palette[4]));

    if (src->color0 > src->color1 || !allow_alpha) {
        InterpolateFloat(4, &(palette[0]), &(palette[4]), 2, &(palette[8]));
    } else {
        InterpolateFloat(4, &(palette[0]), &(palette[4]), 1, &(palette[8]));
        palette[12] = 0.0f; palette[13] = 0.0f; palette[14] = 0.0f; palette[15] = 0.0f;
    }

}

void k3imageObj::DecompressDXT1Block(const k3DXT1Block* src, float* dest, bool allow_alpha)
{
    float palette[16];
    DecompressDXT1Palette(src, palette, allow_alpha);

    uint32_t i, c, palselect, pixmap = src->pixmap;
    float* cur_dest = dest;
    for (i = 0; i < 16; i++) {
        palselect = 4 * (pixmap & 0x3);
        for (c = 0; c < 4; c++) {
            *cur_dest = palette[palselect + c];
            cur_dest++;
        }
        pixmap = pixmap >> 2;
    }
}

void k3imageObj::DecompressDXT3Block(const k3DXT3Block* src, float* dest)
{
    uint32_t i;
    float* cur_alpha = dest + 3;
    DecompressDXT1Block(&(src->dxt1), dest, false);

    for (i = 0; i < 16; i++) {
        *cur_alpha = ((src->alphas >> (4 * i)) & 0xf) / static_cast<float>(0xf);
        cur_alpha += 4;
    }
}

void k3imageObj::DecompressDXT5Block(const k3DXT3Block* src, float* dest)
{
    float* cur_alpha = dest + 3;
    DecompressDXT1Block(&(src->dxt1), dest, false);
    DecompressATI1NBlock(&(src->alphas), cur_alpha, false);
}

void k3imageObj::DecompressATI1NBlock(const uint64_t* src, float* dest, bool clear_others)
{
    uint32_t i;
    uint8_t ialpha0 = static_cast<uint8_t>((*src >> 0) & 0xff);
    uint8_t ialpha1 = static_cast<uint8_t>((*src >> 8) & 0xff);
    uint64_t alphamap = (*src >> 16);
    uint32_t palselect;
    float alpha_palette[8];

    alpha_palette[0] = ialpha0 / static_cast<float>(0xff);
    alpha_palette[1] = ialpha1 / static_cast<float>(0xff);
    if (ialpha0 > ialpha1) {
        InterpolateFloat(1, &(alpha_palette[0]), &(alpha_palette[1]), 6, &(alpha_palette[2]));
    } else {
        InterpolateFloat(1, &(alpha_palette[0]), &(alpha_palette[1]), 4, &(alpha_palette[2]));
        alpha_palette[6] = 0.0f;
        alpha_palette[7] = 1.0f;
    }

    for (i = 0; i < 16; i++) {
        palselect = static_cast<uint32_t>(alphamap & 0x7);
        *dest = alpha_palette[palselect];
        if (clear_others) {
            dest++; *dest = 0.0f;
            dest++; *dest = 0.0f;
            dest++; *dest = 0.0f;
            dest++;
        } else {
            dest += 4;
        }
        alphamap = alphamap >> 3;
    }
}

void k3imageObj::DecompressATI2NBlock(const k3ATI2NBlock* src, float* dest)
{
    float* reds = dest;
    float* greens = dest + 1;
    DecompressATI1NBlock(&(src->reds), reds, true);
    DecompressATI1NBlock(&(src->greens), greens, false);
}

float k3imageObj::CalcRGBLuminance(const float* src)
{
    return src[0] + 2.0f * src[1] + src[2];
}

float k3imageObj::CompressDXT1Pixmap(const float** palette, const float* src, uint32_t* dest)
{
    uint32_t i, p, c, sel;
    const float* cur_color = src;
    const float* cur_pal;
    float cur_err, pix_err, tot_err = 0.0f;
    *dest = 0;

    for (i = 0; i < 16; i++) { // iterate through all pixels in the block
        pix_err = FLT_MAX;
        sel = 0;
        for (p = 0; p < 4; p++) { // iterate through all colors in palette
            cur_err = 0.0f;
            cur_pal = palette[p];
            for (c = 0; c < 4; c++) { // iterate through all channels of the color
                float diff = cur_color[c] - cur_pal[c];
                diff = (diff < 0) ? -diff : diff;
                cur_err += diff;
            }
            if (cur_err < pix_err) {
                sel = p;
                pix_err = cur_err;
            }
        }
        *dest |= sel << (2 * i);
        tot_err += pix_err;
        cur_color += 4;
    }

    return tot_err;
}

float k3imageObj::CompressATI1NPixmap(const float** palette, const float* src, uint64_t* dest)
{
    uint32_t i, p, sel;
    float cur_err, pix_err, tot_err = 0.0f;
    const float* cur_alpha = src;
    *dest = 0;

    for (i = 0; i < 16; i++) {
        pix_err = FLT_MAX;
        sel = 0;
        for (p = 0; p < 8; p++) {
            float diff = *(palette[p]) - *cur_alpha;
            diff = (diff < 0) ? -diff : diff;
            cur_err = diff;
            if (cur_err < pix_err) {
                sel = p;
                pix_err = cur_err;
            }
        }
        *dest |= static_cast<uint64_t>(sel) << (3 * i);
        tot_err += pix_err;
        cur_alpha += 4;
    }

    return tot_err;
}

void k3imageObj::CompressDXT1Block(const float* src, k3DXT1Block* dest, bool allow_alpha)
{
    const float* min_color = src;
    const float* max_color = src;
    float min_luminance = FLT_MAX, max_luminance = -1.0f, cur_luminance;
    const float* cur_color = src;
    uint32_t i;
    uint32_t pixmap0 = 0;
    uint32_t pixmap1 = 0;
    float err0 = 0.0f;
    float err1 = 0.0f;
    float other_colors[8];
    const float* palette[4];
    uint16_t imin_color, imax_color;

    for (i = 0; i < 16; i++) {
        cur_luminance = CalcRGBLuminance(cur_color);
        if (cur_luminance < min_luminance) {
            min_color = cur_color;
            min_luminance = cur_luminance;
        }
        if (cur_luminance > max_luminance) {
            max_color = cur_color;
            max_luminance = cur_luminance;
        }
        cur_color += 4;
    }

    ConvertFromFloat4(k3fmt::B5G6R5_UNORM, min_color, &imin_color);
    ConvertFromFloat4(k3fmt::B5G6R5_UNORM, max_color, &imax_color);

    palette[2] = &(other_colors[0]);
    palette[3] = &(other_colors[4]);

    if ((imin_color > imax_color) || !allow_alpha) {
        palette[0] = min_color;
        palette[1] = max_color;
        InterpolateFloat(4, min_color, max_color, 2, other_colors);
    } else {
        palette[0] = min_color;
        palette[1] = max_color;
        InterpolateFloat(4, min_color, max_color, 1, other_colors);
        other_colors[4] = 0.0f; other_colors[5] = 0.0f; other_colors[6] = 0.0f; other_colors[7] = 0.0f;
    }
    err0 = CompressDXT1Pixmap(palette, src, &pixmap0);

    if ((imin_color >= imax_color) && allow_alpha) {
        palette[0] = max_color;
        palette[1] = min_color;
        InterpolateFloat(4, min_color, max_color, 1, other_colors);
        other_colors[4] = 0.0f; other_colors[5] = 0.0f; other_colors[6] = 0.0f; other_colors[7] = 0.0f;
    } else {
        palette[0] = max_color;
        palette[1] = min_color;
        InterpolateFloat(4, max_color, min_color, 2, other_colors);
    }
    err1 = CompressDXT1Pixmap(palette, src, &pixmap1);

    if (err0 < err1) {
        dest->color0 = imin_color;
        dest->color1 = imax_color;
        dest->pixmap = pixmap0;
    } else {
        dest->color0 = imax_color;
        dest->color1 = imin_color;
        dest->pixmap = pixmap1;
    }
}

void k3imageObj::CompressDXT3Block(const float* src, k3DXT3Block* dest)
{
    uint32_t i;
    const float* cur_alpha = src + 3;
    CompressDXT1Block(src, &(dest->dxt1), true);

    dest->alphas = 0;
    for (i = 0; i < 16; i++) {
        dest->alphas |= static_cast<uint64_t>(*cur_alpha * 0xf) << (4 * i);
        cur_alpha += 4;
    }
}

void k3imageObj::CompressDXT5Block(const float* src, k3DXT3Block* dest)
{
    const float* cur_alpha = src + 3;
    CompressDXT1Block(src, &(dest->dxt1), true);
    CompressATI1NBlock(cur_alpha, &(dest->alphas));
}

void k3imageObj::CompressATI1NBlock(const float* src, uint64_t* dest)
{
    uint32_t i;
    const float* cur_alpha = src;
    const float* min_alpha = cur_alpha;
    const float* max_alpha = cur_alpha;
    uint8_t imin_alpha, imax_alpha;
    float other_alphas[6];
    float err0, err1;
    uint64_t alphamap0, alphamap1;
    const float* palette[8];

    for (i = 0; i < 16; i++) {
        if (*cur_alpha < *min_alpha) min_alpha = cur_alpha;
        if (*cur_alpha > *max_alpha) max_alpha = cur_alpha;
        cur_alpha += 4;
    }

    imin_alpha = static_cast<uint8_t>(*min_alpha * 0xff);
    imax_alpha = static_cast<uint8_t>(*max_alpha * 0xff);

    palette[2] = &(other_alphas[0]);
    palette[3] = &(other_alphas[1]);
    palette[4] = &(other_alphas[2]);
    palette[5] = &(other_alphas[3]);
    palette[6] = &(other_alphas[4]);
    palette[7] = &(other_alphas[5]);

    palette[0] = min_alpha;
    palette[1] = max_alpha;
    InterpolateFloat(1, min_alpha, max_alpha, 4, other_alphas);
    other_alphas[4] = 0.0f;
    other_alphas[5] = 1.0f;
    err0 = CompressATI1NPixmap(palette, src, &alphamap0);

    palette[0] = max_alpha;
    palette[1] = min_alpha;
    InterpolateFloat(1, max_alpha, min_alpha, 6, other_alphas);
    err1 = CompressATI1NPixmap(palette, src, &alphamap1);

    if (err0 <= err1 || imin_alpha == imax_alpha) {
        *dest = (alphamap0 << 16) | (static_cast<uint64_t>(imax_alpha) << 8) | imin_alpha;
    } else {
        *dest = (alphamap1 << 16) | (static_cast<uint64_t>(imin_alpha) << 8) | imax_alpha;
    }
}

void k3imageObj::CompressATI2NBlock(const float* src, k3ATI2NBlock* dest)
{
    const float* reds = src;
    const float* greens = src + 1;
    CompressATI1NBlock(reds, &(dest->reds));
    CompressATI1NBlock(greens, &(dest->greens));
}

void k3imageObj::InterpolateUnorm8(uint32_t num_channels, const uint8_t* src0, const uint8_t* src1, uint8_t levels, uint8_t* out)
{
    uint8_t i, c;
    uint8_t w1, w2;
    uint8_t div = levels + 1;

    for (i = 0; i < levels; i++) {
        w2 = (i + 1);
        w1 = div - w2;
        for (c = 0; c < num_channels; c++) {
            out[num_channels * i + c] = ((w1 * src0[c]) + (w2 * src1[c])) / div;
        }
    }
}

void k3imageObj::DecompressDXT1Palette(const k3DXT1Block* src, uint8_t* palette, bool allow_alpha)
{
    ConvertToUnorm8(k3fmt::B5G6R5_UNORM, &(src->color0), &(palette[0]));
    ConvertToUnorm8(k3fmt::B5G6R5_UNORM, &(src->color1), &(palette[4]));

    if (src->color0 > src->color1 || !allow_alpha) {
        InterpolateUnorm8(4, &(palette[0]), &(palette[4]), 2, &(palette[8]));
    } else {
        InterpolateUnorm8(4, &(palette[0]), &(palette[4]), 1, &(palette[8]));
        palette[12] = 0; palette[13] = 0; palette[14] = 0; palette[15] = 0;
    }
}

void k3imageObj::DecompressDXT1Block(const k3DXT1Block* src, uint8_t* dest, bool allow_alpha)
{
    uint8_t palette[16];
    DecompressDXT1Palette(src, palette, allow_alpha);

    uint32_t i, c, palselect, pixmap = src->pixmap;
    uint8_t* cur_dest = dest;
    for (i = 0; i < 16; i++) {
        palselect = 4 * (pixmap & 0x3);
        for (c = 0; c < 4; c++) {
            *cur_dest = palette[palselect + c];
            cur_dest++;
        }
        pixmap = pixmap >> 2;
    }
}

void k3imageObj::DecompressDXT3Block(const k3DXT3Block* src, uint8_t* dest)
{
    uint32_t i;
    uint8_t alpha_val;
    uint8_t* cur_alpha = dest + 3;
    DecompressDXT1Block(&(src->dxt1), dest, false);

    for (i = 0; i < 16; i++) {
        alpha_val = ((src->alphas >> (4 * i)) & 0xf);
        *cur_alpha = (alpha_val << 4) | alpha_val;
        cur_alpha += 4;
    }
}

void k3imageObj::DecompressDXT5Block(const k3DXT3Block* src, uint8_t* dest)
{
    uint8_t* cur_alpha = dest + 3;
    DecompressDXT1Block(&(src->dxt1), dest, false);
    DecompressATI1NBlock(&(src->alphas), cur_alpha, false);
}

void k3imageObj::DecompressATI1NBlock(const uint64_t* src, uint8_t* dest, bool clear_others)
{
    uint32_t i;
    uint8_t ialpha0 = static_cast<uint8_t>((*src >> 0) & 0xff);
    uint8_t ialpha1 = static_cast<uint8_t>((*src >> 8) & 0xff);
    uint64_t alphamap = (*src >> 16);
    uint32_t palselect;
    uint8_t alpha_palette[8];

    alpha_palette[0] = ialpha0;
    alpha_palette[1] = ialpha1;
    if (ialpha0 > ialpha1) {
        InterpolateUnorm8(1, &(alpha_palette[0]), &(alpha_palette[1]), 6, &(alpha_palette[2]));
    } else {
        InterpolateUnorm8(1, &(alpha_palette[0]), &(alpha_palette[1]), 4, &(alpha_palette[2]));
        alpha_palette[6] = 0x00;
        alpha_palette[7] = 0xff;
    }

    for (i = 0; i < 16; i++) {
        palselect = static_cast<uint32_t>(alphamap & 0x7);
        *dest = alpha_palette[palselect];
        if (clear_others) {
            *(dest + 1) = 0x00;
            *(dest + 2) = 0x00;
            *(dest + 3) = 0x00;
        }
        dest += 4;
        alphamap = alphamap >> 3;
    }
}

void k3imageObj::DecompressATI2NBlock(const k3ATI2NBlock* src, uint8_t* dest)
{
    uint8_t* reds = dest;
    uint8_t* greens = dest + 1;
    DecompressATI1NBlock(&(src->reds), reds, true);
    DecompressATI1NBlock(&(src->greens), greens, false);
}

uint32_t k3imageObj::CalcRGBLuminance(const uint8_t* src)
{
    return src[0] + 2 * static_cast<uint32_t>(src[1]) + src[2];
}

uint32_t k3imageObj::CompressDXT1Pixmap(const uint8_t** palette, const uint8_t* src, uint32_t* dest)
{
    uint32_t i, p, sel;
    const uint8_t* cur_color = src;
    const uint8_t* cur_pal;
    uint32_t cur_err, pix_err, tot_err = 0;
    *dest = 0;

    for (i = 0; i < 2 * 16; i += 2) { // iterate through all pixels in the block
        pix_err = 0xffffffff;
        sel = 0;
        for (p = 0; p < 4; p++) { // iterate through all colors in palette
            cur_pal = palette[p];
            cur_err = abs(*(cur_color + 0) - *(cur_pal + 0));
            cur_err += abs(*(cur_color + 1) - *(cur_pal + 1));
            cur_err += abs(*(cur_color + 2) - *(cur_pal + 2));
            cur_err += abs(*(cur_color + 3) - *(cur_pal + 3));
            if (cur_err < pix_err) {
                sel = p;
                pix_err = cur_err;
            }
        }
        *dest |= sel << (i);
        tot_err += pix_err;
        cur_color += 4;
    }

    return tot_err;
}

uint32_t k3imageObj::CompressATI1NPixmap(const uint8_t** palette, const uint8_t* src, uint64_t* dest)
{
    uint32_t i, p, sel;
    uint8_t cur_err, pix_err;
    uint32_t tot_err = 0;
    const uint8_t* cur_alpha = src;
    *dest = 0;

    for (i = 0; i < 3 * 16; i += 3) {
        pix_err = 0xff;
        sel = 0;
        for (p = 0; p < 8; p++) {
            cur_err = (*(palette[p]) > *cur_alpha) ? *(palette[p]) - *cur_alpha : *cur_alpha - *(palette[p]);
            if (cur_err < pix_err) {
                sel = p;
                pix_err = cur_err;
            }
        }
        *dest |= static_cast<uint64_t>(sel) << (i);
        tot_err += pix_err;
        cur_alpha += 4;
    }

    return tot_err;
}

void k3imageObj::CompressDXT1Block(const uint8_t* src, k3DXT1Block* dest, bool allow_alpha)
{
    const uint8_t* min_color = src;
    const uint8_t* max_color = src;
    uint32_t min_luminance = 0xffffffff, max_luminance = 0x00, cur_luminance;
    const uint8_t* cur_color = src;
    uint32_t i;
    uint32_t pixmap0 = 0;
    uint32_t pixmap1 = 0;
    uint32_t err0 = 0;
    uint32_t err1 = 0;
    uint8_t other_colors[8];
    const uint8_t* palette[4];
    uint16_t imin_color, imax_color;

    for (i = 0; i < 16; i++) {
        cur_luminance = CalcRGBLuminance(cur_color);
        if (cur_luminance < min_luminance) {
            min_color = cur_color;
            min_luminance = cur_luminance;
        }
        if (cur_luminance > max_luminance) {
            max_color = cur_color;
            max_luminance = cur_luminance;
        }
        cur_color += 4;
    }

    ConvertFromUnorm8(k3fmt::B5G6R5_UNORM, min_color, &imin_color);
    ConvertFromUnorm8(k3fmt::B5G6R5_UNORM, max_color, &imax_color);

    palette[2] = &(other_colors[0]);
    palette[3] = &(other_colors[4]);

    if ((imin_color > imax_color) || !allow_alpha) {
        palette[0] = min_color;
        palette[1] = max_color;
        InterpolateUnorm8(4, min_color, max_color, 2, other_colors);
    } else {
        palette[0] = min_color;
        palette[1] = max_color;
        InterpolateUnorm8(4, min_color, max_color, 1, other_colors);
        other_colors[4] = 0; other_colors[5] = 0; other_colors[6] = 0; other_colors[7] = 0;
    }
    err0 = CompressDXT1Pixmap(palette, src, &pixmap0);

    if ((imin_color >= imax_color) && allow_alpha) {
        palette[0] = max_color;
        palette[1] = min_color;
        InterpolateUnorm8(4, min_color, max_color, 1, other_colors);
        other_colors[4] = 0; other_colors[5] = 0; other_colors[6] = 0; other_colors[7] = 0;
    } else {
        palette[0] = max_color;
        palette[1] = min_color;
        InterpolateUnorm8(4, max_color, min_color, 2, other_colors);
    }
    err1 = CompressDXT1Pixmap(palette, src, &pixmap1);

    if (err0 < err1) {
        dest->color0 = imin_color;
        dest->color1 = imax_color;
        dest->pixmap = pixmap0;
    } else {
        dest->color0 = imax_color;
        dest->color1 = imin_color;
        dest->pixmap = pixmap1;
    }
}

void k3imageObj::CompressDXT3Block(const uint8_t* src, k3DXT3Block* dest)
{
    uint32_t i;
    const uint8_t* cur_alpha = src + 3;
    CompressDXT1Block(src, &(dest->dxt1), true);

    dest->alphas = 0;
    for (i = 0; i < 16; i++) {
        dest->alphas |= static_cast<uint64_t>(*cur_alpha >> 4) << (4 * i);
        cur_alpha += 4;
    }
}

void k3imageObj::CompressDXT5Block(const uint8_t* src, k3DXT3Block* dest)
{
    const uint8_t* cur_alpha = src + 3;
    CompressDXT1Block(src, &(dest->dxt1), true);
    CompressATI1NBlock(cur_alpha, &(dest->alphas));
}

void k3imageObj::CompressATI1NBlock(const uint8_t* src, uint64_t* dest)
{
    uint32_t i;
    const uint8_t* cur_alpha = src;
    const uint8_t* min_alpha = cur_alpha;
    const uint8_t* max_alpha = cur_alpha;
    uint8_t imin_alpha, imax_alpha;
    uint8_t other_alphas[6];
    uint32_t err0, err1;
    uint64_t alphamap0, alphamap1;
    const uint8_t* palette[8];

    for (i = 0; i < 16; i++) {
        if (*cur_alpha < *min_alpha) min_alpha = cur_alpha;
        if (*cur_alpha > *max_alpha) max_alpha = cur_alpha;
        cur_alpha += 4;
    }

    imin_alpha = *min_alpha;
    imax_alpha = *max_alpha;

    palette[2] = &(other_alphas[0]);
    palette[3] = &(other_alphas[1]);
    palette[4] = &(other_alphas[2]);
    palette[5] = &(other_alphas[3]);
    palette[6] = &(other_alphas[4]);
    palette[7] = &(other_alphas[5]);

    palette[0] = min_alpha;
    palette[1] = max_alpha;
    InterpolateUnorm8(1, min_alpha, max_alpha, 4, other_alphas);
    other_alphas[4] = 0;
    other_alphas[5] = 1;
    err0 = CompressATI1NPixmap(palette, src, &alphamap0);

    palette[0] = max_alpha;
    palette[1] = min_alpha;
    InterpolateUnorm8(1, max_alpha, min_alpha, 6, other_alphas);
    err1 = CompressATI1NPixmap(palette, src, &alphamap1);

    if (err0 <= err1 || imin_alpha == imax_alpha) {
        *dest = (alphamap0 << 16) | (static_cast<uint64_t>(imax_alpha) << 8) | imin_alpha;
    } else {
        *dest = (alphamap1 << 16) | (static_cast<uint64_t>(imin_alpha) << 8) | imax_alpha;
    }
}

void k3imageObj::CompressATI2NBlock(const uint8_t* src, k3ATI2NBlock* dest)
{
    const uint8_t* reds = src;
    const uint8_t* greens = src + 1;
    CompressATI1NBlock(reds, &(dest->reds));
    CompressATI1NBlock(greens, &(dest->greens));
}

float k3imageObj::ConvertFloat16ToFloat32(uint16_t f16)
{
    uint32_t u32out;
    float f32out;
    uint32_t sign = (f16 >> 15) & 0x1;
    uint32_t exp = (f16 >> 10) & 0x1f;
    uint32_t man = f16 & 0x3ff;

    if (exp == 0) {
        // Denorms
        if (man != 0) {
            // For non-zero denorm, normalize
            while (!(man & 0x400)) {
                man = man << 1;
                exp--;
            }
            man = man & 0x3ff;  // remove bit 0x400
        }
    } else if (exp == 31) {
        exp = 0xff;
    } else {
        exp = exp + 127 - 15;
    }
    u32out = ((sign << 31) | (exp << 23) | (man << 13));
    f32out = *reinterpret_cast<float*>(&u32out);
    return f32out;
}

uint16_t k3imageObj::ConvertFloat32ToFloat16(float f32)
{
    uint16_t u16;
    uint32_t u32 = *reinterpret_cast<uint32_t*>(&f32);
    uint32_t sign = (u32 >> 31) & 0x1;
    int32_t exp = ((u32 >> 23) & 0xff) - 127;
    uint32_t man = u32 & 0x7fffff;

    if (exp > 15) {
        exp = 0x1f;
        man = 0x0;
    } else if (exp < -14) {
        exp = 0x0;
        man = 0x0;
    } else {
        exp = exp + 15;
    }

    u16 = static_cast<uint16_t>((sign << 15) | (exp << 10) | (man >> 13));
    return u16;
}

void k3imageObj::ConvertToFloat4(k3fmt format, const void* src, float* dest)
{
    const uint8_t* u8src = static_cast<const uint8_t*>(src);
    const uint16_t* u16src = static_cast<const uint16_t*>(src);
    const uint32_t* u32src = static_cast<const uint32_t*>(src);
    const float* f32src = static_cast<const float*>(src);
    const k3DXT1Block* dxt1src = static_cast<const k3DXT1Block*>(src);
    const k3DXT3Block* dxt3src = static_cast<const k3DXT3Block*>(src);
    const uint64_t* u64src = static_cast<const uint64_t*>(src);
    const k3ATI2NBlock* ati2nsrc = static_cast<const k3ATI2NBlock*>(src);
    dest[0] = 0.0; dest[1] = 0.0; dest[2] = 0.0; dest[3] = 1.0;

    switch (format) {
    case k3fmt::RGBA8_UNORM:
        dest[0] = u8src[0] / static_cast<float>(0xff);
        dest[1] = u8src[1] / static_cast<float>(0xff);
        dest[2] = u8src[2] / static_cast<float>(0xff);
        dest[3] = u8src[3] / static_cast<float>(0xff);
        break;
    case k3fmt::BGRA8_UNORM:
        dest[0] = u8src[2] / static_cast<float>(0xff);
        dest[1] = u8src[1] / static_cast<float>(0xff);
        dest[2] = u8src[0] / static_cast<float>(0xff);
        dest[3] = u8src[3] / static_cast<float>(0xff);
        break;
    case k3fmt::RGBX8_UNORM:
        dest[0] = u8src[0] / static_cast<float>(0xff);
        dest[1] = u8src[1] / static_cast<float>(0xff);
        dest[2] = u8src[2] / static_cast<float>(0xff);
        break;
    case k3fmt::BGRX8_UNORM:
        dest[0] = u8src[2] / static_cast<float>(0xff);
        dest[1] = u8src[1] / static_cast<float>(0xff);
        dest[2] = u8src[0] / static_cast<float>(0xff);
        break;
    case k3fmt::RGBA16_UNORM:
        dest[0] = u16src[0] / static_cast<float>(0xffff);
        dest[1] = u16src[1] / static_cast<float>(0xffff);
        dest[2] = u16src[2] / static_cast<float>(0xffff);
        dest[3] = u16src[3] / static_cast<float>(0xffff);
        break;
    case k3fmt::RGBA16_UINT:
        dest[0] = u16src[0];
        dest[1] = u16src[1];
        dest[2] = u16src[2];
        dest[3] = u16src[3];
        break;
    case k3fmt::RGBA16_FLOAT:
        dest[0] = ConvertFloat16ToFloat32(*(u16src + 0));
        dest[1] = ConvertFloat16ToFloat32(*(u16src + 1));
        dest[2] = ConvertFloat16ToFloat32(*(u16src + 2));
        dest[3] = ConvertFloat16ToFloat32(*(u16src + 3));
        break;
    case k3fmt::RGBA32_UNORM:
        dest[0] = u32src[0] / static_cast<float>(0xffffffff);
        dest[1] = u32src[1] / static_cast<float>(0xffffffff);
        dest[2] = u32src[2] / static_cast<float>(0xffffffff);
        dest[3] = u32src[3] / static_cast<float>(0xffffffff);
        break;
    case k3fmt::RGBA32_UINT:
        dest[0] = static_cast<float>(u32src[0]);
        dest[1] = static_cast<float>(u32src[1]);
        dest[2] = static_cast<float>(u32src[2]);
        dest[3] = static_cast<float>(u32src[3]);
        break;
    case k3fmt::RGBA32_FLOAT:
        dest[0] = f32src[0];
        dest[1] = f32src[1];
        dest[2] = f32src[2];
        dest[3] = f32src[3];
        break;
    case k3fmt::RGB10A2_UNORM:
        dest[0] = ((u32src[0] >> 22) & 0x3ff) / static_cast<float>(0x3ff);
        dest[1] = ((u32src[0] >> 12) & 0x3ff) / static_cast<float>(0x3ff);
        dest[2] = ((u32src[0] >> 2) & 0x3ff) / static_cast<float>(0x3ff);
        dest[3] = ((u32src[0] >> 0) & 0x3) / static_cast<float>(0x3);
        break;
    case k3fmt::BGR5A1_UNORM:
        dest[0] = ((u16src[0] >> 1) & 0x1f) / static_cast<float>(0x1f);
        dest[1] = ((u16src[0] >> 6) & 0x1f) / static_cast<float>(0x1f);
        dest[2] = ((u16src[0] >> 11) & 0x1f) / static_cast<float>(0x1f);
        dest[3] = ((u16src[0] >> 0) & 0x1) / static_cast<float>(0x1);
        break;
        // 3 component
    case k3fmt::RGB8_UNORM:
        dest[0] = u8src[0] / static_cast<float>(0xff);
        dest[1] = u8src[1] / static_cast<float>(0xff);
        dest[2] = u8src[2] / static_cast<float>(0xff);
        break;
    case k3fmt::BGR8_UNORM:
        dest[0] = u8src[2] / static_cast<float>(0xff);
        dest[1] = u8src[1] / static_cast<float>(0xff);
        dest[2] = u8src[0] / static_cast<float>(0xff);
        break;
    case k3fmt::RGB32_UINT:
        dest[0] = static_cast<float>(u32src[0]);
        dest[1] = static_cast<float>(u32src[1]);
        dest[2] = static_cast<float>(u32src[2]);
        break;
    case k3fmt::RGB32_FLOAT:
        dest[0] = f32src[0];
        dest[1] = f32src[1];
        dest[2] = f32src[2];
        break;
    case k3fmt::B5G6R5_UNORM:
        dest[0] = ((u16src[0] >> 11) & 0x1f) / static_cast<float>(0x1f);
        dest[1] = ((u16src[0] >> 5) & 0x3f) / static_cast<float>(0x3f);
        dest[2] = ((u16src[0] >> 0) & 0x1f) / static_cast<float>(0x1f);
        break;
        // 2 component
    case k3fmt::RG8_UNORM:
        dest[0] = u8src[0] / static_cast<float>(0xff);
        dest[1] = u8src[1] / static_cast<float>(0xff);
        break;
    case k3fmt::RG16_UNORM:
        dest[0] = u16src[0] / static_cast<float>(0xffff);
        dest[1] = u16src[1] / static_cast<float>(0xffff);
        break;
    case k3fmt::RG16_UINT:
        dest[0] = u16src[0];
        dest[1] = u16src[1];
        break;
    case k3fmt::RG16_FLOAT:
        dest[0] = ConvertFloat16ToFloat32(*(u16src + 0));
        dest[1] = ConvertFloat16ToFloat32(*(u16src + 1));
        break;
    case k3fmt::RG32_UNORM:
        dest[0] = u32src[0] / static_cast<float>(0xffffffff);
        dest[1] = u32src[1] / static_cast<float>(0xffffffff);
        break;
    case k3fmt::RG32_UINT:
        dest[0] = static_cast<float>(u32src[0]);
        dest[1] = static_cast<float>(u32src[1]);
        break;
    case k3fmt::RG32_FLOAT:
        dest[0] = f32src[0];
        dest[1] = f32src[1];
        break;
        // 1 component
    case k3fmt::R8_UNORM:
        dest[0] = u8src[0] / static_cast<float>(0xff);
        break;
    case k3fmt::A8_UNORM:
        dest[3] = u8src[0] / static_cast<float>(0xff);
        break;
    case k3fmt::R16_UNORM:
        dest[0] = u16src[0] / static_cast<float>(0xffff);
        break;
    case k3fmt::R16_UINT:
        dest[0] = u16src[0];
        break;
    case k3fmt::R16_FLOAT:
        dest[0] = ConvertFloat16ToFloat32(*(u16src + 0));
        break;
    case k3fmt::R32_UNORM:
        dest[0] = u32src[0] / static_cast<float>(0xffffffff);
        break;
    case k3fmt::R32_UINT:
        dest[0] = static_cast<float>(u32src[0]);
        break;
    case k3fmt::R32_FLOAT:
        dest[0] = f32src[0];
        break;
        // Compressed formats
    case k3fmt::BC1_UNORM:
        DecompressDXT1Block(dxt1src, dest);
        break;
    case k3fmt::BC2_UNORM:
        DecompressDXT3Block(dxt3src, dest);
        break;
    case k3fmt::BC3_UNORM:
        DecompressDXT5Block(dxt3src, dest);
        break;
    case k3fmt::BC4_UNORM:
        DecompressATI1NBlock(u64src, dest);
        break;
    case k3fmt::BC5_UNORM:
        DecompressATI2NBlock(ati2nsrc, dest);
        break;
    case k3fmt::BC6_UNORM:
    case k3fmt::BC7_UNORM:
        // TODO: implement these
        break;
        // Depth/Stencil formats
    case k3fmt::D16_UNORM:
        dest[0] = u16src[0] / static_cast<float>(0xffff);
        break;
    case k3fmt::D24X8_UNORM:
        dest[0] = (u32src[0] & 0xffffff) / static_cast<float>(0xffffff);
        break;
    case k3fmt::D24_UNORM_S8_UINT:
        dest[0] = (u32src[0] & 0xffffff) / static_cast<float>(0xffffff);
        dest[1] = static_cast<float>((u32src[0] >> 24) & 0xff);
        break;
    case k3fmt::D32_FLOAT:
        dest[0] = f32src[0];
        break;
    case k3fmt::D32_FLOAT_S8X24_UINT:
        dest[0] = f32src[0];
        dest[1] = static_cast<float>((u32src[1] >> 8) & 0xff);
        break;
    default:
        break;
    }
}

void k3imageObj::ConvertFromFloat4(k3fmt format, const float* src, void* dest)
{
    uint8_t* u8dest = static_cast<uint8_t*>(dest);
    uint16_t* u16dest = static_cast<uint16_t*>(dest);
    uint32_t* u32dest = static_cast<uint32_t*>(dest);
    float* f32dest = static_cast<float*>(dest);

    switch (format) {
    case k3fmt::RGBA8_UNORM:
        u8dest[0] = static_cast<uint8_t>(src[0] * 0xff + 0.5);
        u8dest[1] = static_cast<uint8_t>(src[1] * 0xff + 0.5);
        u8dest[2] = static_cast<uint8_t>(src[2] * 0xff + 0.5);
        u8dest[3] = static_cast<uint8_t>(src[3] * 0xff + 0.5);
        break;
    case k3fmt::BGRA8_UNORM:
        u8dest[0] = static_cast<uint8_t>(src[2] * 0xff + 0.5);
        u8dest[1] = static_cast<uint8_t>(src[1] * 0xff + 0.5);
        u8dest[2] = static_cast<uint8_t>(src[0] * 0xff + 0.5);
        u8dest[3] = static_cast<uint8_t>(src[3] * 0xff + 0.5);
        break;
    case k3fmt::RGBX8_UNORM:
        u8dest[0] = static_cast<uint8_t>(src[0] * 0xff + 0.5);
        u8dest[1] = static_cast<uint8_t>(src[1] * 0xff + 0.5);
        u8dest[2] = static_cast<uint8_t>(src[2] * 0xff + 0.5);
        break;
    case k3fmt::BGRX8_UNORM:
        u8dest[0] = static_cast<uint8_t>(src[2] * 0xff + 0.5);
        u8dest[1] = static_cast<uint8_t>(src[1] * 0xff + 0.5);
        u8dest[2] = static_cast<uint8_t>(src[0] * 0xff + 0.5);
        break;
    case k3fmt::RGBA16_UNORM:
        u16dest[0] = static_cast<uint16_t>(src[0] * 0xffff + 0.5);
        u16dest[1] = static_cast<uint16_t>(src[1] * 0xffff + 0.5);
        u16dest[2] = static_cast<uint16_t>(src[2] * 0xffff + 0.5);
        u16dest[3] = static_cast<uint16_t>(src[3] * 0xffff + 0.5);
        break;
    case k3fmt::RGBA16_UINT:
        u16dest[0] = static_cast<uint16_t>(src[0]);
        u16dest[1] = static_cast<uint16_t>(src[1]);
        u16dest[2] = static_cast<uint16_t>(src[2]);
        u16dest[3] = static_cast<uint16_t>(src[3]);
        break;
    case k3fmt::RGBA16_FLOAT:
        u16dest[0] = ConvertFloat32ToFloat16(src[0]);
        u16dest[1] = ConvertFloat32ToFloat16(src[1]);
        u16dest[2] = ConvertFloat32ToFloat16(src[2]);
        u16dest[3] = ConvertFloat32ToFloat16(src[3]);
        break;
    case k3fmt::RGBA32_UNORM:
        u32dest[0] = static_cast<uint32_t>(src[0] * 0xffffffff + 0.5);
        u32dest[1] = static_cast<uint32_t>(src[1] * 0xffffffff + 0.5);
        u32dest[2] = static_cast<uint32_t>(src[2] * 0xffffffff + 0.5);
        u32dest[3] = static_cast<uint32_t>(src[3] * 0xffffffff + 0.5);
        break;
    case k3fmt::RGBA32_UINT:
        u32dest[0] = static_cast<uint32_t>(src[0]);
        u32dest[1] = static_cast<uint32_t>(src[1]);
        u32dest[2] = static_cast<uint32_t>(src[2]);
        u32dest[3] = static_cast<uint32_t>(src[3]);
        break;
    case k3fmt::RGBA32_FLOAT:
        f32dest[0] = src[0];
        f32dest[1] = src[1];
        f32dest[2] = src[2];
        f32dest[3] = src[3];
        break;
    case k3fmt::RGB10A2_UNORM:
        u32dest[0] = ((static_cast<uint32_t>(src[0] * 0x3ff + 0.5) << 22) |
            (static_cast<uint32_t>(src[1] * 0x3ff + 0.5) << 12) |
            (static_cast<uint32_t>(src[2] * 0x3ff + 0.5) << 2) |
            (static_cast<uint32_t>(src[3] * 0x3 + 0.5) << 0));
        break;
    case k3fmt::BGR5A1_UNORM:
        u16dest[0] = ((static_cast<uint16_t>(src[0] * 0x1f + 0.5) << 1) |
            (static_cast<uint16_t>(src[1] * 0x1f + 0.5) << 6) |
            (static_cast<uint16_t>(src[2] * 0x1f + 0.5) << 11) |
            (static_cast<uint16_t>(src[3] * 0x1 + 0.5) << 0));
        break;
        // 3 component
    case k3fmt::RGB8_UNORM:
        u8dest[0] = static_cast<uint8_t>(src[0] * 0xff + 0.5);
        u8dest[1] = static_cast<uint8_t>(src[1] * 0xff + 0.5);
        u8dest[2] = static_cast<uint8_t>(src[2] * 0xff + 0.5);
        break;
    case k3fmt::BGR8_UNORM:
        u8dest[0] = static_cast<uint8_t>(src[2] * 0xff + 0.5);
        u8dest[1] = static_cast<uint8_t>(src[1] * 0xff + 0.5);
        u8dest[2] = static_cast<uint8_t>(src[0] * 0xff + 0.5);
        break;
    case k3fmt::RGB32_UINT:
        u32dest[0] = static_cast<uint32_t>(src[0]);
        u32dest[1] = static_cast<uint32_t>(src[1]);
        u32dest[2] = static_cast<uint32_t>(src[2]);
        break;
    case k3fmt::RGB32_FLOAT:
        f32dest[0] = src[0];
        f32dest[1] = src[1];
        f32dest[2] = src[2];
        break;
    case k3fmt::B5G6R5_UNORM:
        u16dest[0] = ((static_cast<uint16_t>(src[0] * 0x1f + 0.5) << 11) |
            (static_cast<uint16_t>(src[1] * 0x3f + 0.5) << 5) |
            (static_cast<uint16_t>(src[2] * 0x1f + 0.5) << 0));
        break;
        // 2 component
    case k3fmt::RG8_UNORM:
        u8dest[0] = static_cast<uint8_t>(src[0] * 0xff + 0.5);
        u8dest[1] = static_cast<uint8_t>(src[1] * 0xff + 0.5);
        break;
    case k3fmt::RG16_UNORM:
        u16dest[0] = static_cast<uint16_t>(src[0] * 0xffff + 0.5);
        u16dest[1] = static_cast<uint16_t>(src[1] * 0xffff + 0.5);
        break;
    case k3fmt::RG16_UINT:
        u16dest[0] = static_cast<uint16_t>(src[0]);
        u16dest[1] = static_cast<uint16_t>(src[1]);
        break;
    case k3fmt::RG16_FLOAT:
        u16dest[0] = ConvertFloat32ToFloat16(src[0]);
        u16dest[1] = ConvertFloat32ToFloat16(src[1]);
        break;
    case k3fmt::RG32_UNORM:
        u32dest[0] = static_cast<uint32_t>(src[0] * 0xffffffff + 0.5);
        u32dest[1] = static_cast<uint32_t>(src[1] * 0xffffffff + 0.5);
        break;
    case k3fmt::RG32_UINT:
        u32dest[0] = static_cast<uint32_t>(src[0]);
        u32dest[1] = static_cast<uint32_t>(src[1]);
        break;
    case k3fmt::RG32_FLOAT:
        f32dest[0] = src[0];
        f32dest[1] = src[1];
        break;
        // 1 component
    case k3fmt::R8_UNORM:
        u8dest[0] = static_cast<uint8_t>(src[0] * 0xff + 0.5);
        break;
    case k3fmt::A8_UNORM:
        u8dest[0] = static_cast<uint8_t>(src[3] * 0xff + 0.5);
        break;
    case k3fmt::R16_UNORM:
        u16dest[0] = static_cast<uint16_t>(src[0] * 0xffff + 0.5);
        break;
    case k3fmt::R16_UINT:
        u16dest[0] = static_cast<uint16_t>(src[0]);
        break;
    case k3fmt::R16_FLOAT:
        u16dest[0] = ConvertFloat32ToFloat16(src[0]);
        break;
    case k3fmt::R32_UNORM:
        u32dest[0] = static_cast<uint32_t>(src[0] * 0xffffffff + 0.5);
        break;
    case k3fmt::R32_UINT:
        u32dest[0] = static_cast<uint32_t>(src[0]);
        break;
    case k3fmt::R32_FLOAT:
        f32dest[0] = src[0];
        break;
        // Compressed formats
    case k3fmt::BC1_UNORM:
        CompressDXT1Block(src, static_cast<k3DXT1Block*>(dest));
        break;
    case k3fmt::BC2_UNORM:
        CompressDXT3Block(src, static_cast<k3DXT3Block*>(dest));
        break;
    case k3fmt::BC3_UNORM:
        CompressDXT5Block(src, static_cast<k3DXT3Block*>(dest));
        break;
    case k3fmt::BC4_UNORM:
        CompressATI1NBlock(src, static_cast<uint64_t*>(dest));
        break;
    case k3fmt::BC5_UNORM:
        CompressATI2NBlock(src, static_cast<k3ATI2NBlock*>(dest));
        break;
    case k3fmt::BC6_UNORM:
    case k3fmt::BC7_UNORM:
        // TODO: implement these
        break;
        // Depth/Stencil formats
    case k3fmt::D16_UNORM:
        u16dest[0] = static_cast<uint16_t>(src[0] * 0xffff + 0.5);
        break;
    case k3fmt::D24X8_UNORM:
        u32dest[0] = static_cast<uint32_t>(src[0] * 0xffffff + 0.5);
        break;
    case k3fmt::D24_UNORM_S8_UINT:
        u32dest[0] = static_cast<uint32_t>(src[0] * 0xffffff + 0.5);
        u32dest[0] |= (static_cast<uint32_t>(src[1]) & 0xff) << 24;
        break;
    case k3fmt::D32_FLOAT:
        f32dest[0] = src[0];
        break;
    case k3fmt::D32_FLOAT_S8X24_UINT:
        f32dest[0] = src[0];
        u32dest[1] = (static_cast<uint32_t>(src[1]) & 0xff);
        break;
    default:
        break;
    }
}

void k3imageObj::ConvertToUnorm8(k3fmt format, const void* src, uint8_t* dest)
{
    const uint8_t* u8src = static_cast<const uint8_t*>(src);
    const uint16_t* u16src = static_cast<const uint16_t*>(src);
    const uint32_t* u32src = static_cast<const uint32_t*>(src);
    const float* f32src = static_cast<const float*>(src);
    const k3DXT1Block* dxt1src = static_cast<const k3DXT1Block*>(src);
    const k3DXT3Block* dxt3src = static_cast<const k3DXT3Block*>(src);
    const uint64_t* u64src = static_cast<const uint64_t*>(src);
    const k3ATI2NBlock* ati2nsrc = static_cast<const k3ATI2NBlock*>(src);
    dest[0] = 0; dest[1] = 0; dest[2] = 0; dest[3] = 0xff;

    uint8_t alpha_val;

    switch (format) {
    case k3fmt::RGBA8_UNORM:
        dest[0] = u8src[0];
        dest[1] = u8src[1];
        dest[2] = u8src[2];
        dest[3] = u8src[3];
        break;
    case k3fmt::BGRA8_UNORM:
        dest[0] = u8src[2];
        dest[1] = u8src[1];
        dest[2] = u8src[0];
        dest[3] = u8src[3];
        break;
    case k3fmt::RGBX8_UNORM:
        dest[0] = u8src[0];
        dest[1] = u8src[1];
        dest[2] = u8src[2];
        break;
    case k3fmt::BGRX8_UNORM:
        dest[0] = u8src[2];
        dest[1] = u8src[1];
        dest[2] = u8src[0];
        break;
    case k3fmt::RGBA16_UNORM:
        dest[0] = static_cast<uint8_t>(u16src[0] >> 8);
        dest[1] = static_cast<uint8_t>(u16src[1] >> 8);
        dest[2] = static_cast<uint8_t>(u16src[2] >> 8);
        dest[3] = static_cast<uint8_t>(u16src[3] >> 8);
        break;
    case k3fmt::RGBA16_UINT:
        dest[0] = (u16src[0] != 0) ? 1 : 0;
        dest[1] = (u16src[1] != 0) ? 1 : 0;
        dest[2] = (u16src[2] != 0) ? 1 : 0;
        dest[3] = (u16src[3] != 0) ? 1 : 0;
        break;
    case k3fmt::RGBA16_FLOAT:
        dest[0] = static_cast<uint8_t>(ConvertFloat16ToFloat32(*(u16src + 0)) * 0xff);
        dest[1] = static_cast<uint8_t>(ConvertFloat16ToFloat32(*(u16src + 1)) * 0xff);
        dest[2] = static_cast<uint8_t>(ConvertFloat16ToFloat32(*(u16src + 2)) * 0xff);
        dest[3] = static_cast<uint8_t>(ConvertFloat16ToFloat32(*(u16src + 3)) * 0xff);
        break;
    case k3fmt::RGBA32_UNORM:
        dest[0] = static_cast<uint8_t>(u32src[0] >> 24);
        dest[1] = static_cast<uint8_t>(u32src[1] >> 24);
        dest[2] = static_cast<uint8_t>(u32src[2] >> 24);
        dest[3] = static_cast<uint8_t>(u32src[3] >> 24);
        break;
    case k3fmt::RGBA32_UINT:
        dest[0] = (u32src[0] != 0) ? 1 : 0;
        dest[1] = (u32src[1] != 0) ? 1 : 0;
        dest[2] = (u32src[2] != 0) ? 1 : 0;
        dest[3] = (u32src[3] != 0) ? 1 : 0;
        break;
    case k3fmt::RGBA32_FLOAT:
        dest[0] = static_cast<uint8_t>(f32src[0] * 0xff);
        dest[1] = static_cast<uint8_t>(f32src[1] * 0xff);
        dest[2] = static_cast<uint8_t>(f32src[2] * 0xff);
        dest[3] = static_cast<uint8_t>(f32src[3] * 0xff);
        break;
    case k3fmt::RGB10A2_UNORM:
        dest[0] = static_cast<uint8_t>((u32src[0] >> 24) & 0xff);
        dest[1] = static_cast<uint8_t>((u32src[0] >> 14) & 0xff);
        dest[2] = static_cast<uint8_t>((u32src[0] >> 4) & 0xff);
        alpha_val = static_cast<uint8_t>((u32src[0] >> 0) & 0x3);
        dest[3] = (alpha_val << 6) | (alpha_val << 4) | (alpha_val << 2) | alpha_val;
        break;
    case k3fmt::BGR5A1_UNORM:
        dest[0] = ((u16src[0] << 2) & 0xf8) | ((u16src[0] >> 3) & 0x07);
        dest[1] = ((u16src[0] >> 3) & 0xf8) | ((u16src[0] >> 8) & 0x07);
        dest[2] = ((u16src[0] >> 8) & 0xf8) | ((u16src[0] >> 13) & 0x07);
        dest[3] = ((u16src[0] >> 0) & 0x1) ? 0xff : 0x00;
        break;
        // 3 component
    case k3fmt::RGB8_UNORM:
        dest[0] = u8src[0];
        dest[1] = u8src[1];
        dest[2] = u8src[2];
        break;
    case k3fmt::BGR8_UNORM:
        dest[0] = u8src[2];
        dest[1] = u8src[1];
        dest[2] = u8src[0];
        break;
    case k3fmt::RGB32_UINT:
        dest[0] = (u32src[0] != 0) ? 1 : 0;
        dest[1] = (u32src[1] != 0) ? 1 : 0;
        dest[2] = (u32src[2] != 0) ? 1 : 0;
        break;
    case k3fmt::RGB32_FLOAT:
        dest[0] = static_cast<uint8_t>(f32src[0] * 0xff);
        dest[1] = static_cast<uint8_t>(f32src[1] * 0xff);
        dest[2] = static_cast<uint8_t>(f32src[2] * 0xff);
        break;
    case k3fmt::B5G6R5_UNORM:
        dest[0] = ((u16src[0] >> 8) & 0xf8) | ((u16src[0] >> 13) & 0x07);
        dest[1] = ((u16src[0] >> 3) & 0xfc) | ((u16src[0] >> 9) & 0x03);
        dest[2] = ((u16src[0] << 3) & 0xf8) | ((u16src[0] >> 2) & 0x07);
        break;
        // 2 component
    case k3fmt::RG8_UNORM:
        dest[0] = u8src[0];
        dest[1] = u8src[1];
        break;
    case k3fmt::RG16_UNORM:
        dest[0] = static_cast<uint8_t>(u16src[0] >> 8);
        dest[1] = static_cast<uint8_t>(u16src[1] >> 8);
        break;
    case k3fmt::RG16_UINT:
        dest[0] = (u16src[0] != 0) ? 1 : 0;
        dest[1] = (u16src[1] != 0) ? 1 : 0;
        break;
    case k3fmt::RG16_FLOAT:
        dest[0] = static_cast<uint8_t>(ConvertFloat16ToFloat32(*(u16src + 0)) * 0xff);
        dest[1] = static_cast<uint8_t>(ConvertFloat16ToFloat32(*(u16src + 1)) * 0xff);
        break;
    case k3fmt::RG32_UNORM:
        dest[0] = static_cast<uint8_t>(u32src[0] >> 24);
        dest[1] = static_cast<uint8_t>(u32src[1] >> 24);
        break;
    case k3fmt::RG32_UINT:
        dest[0] = (u32src[0] != 0) ? 1 : 0;
        dest[1] = (u32src[1] != 0) ? 1 : 0;
        break;
    case k3fmt::RG32_FLOAT:
        dest[0] = static_cast<uint8_t>(f32src[0] * 0xff);
        dest[1] = static_cast<uint8_t>(f32src[1] * 0xff);
        break;
        // 1 component
    case k3fmt::R8_UNORM:
        dest[0] = u8src[0];
        break;
    case k3fmt::A8_UNORM:
        dest[3] = u8src[0];
        break;
    case k3fmt::R16_UNORM:
        dest[0] = static_cast<uint8_t>(u16src[0] >> 8);
        break;
    case k3fmt::R16_UINT:
        dest[0] = (u16src[0] != 0) ? 1 : 0;
        break;
    case k3fmt::R16_FLOAT:
        dest[0] = static_cast<uint8_t>(ConvertFloat16ToFloat32(*(u16src + 0)) * 0xff);
        break;
    case k3fmt::R32_UNORM:
        dest[0] = static_cast<uint8_t>(u32src[0] >> 24);
        break;
    case k3fmt::R32_UINT:
        dest[0] = (u32src[0] != 0) ? 1 : 0;
        break;
    case k3fmt::R32_FLOAT:
        dest[0] = static_cast<uint8_t>(f32src[0] * 0xff);
        break;
        // Compressed formats
    case k3fmt::BC1_UNORM:
        DecompressDXT1Block(dxt1src, dest);
        break;
    case k3fmt::BC2_UNORM:
        DecompressDXT3Block(dxt3src, dest);
        break;
    case k3fmt::BC3_UNORM:
        DecompressDXT5Block(dxt3src, dest);
        break;
    case k3fmt::BC4_UNORM:
        DecompressATI1NBlock(u64src, dest);
        break;
    case k3fmt::BC5_UNORM:
        DecompressATI2NBlock(ati2nsrc, dest);
        break;
    case k3fmt::BC6_UNORM:
    case k3fmt::BC7_UNORM:
        // TODO: implement these
        break;
        // Depth/Stencil formats
    case k3fmt::D16_UNORM:
        dest[0] = static_cast<uint8_t>(u16src[0] >> 8);
        break;
    case k3fmt::D24X8_UNORM:
        dest[0] = static_cast<uint8_t>((u32src[0] & 0xffffff) >> 16);
        break;
    case k3fmt::D24_UNORM_S8_UINT:
        dest[0] = static_cast<uint8_t>((u32src[0] & 0xffffff) >> 16);
        dest[1] = static_cast<uint8_t>((u32src[0] & 0xff) >> 24);
        break;
    case k3fmt::D32_FLOAT:
        dest[0] = static_cast<uint8_t>(f32src[0] * 0xff);
        break;
    case k3fmt::D32_FLOAT_S8X24_UINT:
        dest[0] = static_cast<uint8_t>(f32src[0] * 0xff);
        dest[1] = static_cast<uint8_t>((u32src[1]) & 0xff);
        break;
    default:
        break;
    }
}

void k3imageObj::ConvertFromUnorm8(k3fmt format, const uint8_t* src, void* dest)
{
    uint8_t* u8dest = static_cast<uint8_t*>(dest);
    uint16_t* u16dest = static_cast<uint16_t*>(dest);
    uint32_t* u32dest = static_cast<uint32_t*>(dest);
    float* f32dest = static_cast<float*>(dest);

    uint32_t color32r, color32g, color32b, color32a;

    switch (format) {
    case k3fmt::RGBA8_UNORM:
        u8dest[0] = src[0];
        u8dest[1] = src[1];
        u8dest[2] = src[2];
        u8dest[3] = src[3];
        break;
    case k3fmt::BGRA8_UNORM:
        u8dest[0] = src[2];
        u8dest[1] = src[1];
        u8dest[2] = src[0];
        u8dest[3] = src[3];
        break;
    case k3fmt::RGBX8_UNORM:
        u8dest[0] = src[0];
        u8dest[1] = src[1];
        u8dest[2] = src[2];
        break;
    case k3fmt::BGRX8_UNORM:
        u8dest[0] = src[2];
        u8dest[1] = src[1];
        u8dest[2] = src[0];
        break;
    case k3fmt::RGBA16_UNORM:
        u16dest[0] = (static_cast<uint16_t>(src[0]) << 8) | src[0];
        u16dest[1] = (static_cast<uint16_t>(src[1]) << 8) | src[1];
        u16dest[2] = (static_cast<uint16_t>(src[2]) << 8) | src[2];
        u16dest[3] = (static_cast<uint16_t>(src[3]) << 8) | src[3];
        break;
    case k3fmt::RGBA16_UINT:
        u16dest[0] = static_cast<uint16_t>(src[0] >> 7);
        u16dest[1] = static_cast<uint16_t>(src[1] >> 7);
        u16dest[2] = static_cast<uint16_t>(src[2] >> 7);
        u16dest[3] = static_cast<uint16_t>(src[3] >> 7);
        break;
    case k3fmt::RGBA16_FLOAT:
        u16dest[0] = ConvertFloat32ToFloat16(src[0] / 255.0f);
        u16dest[1] = ConvertFloat32ToFloat16(src[1] / 255.0f);
        u16dest[2] = ConvertFloat32ToFloat16(src[2] / 255.0f);
        u16dest[3] = ConvertFloat32ToFloat16(src[3] / 255.0f);
        break;
    case k3fmt::RGBA32_UNORM:
        color32r = static_cast<uint32_t>(src[0]);
        color32g = static_cast<uint32_t>(src[1]);
        color32b = static_cast<uint32_t>(src[2]);
        color32a = static_cast<uint32_t>(src[3]);
        u32dest[0] = (color32r << 24) | (color32r << 16) | (color32r << 8) | color32r;
        u32dest[1] = (color32g << 24) | (color32g << 16) | (color32g << 8) | color32g;
        u32dest[2] = (color32b << 24) | (color32b << 16) | (color32b << 8) | color32b;
        u32dest[3] = (color32a << 24) | (color32a << 16) | (color32a << 8) | color32a;
        break;
    case k3fmt::RGBA32_UINT:
        u32dest[0] = static_cast<uint32_t>(src[0] >> 7);
        u32dest[1] = static_cast<uint32_t>(src[1] >> 7);
        u32dest[2] = static_cast<uint32_t>(src[2] >> 7);
        u32dest[3] = static_cast<uint32_t>(src[3] >> 7);
        break;
    case k3fmt::RGBA32_FLOAT:
        f32dest[0] = src[0] / 255.0f;
        f32dest[1] = src[1] / 255.0f;
        f32dest[2] = src[2] / 255.0f;
        f32dest[3] = src[3] / 255.0f;
        break;
    case k3fmt::RGB10A2_UNORM:
        color32r = src[0]; color32r = (color32r << 2) | (color32r >> 6);
        color32g = src[1]; color32g = (color32g << 2) | (color32g >> 6);
        color32b = src[2]; color32b = (color32b << 2) | (color32b >> 6);
        color32a = src[3] >> 6;
        u32dest[0] = (color32r << 22) | (color32g << 12) | (color32b << 2) | color32a;
        break;
    case k3fmt::BGR5A1_UNORM:
        color32r = src[0] >> 3;
        color32g = src[1] >> 3;
        color32b = src[2] >> 3;
        color32a = src[3] >> 7;
        u16dest[0] = static_cast<uint16_t>((color32r << 1) | (color32g << 6) | (color32b << 11) | color32a);
        break;
        // 3 component
    case k3fmt::RGB8_UNORM:
        u8dest[0] = src[0];
        u8dest[1] = src[1];
        u8dest[2] = src[2];
        break;
    case k3fmt::BGR8_UNORM:
        u8dest[0] = src[2];
        u8dest[1] = src[1];
        u8dest[2] = src[0];
        break;
    case k3fmt::RGB32_UINT:
        u32dest[0] = static_cast<uint32_t>(src[0] >> 7);
        u32dest[1] = static_cast<uint32_t>(src[1] >> 7);
        u32dest[2] = static_cast<uint32_t>(src[2] >> 7);
        break;
    case k3fmt::RGB32_FLOAT:
        f32dest[0] = src[0] / 255.0f;
        f32dest[1] = src[1] / 255.0f;
        f32dest[2] = src[2] / 255.0f;
        break;
    case k3fmt::B5G6R5_UNORM:
        color32r = src[0] >> 3;
        color32g = src[1] >> 2;
        color32b = src[2] >> 3;
        u16dest[0] = static_cast<uint16_t>((color32r << 11) | (color32g << 5) | color32b);
        break;
        // 2 component
    case k3fmt::RG8_UNORM:
        u8dest[0] = src[0];
        u8dest[1] = src[1];
        break;
    case k3fmt::RG16_UNORM:
        u16dest[0] = (static_cast<uint16_t>(src[0]) << 8) | src[0];
        u16dest[1] = (static_cast<uint16_t>(src[1]) << 8) | src[1];
        break;
    case k3fmt::RG16_UINT:
        u16dest[0] = static_cast<uint16_t>(src[0] >> 7);
        u16dest[1] = static_cast<uint16_t>(src[1] >> 7);
        break;
    case k3fmt::RG16_FLOAT:
        u16dest[0] = ConvertFloat32ToFloat16(src[0] / 255.0f);
        u16dest[1] = ConvertFloat32ToFloat16(src[1] / 255.0f);
        break;
    case k3fmt::RG32_UNORM:
        color32r = static_cast<uint32_t>(src[0]);
        color32g = static_cast<uint32_t>(src[1]);
        u32dest[0] = (color32r << 24) | (color32r << 16) | (color32r << 8) | color32r;
        u32dest[1] = (color32g << 24) | (color32g << 16) | (color32g << 8) | color32g;
        break;
    case k3fmt::RG32_UINT:
        u32dest[0] = static_cast<uint32_t>(src[0] >> 7);
        u32dest[1] = static_cast<uint32_t>(src[1] >> 7);
        break;
    case k3fmt::RG32_FLOAT:
        f32dest[0] = src[0] / 255.0f;
        f32dest[1] = src[1] / 255.0f;
        break;
        // 1 component
    case k3fmt::R8_UNORM:
        u8dest[0] = src[0];
        break;
    case k3fmt::A8_UNORM:
        u8dest[0] = src[3];
        break;
    case k3fmt::R16_UNORM:
        u16dest[0] = (static_cast<uint16_t>(src[0]) << 8) | src[0];
        break;
    case k3fmt::R16_UINT:
        u16dest[0] = static_cast<uint16_t>(src[0] >> 7);
        break;
    case k3fmt::R16_FLOAT:
        u16dest[0] = ConvertFloat32ToFloat16(src[0] / 255.0f);
        break;
    case k3fmt::R32_UNORM:
        color32r = static_cast<uint32_t>(src[0]);
        u32dest[0] = (color32r << 24) | (color32r << 16) | (color32r << 8) | color32r;
        break;
    case k3fmt::R32_UINT:
        u32dest[0] = static_cast<uint32_t>(src[0] >> 7);
        break;
    case k3fmt::R32_FLOAT:
        f32dest[0] = src[0] / 255.0f;
        break;
        // Compressed formats
    case k3fmt::BC1_UNORM:
        CompressDXT1Block(src, static_cast<k3DXT1Block*>(dest));
        break;
    case k3fmt::BC2_UNORM:
        CompressDXT3Block(src, static_cast<k3DXT3Block*>(dest));
        break;
    case k3fmt::BC3_UNORM:
        CompressDXT5Block(src, static_cast<k3DXT3Block*>(dest));
        break;
    case k3fmt::BC4_UNORM:
        CompressATI1NBlock(src, static_cast<uint64_t*>(dest));
        break;
    case k3fmt::BC5_UNORM:
        CompressATI2NBlock(src, static_cast<k3ATI2NBlock*>(dest));
        break;
    case k3fmt::BC6_UNORM:
    case k3fmt::BC7_UNORM:
        // TODO: implement these
        break;
        // Depth/Stencil formats
    case k3fmt::D16_UNORM:
        u16dest[0] = (static_cast<uint16_t>(src[0]) << 8) | src[0];
        break;
    case k3fmt::D24X8_UNORM:
        color32r = static_cast<uint32_t>(src[0]);
        u32dest[0] = (color32r << 16) | (color32r << 8) | (color32r);
        break;
    case k3fmt::D24_UNORM_S8_UINT:
        color32r = static_cast<uint32_t>(src[0]);
        color32g = static_cast<uint32_t>(src[1]);
        u32dest[0] = (color32r << 16) | (color32r << 8) | (color32r) | (color32g << 24);
        break;
    case k3fmt::D32_FLOAT:
        f32dest[0] = src[0] / 255.0f;
        break;
    case k3fmt::D32_FLOAT_S8X24_UINT:
        f32dest[0] = src[0] / 255.0f;
        u32dest[1] = (static_cast<uint32_t>(src[1]) & 0xff);
        break;
    default:
        break;
    }
}

uint32_t k3imageObj::GetFormatSize(k3fmt format)
{
    switch (format) {
    case k3fmt::RGBA32_UNORM:
    case k3fmt::RGBA32_UINT:
    case k3fmt::RGBA32_FLOAT:
        return 16;

    case k3fmt::RGB32_UINT:
    case k3fmt::RGB32_FLOAT:
        return 12;

    case k3fmt::RGBA16_UNORM:
    case k3fmt::RGBA16_UINT:
    case k3fmt::RGBA16_FLOAT:
    case k3fmt::RG32_UNORM:
    case k3fmt::RG32_UINT:
    case k3fmt::RG32_FLOAT:
    case k3fmt::D32_FLOAT_S8X24_UINT:
        return 8;

    case k3fmt::RGBA8_UNORM:
    case k3fmt::BGRA8_UNORM:
    case k3fmt::RGBX8_UNORM:
    case k3fmt::BGRX8_UNORM:
    case k3fmt::RG16_UNORM:
    case k3fmt::RG16_UINT:
    case k3fmt::RG16_FLOAT:
    case k3fmt::R32_UNORM:
    case k3fmt::R32_UINT:
    case k3fmt::R32_FLOAT:
    case k3fmt::D24X8_UNORM:
    case k3fmt::D32_FLOAT:
    case k3fmt::D24_UNORM_S8_UINT:
        return 4;

    case k3fmt::RGB8_UNORM:
    case k3fmt::BGR8_UNORM:
        return 3;

    case k3fmt::RGB10A2_UNORM:
    case k3fmt::BGR5A1_UNORM:
    case k3fmt::B5G6R5_UNORM:
    case k3fmt::RG8_UNORM:
    case k3fmt::R16_UNORM:
    case k3fmt::R16_UINT:
    case k3fmt::R16_FLOAT:
    case k3fmt::D16_UNORM:
        return 2;

    case k3fmt::R8_UNORM:
    case k3fmt::A8_UNORM:
        return 1;

        // Compressed formats - size of a block
    case k3fmt::BC1_UNORM:
        return 8;
    case k3fmt::BC2_UNORM:
    case k3fmt::BC3_UNORM:
        return 16;
    case k3fmt::BC4_UNORM:
        return 8;
    case k3fmt::BC5_UNORM:
        return 16;
    case k3fmt::BC6_UNORM:
    case k3fmt::BC7_UNORM:
        // TODO: check on these
        return 0;
    default:
        return 0;
    }
}

uint32_t k3imageObj::GetFormatBlockSize(k3fmt format)
{
    switch (format) {
    case k3fmt::RGBA32_UNORM:
    case k3fmt::RGBA32_FLOAT:
    case k3fmt::RGB32_FLOAT:
    case k3fmt::RG32_UNORM:
    case k3fmt::RG32_FLOAT:
    case k3fmt::RGBA8_UNORM:
    case k3fmt::BGRA8_UNORM:
    case k3fmt::RGBX8_UNORM:
    case k3fmt::BGRX8_UNORM:
    case k3fmt::RG16_UNORM:
    case k3fmt::RG16_FLOAT:
    case k3fmt::R32_UNORM:
    case k3fmt::R32_UINT:
    case k3fmt::R32_FLOAT:
    case k3fmt::RGB8_UNORM:
    case k3fmt::BGR8_UNORM:
    case k3fmt::RGBA16_UNORM:
    case k3fmt::RGBA16_FLOAT:
    case k3fmt::RGB10A2_UNORM:
    case k3fmt::BGR5A1_UNORM:
    case k3fmt::B5G6R5_UNORM:
    case k3fmt::RG8_UNORM:
    case k3fmt::R16_UNORM:
    case k3fmt::R16_UINT:
    case k3fmt::R16_FLOAT:
    case k3fmt::R8_UNORM:
    case k3fmt::A8_UNORM:
    case k3fmt::D16_UNORM:
    case k3fmt::D24X8_UNORM:
    case k3fmt::D32_FLOAT:
    case k3fmt::D24_UNORM_S8_UINT:
    case k3fmt::D32_FLOAT_S8X24_UINT:
        return 1;

        // Compressed formats
    case k3fmt::BC1_UNORM:
    case k3fmt::BC2_UNORM:
    case k3fmt::BC3_UNORM:
    case k3fmt::BC4_UNORM:
    case k3fmt::BC5_UNORM:
    case k3fmt::BC6_UNORM:
    case k3fmt::BC7_UNORM:
        // TODO: check on these
        return 4;
    default:
        return 0;
    }
}

uint32_t k3imageObj::GetImageSize(uint32_t width, uint32_t height, uint32_t depth, k3fmt format)
{
    uint32_t format_size = GetFormatSize(format);
    uint32_t block_size = GetFormatBlockSize(format);

    uint32_t adj_width = ((width + block_size - 1) / block_size);
    uint32_t adj_height = ((height + block_size - 1) / block_size);

    return adj_width * adj_height * depth * format_size;
}

uint32_t k3imageObj::GetFormatNumComponents(k3fmt format)
{
    switch (format) {
    case k3fmt::RGBA32_UNORM:
    case k3fmt::RGBA32_FLOAT:
    case k3fmt::RGBA8_UNORM:
    case k3fmt::BGRA8_UNORM:
    case k3fmt::RGBA16_UNORM:
    case k3fmt::RGBA16_FLOAT:
    case k3fmt::RGB10A2_UNORM:
    case k3fmt::BGR5A1_UNORM:
        return 4;

    case k3fmt::RGB32_FLOAT:
    case k3fmt::RGBX8_UNORM:
    case k3fmt::BGRX8_UNORM:
    case k3fmt::RGB8_UNORM:
    case k3fmt::BGR8_UNORM:
    case k3fmt::B5G6R5_UNORM:
        return 3;

    case k3fmt::RG32_UNORM:
    case k3fmt::RG32_FLOAT:
    case k3fmt::RG16_UNORM:
    case k3fmt::RG16_FLOAT:
    case k3fmt::D24_UNORM_S8_UINT:
    case k3fmt::D32_FLOAT_S8X24_UINT:
        return 2;

    case k3fmt::R32_UNORM:
    case k3fmt::R32_UINT:
    case k3fmt::R32_FLOAT:
    case k3fmt::RG8_UNORM:
    case k3fmt::R16_UNORM:
    case k3fmt::R16_UINT:
    case k3fmt::R16_FLOAT:
    case k3fmt::R8_UNORM:
    case k3fmt::A8_UNORM:
    case k3fmt::D16_UNORM:
    case k3fmt::D24X8_UNORM:
    case k3fmt::D32_FLOAT:
        return 1;

        // Compressed formats
    case k3fmt::BC1_UNORM:
    case k3fmt::BC2_UNORM:
    case k3fmt::BC3_UNORM:
        return 4;

    case k3fmt::BC4_UNORM:
        return 1;

    case k3fmt::BC5_UNORM:
        return 2;

    case k3fmt::BC6_UNORM:
    case k3fmt::BC7_UNORM:
        // TODO: check on these
        return 4;
    default:
        return 0;
    }
}

uint32_t k3imageObj::GetComponentBits(k3component component, k3fmt format)
{
    if (component == k3component::RED || component == k3component::GREEN || component == k3component::BLUE || component == k3component::ALPHA) {
        switch (format) {
        case k3fmt::RGBA32_UNORM:
        case k3fmt::RGBA32_FLOAT:
            return 32;
        case k3fmt::RGBA8_UNORM:
        case k3fmt::BGRA8_UNORM:
            return 8;
        case k3fmt::RGBA16_UNORM:
        case k3fmt::RGBA16_FLOAT:
            return 16;
        case k3fmt::RGB10A2_UNORM:
            if (component == k3component::ALPHA) return 2;
            return 10;
        case k3fmt::BGR5A1_UNORM:
            if (component == k3component::ALPHA) return 1;
            return 5;

        case k3fmt::RGB32_FLOAT:
            if (component == k3component::ALPHA) return 0;
            return 32;
        case k3fmt::RGBX8_UNORM:
        case k3fmt::BGRX8_UNORM:
        case k3fmt::RGB8_UNORM:
        case k3fmt::BGR8_UNORM:
            if (component == k3component::ALPHA) return 0;
            return 8;
        case k3fmt::B5G6R5_UNORM:
            switch (component) {
            case k3component::RED:
            case k3component::BLUE:
                return 5;
            case k3component::GREEN:
                return 6;
            default:
                return 0;
            }
            break;
        case k3fmt::RG32_UNORM:
        case k3fmt::RG32_FLOAT:
            switch (component) {
            case k3component::RED:
            case k3component::BLUE:
                return 32;
            default:
                return 0;
            }
            break;
        case k3fmt::RG16_UNORM:
        case k3fmt::RG16_FLOAT:
            switch (component) {
            case k3component::RED:
            case k3component::BLUE:
                return 16;
            default:
                return 0;
            }
            break;

        case k3fmt::R32_UNORM:
        case k3fmt::R32_UINT:
        case k3fmt::R32_FLOAT:
            if (component == k3component::RED) return 32;
            return 0;
        case k3fmt::RG8_UNORM:
            switch (component) {
            case k3component::RED:
            case k3component::BLUE:
                return 8;
            default:
                return 0;
            }
            break;
        case k3fmt::R16_UNORM:
        case k3fmt::R16_UINT:
        case k3fmt::R16_FLOAT:
            if (component == k3component::RED) return 16;
            return 0;
        case k3fmt::R8_UNORM:
            if (component == k3component::RED) return 8;
            return 0;
        case k3fmt::A8_UNORM:
            if (component == k3component::ALPHA) return 8;
            return 0;

            // Compressed formats
        case k3fmt::BC1_UNORM:
            if (component == k3component::ALPHA) return 1;
            return 8;
        case k3fmt::BC2_UNORM:
        case k3fmt::BC3_UNORM:
            return 8;

        case k3fmt::BC4_UNORM:
            if (component == k3component::RED) return 8;
            return 0;

        case k3fmt::BC5_UNORM:
            if (component == k3component::RED || component == k3component::GREEN) return 8;
            return 0;

        case k3fmt::BC6_UNORM:
        case k3fmt::BC7_UNORM:
            // TODO: check on these
            return 8;
        default:
            return 0;
        }
    } else if (component == k3component::DEPTH || component == k3component::STENCIL) {
        switch (format) {
        case k3fmt::D24_UNORM_S8_UINT:
            if (component == k3component::DEPTH) return 24;
            return 8;
        case k3fmt::D32_FLOAT_S8X24_UINT:
            if (component == k3component::DEPTH) return 32;
            return 8;

        case k3fmt::D16_UNORM:
            if (component == k3component::DEPTH) return 16;
            return 0;
        case k3fmt::D24X8_UNORM:
            if (component == k3component::DEPTH) return 24;
            return 0;
        case k3fmt::D32_FLOAT:
            if (component == k3component::DEPTH) return 32;
            return 0;
        default:
            return 0;
        }
    } else {
        return 0;
    }
}

uint32_t k3imageObj::GetMaxComponentBits(k3fmt format)
{
    switch (format) {
    case k3fmt::RGBA32_UNORM:
    case k3fmt::RGBA32_FLOAT:
        return 32;
    case k3fmt::RGBA8_UNORM:
    case k3fmt::BGRA8_UNORM:
        return 8;
    case k3fmt::RGBA16_UNORM:
    case k3fmt::RGBA16_FLOAT:
        return 16;
    case k3fmt::RGB10A2_UNORM:
        return 10;
    case k3fmt::BGR5A1_UNORM:
        return 5;

    case k3fmt::RGB32_FLOAT:
        return 32;
    case k3fmt::RGBX8_UNORM:
    case k3fmt::BGRX8_UNORM:
    case k3fmt::RGB8_UNORM:
    case k3fmt::BGR8_UNORM:
        return 8;
    case k3fmt::B5G6R5_UNORM:
        return 6;
    case k3fmt::RG32_UNORM:
    case k3fmt::RG32_FLOAT:
        return 32;
    case k3fmt::RG16_UNORM:
    case k3fmt::RG16_FLOAT:
        return 16;

    case k3fmt::R32_UNORM:
    case k3fmt::R32_UINT:
    case k3fmt::R32_FLOAT:
        return 32;
    case k3fmt::RG8_UNORM:
        return 8;
    case k3fmt::R16_UNORM:
    case k3fmt::R16_UINT:
    case k3fmt::R16_FLOAT:
        return 16;
    case k3fmt::R8_UNORM:
        return 8;
    case k3fmt::A8_UNORM:
        return 8;

        // Compressed formats
    case k3fmt::BC1_UNORM:
        return 8;
    case k3fmt::BC2_UNORM:
    case k3fmt::BC3_UNORM:
        return 8;

    case k3fmt::BC4_UNORM:
        return 8;

    case k3fmt::BC5_UNORM:
        return 8;

    case k3fmt::BC6_UNORM:
    case k3fmt::BC7_UNORM:
        // TODO: check on these
        return 8;

    case k3fmt::D24_UNORM_S8_UINT:
        return 24;
    case k3fmt::D32_FLOAT_S8X24_UINT:
        return 32;

    case k3fmt::D16_UNORM:
        return 16;
    case k3fmt::D24X8_UNORM:
        return 24;
    case k3fmt::D32_FLOAT:
        return 32;
    default:
        return 0;
    }
}

int32_t k3imageObj::CalcFinalAddress(int32_t x, int32_t length, k3texAddr addr_mode)
{
    switch (addr_mode) {
    case k3texAddr::MIRROR:
        x = abs(x);
        x = x % (2 * length);
        x = (x >= length) ? 2 * length - x - 1 : x;
        break;
    case k3texAddr::MIRROR_ONCE:
        x = abs(x);
        x = (x >= length) ? length - 1 : x;
        break;
    case k3texAddr::CLAMP:
        x = (x < 0) ? 0 : ((x >= length) ? length - 1 : x);
        break;
    case k3texAddr::WRAP:
        x = (x < 0) ? (length - (abs(x) % length)) : x % length;
        break;
    }
    return x;
}

void* k3imageObj::GetSamplePointer(int32_t x, int32_t y, int32_t z,
    int32_t width, int32_t height, int32_t depth,
    uint32_t pitch, uint32_t slice_pitch,
    uint32_t format_size, uint32_t block_size,
    void* data,
    k3texAddr x_addr_mode,
    k3texAddr y_addr_mode,
    k3texAddr z_addr_mode,
    uint32_t* block_offset)
{
    const void* cdata = data;
    return const_cast<void*>(GetSamplePointer(x, y, z, width, height, depth, pitch, slice_pitch, format_size, block_size, cdata,
        x_addr_mode, y_addr_mode, z_addr_mode, block_offset));
}

// block size can be 1 or 4 for now
inline uint32_t calc_block_size_log2(uint32_t n)
{
    return (n == 4) ? 2 : 0;
}

const void* k3imageObj::GetSamplePointer(int32_t x, int32_t y, int32_t z,
    int32_t width, int32_t height, int32_t depth,
    uint32_t pitch, uint32_t slice_pitch,
    uint32_t format_size, uint32_t block_size,
    const void* data,
    k3texAddr x_addr_mode,
    k3texAddr y_addr_mode,
    k3texAddr z_addr_mode,
    uint32_t* block_offset)
{
    uint32_t block_size_log2 = calc_block_size_log2(block_size);

    if (x < 0 || x >= width)  x = CalcFinalAddress(x, width, x_addr_mode);
    if (y < 0 || y >= height) y = CalcFinalAddress(y, height, y_addr_mode);
    if (z < 0 || z >= depth)  z = CalcFinalAddress(z, depth, z_addr_mode);

    //uint32_t adj_width = (width + block_size - 1) >> block_size_log2;
    //uint32_t adj_height = (height + block_size - 1) >> block_size_log2;
    //uint32_t adj_slice = adj_width * adj_height;
    uint32_t block_x = x >> block_size_log2;
    uint32_t block_y = y >> block_size_log2;

    const uint8_t* u8data = static_cast<const uint8_t*>(data);
    uint32_t offset;
    offset = format_size * block_x + pitch * block_y + slice_pitch * z;
    u8data += offset;

    const void* vdata = static_cast<const void*>(u8data);

    if (block_offset) {
        uint32_t mask = ~(0xffffffff << block_size_log2);
        uint32_t offset_x = x & mask;
        uint32_t offset_y = y & mask;
        *block_offset = (offset_y << block_size_log2) + offset_x;
    }

    return vdata;
}

void k3imageObj::GetWeights(float start, float end, int32_t& istart, int32_t& iend, float* weights)
{
    float size;
    if (start > end) {
        size = end;
        end = start;
        start = size;
    }
    size = end - start;

    if (size <= 1.0f) {
        float norm_center = start + (size / 2.0f) - 0.5f;
        istart = static_cast<int32_t>(floorf(norm_center));
        iend = istart + 1;
        weights[2] = norm_center - istart;
        weights[0] = 1.0f - weights[2];
        weights[1] = weights[0];
    } else { // size > 1.0
        istart = static_cast<int32_t>(floorf(start));
        iend = static_cast<int32_t>(floorf(end));
        weights[0] = (1.0f - (start - floorf(start))) / size;
        weights[1] = 1.0f / size;
        weights[2] = (end - floorf(end)) / size;
    }

    if (weights[2] < 0.01) {
        iend--;
        weights[2] += weights[1];
    }
    if (weights[0] < 0.01) {
        istart++;
        weights[0] += (istart == iend) ? weights[2] : weights[1];
    }
}

void k3imageObj::ReformatBuffer(uint32_t src_width, uint32_t src_height, uint32_t src_depth,
    uint32_t src_pitch, uint32_t src_slice_pitch,
    k3fmt src_format, const void* src_data,
    uint32_t dest_width, uint32_t dest_height, uint32_t dest_depth,
    uint32_t dest_pitch, uint32_t dest_slice_pitch,
    k3fmt dest_format, void* dest_data,
    const float* transform,
    k3texAddr x_addr_mode,
    k3texAddr y_addr_mode,
    k3texAddr z_addr_mode)
{
    // If any dest parameters are set to 0, set them to the src
    if (dest_width == 0) dest_width = src_width;
    if (dest_height == 0) dest_height = src_height;
    if (dest_depth == 0) dest_depth = src_depth;
    if (dest_format == k3fmt::UNKNOWN) dest_format = src_format;

    float f32src[16 * 4];
    float f32dest[16 * 4];

    uint8_t u8src[16 * 4];
    uint8_t u8dest[16 * 4];
    uint32_t u32dest[4];

    // determine if we can use unorm8 as intermediary during conversion, instead of float32
    bool use_unorm8 = false;

    const void* src_pixel;
    const void* last_src_pixel = NULL;
    void* dest_pixel;

    float weights_x[3];
    float weights_y[3];
    float weights_z[3];

    float wx, wy, wz, pix_weight;

    uint32_t udx_block, udy_block, udx_block_offset, udy_block_offset, dest_block_offset;
    uint32_t udx, udy, udz;
    uint32_t src_block_offset;
    int32_t isx, isy, isz;
    int32_t isx_start, isy_start, isz_start;
    int32_t isx_end, isy_end, isz_end;
    //float fsx, fsy, fsz;
    float fsrc[4], fdest[4];
    float fsrc_start[4], fsrc_end[4];

    float local_xform_static[16];
    float* local_xform = NULL;
    float dest_to_norm[16] = { 2.0f / dest_width, 0.0f, 0.0f,  -1.0f,
                                   0.0f, 2.0f / dest_height, 0.0f, -1.0f,
                                   0.0f, 0.0f, 2.0f / dest_depth,  -1.0f,
                                   0.0f, 0.0f, 0.0f, 1.0f };
    float norm_to_src[16] = { src_width / 2.0f, 0.0f, 0.0f, src_width / 2.0f,
                                  0.0f, src_height / 2.0f, 0.0f, src_height / 2.0f,
                                  0.0f, 0.0f, src_depth / 2.0f, src_depth / 2.0f,
                                  0.0f, 0.0f, 0.0f, 1.0f };

    uint32_t dest_format_size = GetFormatSize(dest_format);
    uint32_t dest_block_size = GetFormatBlockSize(dest_format);

    uint32_t dest_block_width = (dest_width + dest_block_size - 1);
    uint32_t dest_block_height = (dest_height + dest_block_size - 1);

    uint32_t src2dest_width = 0, src2dest_height = 0, src2dest_depth = 0, src2dest_total = 0;
    uint32_t src_format_size = GetFormatSize(src_format);
    uint32_t src_block_size = GetFormatBlockSize(src_format);

    if (transform == NULL && (src_width % dest_width == 0) && (src_height % dest_height == 0) && (src_depth % dest_depth == 0)) {
        local_xform = NULL;
        src2dest_width = src_width / dest_width;
        src2dest_height = src_height / dest_height;
        src2dest_depth = src_depth / dest_depth;
        src2dest_total = src2dest_width * src2dest_height * src2dest_depth;
        if (GetMaxComponentBits(src_format) <= 8 || GetMaxComponentBits(dest_format) <= 8) use_unorm8 = true;
    } else {
        local_xform = local_xform_static;
        if (transform == NULL) {
            k3m4_Mul(local_xform, norm_to_src, dest_to_norm);
        } else {
            for (isx = 0; isx < 16; isx++) local_xform[isx] = transform[isx];
            k3m4_Inverse(local_xform);
            k3m4_Mul(local_xform, local_xform, dest_to_norm);
            k3m4_Mul(local_xform, norm_to_src, local_xform);
        }
    }

    fdest[3] = 1.0f;
    // Outer loops; loop through all pixel in the destination
    for (udz = 0; udz < dest_depth; udz++) {
        fdest[2] = static_cast<float>(udz);
        //fsz = udz * src_to_dest_z;
        //GetWeights( fsz, src_to_dest_z, isz_start, isz_end, weights_z );
        for (udy_block = 0; udy_block < dest_block_height; udy_block += dest_block_size) {

            for (udx_block = 0; udx_block < dest_block_width; udx_block += dest_block_size) {

                for (udy_block_offset = 0; udy_block_offset < dest_block_size; udy_block_offset++) {
                    udy = udy_block + udy_block_offset;
                    fdest[1] = static_cast<float>(udy);
                    //fsy = udy * src_to_dest_y;
                    //GetWeights( fsy, src_to_dest_y, isy_start, isy_end, weights_y );

                    for (udx_block_offset = 0; udx_block_offset < dest_block_size; udx_block_offset++) {
                        dest_block_offset = 4 * (dest_block_size * udy_block_offset + udx_block_offset);
                        udx = udx_block + udx_block_offset;
                        if (local_xform == NULL) {

                            if (src2dest_total == 1) {
                                src_pixel = GetSamplePointer(udx, udy, udz, src_width, src_height, src_depth, src_pitch, src_slice_pitch, src_format_size, src_block_size,
                                    src_data, x_addr_mode, y_addr_mode, z_addr_mode, &src_block_offset);
                                src_block_offset = src_block_offset * 4;
                                if (use_unorm8) {
                                    ConvertToUnorm8(src_format, src_pixel, u8src);
                                    u8dest[dest_block_offset + 0] = u8src[src_block_offset + 0];
                                    u8dest[dest_block_offset + 1] = u8src[src_block_offset + 1];
                                    u8dest[dest_block_offset + 2] = u8src[src_block_offset + 2];
                                    u8dest[dest_block_offset + 3] = u8src[src_block_offset + 3];
                                } else {
                                    ConvertToFloat4(src_format, src_pixel, f32src);
                                    f32dest[dest_block_offset + 0] = f32src[src_block_offset + 0];
                                    f32dest[dest_block_offset + 1] = f32src[src_block_offset + 1];
                                    f32dest[dest_block_offset + 2] = f32src[src_block_offset + 2];
                                    f32dest[dest_block_offset + 3] = f32src[src_block_offset + 3];
                                }
                            } else {

                                isx_start = src2dest_width * udx;
                                isy_start = src2dest_height * udy;
                                isz_start = src2dest_depth * udz;

                                isx_end = isx_start + src2dest_width;
                                isy_end = isy_start + src2dest_height;
                                isz_end = isz_start + src2dest_depth;

                                if (use_unorm8) {
                                    u32dest[0] = 0;
                                    u32dest[1] = 0;
                                    u32dest[2] = 0;
                                    u32dest[3] = 0;
                                } else {
                                    f32dest[dest_block_offset + 0] = 0.0f;
                                    f32dest[dest_block_offset + 1] = 0.0f;
                                    f32dest[dest_block_offset + 2] = 0.0f;
                                    f32dest[dest_block_offset + 3] = 0.0f;
                                }

                                for (isz = isz_start; isz < isz_end; isz++) {
                                    for (isy = isy_start; isy < isy_end; isy++) {
                                        for (isx = isx_start; isx < isx_end; isx++) {

                                            src_pixel = GetSamplePointer(isx, isy, isz, src_width, src_height, src_depth, src_pitch, src_slice_pitch, src_format_size, src_block_size,
                                                src_data, x_addr_mode, y_addr_mode, z_addr_mode, &src_block_offset);
                                            src_block_offset = src_block_offset * 4;
                                            if (use_unorm8) {
                                                ConvertToUnorm8(src_format, src_pixel, u8src);
                                                u32dest[0] += u8src[src_block_offset + 0];
                                                u32dest[1] += u8src[src_block_offset + 1];
                                                u32dest[2] += u8src[src_block_offset + 2];
                                                u32dest[3] += u8src[src_block_offset + 3];
                                            } else {
                                                ConvertToFloat4(src_format, src_pixel, f32src);
                                                f32dest[dest_block_offset + 0] += f32src[src_block_offset + 0];
                                                f32dest[dest_block_offset + 1] += f32src[src_block_offset + 1];
                                                f32dest[dest_block_offset + 2] += f32src[src_block_offset + 2];
                                                f32dest[dest_block_offset + 3] += f32src[src_block_offset + 3];
                                            }
                                        } // for(isx=isx_start; ...
                                    } // for(isy=isy_start; ...
                                } // for( isz=isz_start; ...
                                if (use_unorm8) {
                                    u32dest[0] /= src2dest_total;
                                    u32dest[1] /= src2dest_total;
                                    u32dest[2] /= src2dest_total;
                                    u32dest[3] /= src2dest_total;
                                    u8dest[dest_block_offset + 0] = static_cast<uint8_t>((u32dest[0] > 0xff) ? 0xff : u32dest[0]);
                                    u8dest[dest_block_offset + 1] = static_cast<uint8_t>((u32dest[1] > 0xff) ? 0xff : u32dest[1]);
                                    u8dest[dest_block_offset + 2] = static_cast<uint8_t>((u32dest[2] > 0xff) ? 0xff : u32dest[2]);
                                    u8dest[dest_block_offset + 3] = static_cast<uint8_t>((u32dest[3] > 0xff) ? 0xff : u32dest[3]);
                                } else {
                                    f32dest[dest_block_offset + 0] /= src2dest_total;
                                    f32dest[dest_block_offset + 1] /= src2dest_total;
                                    f32dest[dest_block_offset + 2] /= src2dest_total;
                                    f32dest[dest_block_offset + 3] /= src2dest_total;
                                }
                            } // if( src2dest_total == 1 )

                        } else {
                            fdest[0] = static_cast<float>(udx);
                            //fsx = udx * src_to_dest_x;
                            //GetWeights( fsx, src_to_dest_x, isx_start, isx_end, weights_x );

                            k3mv4_Mul(fsrc_start, local_xform, fdest);
                            fsrc_end[0] = fsrc_start[0];
                            fsrc_end[1] = fsrc_start[1];
                            fsrc_end[2] = fsrc_start[2];
                            fsrc_end[3] = fsrc_start[3];

                            fdest[0] += 1.0f;
                            k3mv4_Mul(fsrc, local_xform, fdest);
                            k3v4_Min(fsrc_start, fsrc, fsrc_start);
                            k3v4_Max(fsrc_end, fsrc, fsrc_end);

                            fdest[0] -= 1.0f; fdest[1] += 1.0f;
                            k3mv4_Mul(fsrc, local_xform, fdest);
                            k3v4_Min(fsrc_start, fsrc, fsrc_start);
                            k3v4_Max(fsrc_end, fsrc, fsrc_end);

                            fdest[1] -= 1.0f; fdest[2] += 1.0f;
                            k3mv4_Mul(fsrc, local_xform, fdest);
                            k3v4_Min(fsrc_start, fsrc, fsrc_start);
                            k3v4_Max(fsrc_end, fsrc, fsrc_end);

                            fdest[2] -= 1.0f;
                            k3v4s_Div(fsrc_start, fsrc_start, fsrc_start[3]);
                            k3v4s_Div(fsrc_end, fsrc_end, fsrc_end[3]);

                            GetWeights(fsrc_start[0], fsrc_end[0], isx_start, isx_end, weights_x);
                            GetWeights(fsrc_start[1], fsrc_end[1], isy_start, isy_end, weights_y);
                            GetWeights(fsrc_start[2], fsrc_end[2], isz_start, isz_end, weights_z);

                            // Initialize the current pixel
                            f32dest[dest_block_offset + 0] = 0.0;
                            f32dest[dest_block_offset + 1] = 0.0;
                            f32dest[dest_block_offset + 2] = 0.0;
                            f32dest[dest_block_offset + 3] = 0.0;

                            // Inner loops; loop through all src pixels that are covered by this dest pixel
                            for (isz = isz_start; isz <= isz_end; isz++) {
                                wz = (isz == isz_start) ? weights_z[0] : ((isz == isz_end) ? weights_z[2] : weights_z[1]);
                                for (isy = isy_start; isy <= isy_end; isy++) {
                                    wy = (isy == isy_start) ? weights_y[0] : ((isy == isy_end) ? weights_y[2] : weights_y[1]);
                                    for (isx = isx_start; isx <= isx_end; isx++) {
                                        wx = (isx == isx_start) ? weights_x[0] : ((isx == isx_end) ? weights_x[2] : weights_x[1]);

                                        pix_weight = wx * wy * wz;
                                        src_pixel = GetSamplePointer(isx, isy, isz, src_width, src_height, src_depth, src_pitch, src_slice_pitch, src_format_size, src_block_size,
                                            src_data, x_addr_mode, y_addr_mode, z_addr_mode, &src_block_offset);
                                        src_block_offset = src_block_offset * 4;

                                        if (src_pixel != last_src_pixel) {
                                            ConvertToFloat4(src_format, src_pixel, f32src);
                                            last_src_pixel = src_pixel;
                                        }
                                        f32dest[dest_block_offset + 0] += pix_weight * f32src[src_block_offset + 0];
                                        f32dest[dest_block_offset + 1] += pix_weight * f32src[src_block_offset + 1];
                                        f32dest[dest_block_offset + 2] += pix_weight * f32src[src_block_offset + 2];
                                        f32dest[dest_block_offset + 3] += pix_weight * f32src[src_block_offset + 3];
                                    } // for (isx...
                                } // for (isy...
                            } // for (isz...

                        } // if( local_xform == NULL ) {

                    } // for (udx_block_offset...
                } // for (udy_block_offset...

                dest_pixel = GetSamplePointer(udx_block, udy_block, udz, dest_width, dest_height, dest_depth, dest_pitch, dest_slice_pitch, dest_format_size, dest_block_size, dest_data);
                if (use_unorm8) {
                    ConvertFromUnorm8(dest_format, u8dest, dest_pixel);
                } else {
                    ConvertFromFloat4(dest_format, f32dest, dest_pixel);
                }
            } // for (udx_block...
        } // for (udy_block...
    } // for (udz
}

void k3imageObj::SampleBuffer(float x, float y, float z,
    uint32_t width, uint32_t height, uint32_t depth,
    uint32_t pitch, uint32_t slice_pitch,
    k3fmt format, const void* data, float* color,
    k3texAddr x_addr_mode,
    k3texAddr y_addr_mode,
    k3texAddr z_addr_mode)
{
    uint32_t num_samples = 1;
    float ux = x * width;
    float uy = y * height;
    float uz = z * depth;
    int32_t start_x = static_cast<int32_t>(ux);
    int32_t start_y = static_cast<int32_t>(uy);
    int32_t start_z = static_cast<int32_t>(uz);
    int32_t end_x = start_x;
    int32_t end_y = start_y;
    int32_t end_z = start_z;
    uint32_t format_size = GetFormatSize(format);
    uint32_t block_size = GetFormatBlockSize(format);
    float weights[8] = { 1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f };

    uint32_t s;

    ux -= start_x;
    uy -= start_y;
    uz -= start_z;

    if (width > 1) {
        weights[1] = ux;
        weights[0] = 1.0f - ux;
        num_samples *= 2;
        end_x++;
    }
    if (height > 1) {
        for (s = 0; s < num_samples; s++) {
            weights[num_samples + s] = weights[s] * uy;
            weights[s] = weights[s] * (1.0f - uy);
        }
        num_samples *= 2;
        end_y++;
    }
    if (depth > 1) {
        for (s = 0; s < num_samples; s++) {
            weights[num_samples + s] = weights[s] * uz;
            weights[s] = weights[s] * (1.0f - uz);
        }
        num_samples *= 2;
        end_z++;
    }

    int32_t cur_x, cur_y, cur_z;
    const void* formatted_sample;
    float fp32_sample[4];
    float* cur_weight = weights;
    color[0] = 0.0f;
    color[1] = 0.0f;
    color[2] = 0.0f;
    color[3] = 0.0f;
    for (cur_z = start_z; cur_z <= end_z; cur_z++) {
        for (cur_y = start_y; cur_y <= end_y; cur_y++) {
            for (cur_x = start_x; cur_x <= end_x; cur_x++) {
                formatted_sample = GetSamplePointer(cur_x, cur_y, cur_z,
                    width, height, depth,
                    pitch, slice_pitch,
                    format_size, block_size,
                    data,
                    x_addr_mode, y_addr_mode, z_addr_mode,
                    NULL);
                ConvertToFloat4(format, formatted_sample, fp32_sample);
                color[0] += fp32_sample[0] * *cur_weight;
                color[1] += fp32_sample[1] * *cur_weight;
                color[2] += fp32_sample[2] * *cur_weight;
                color[3] += fp32_sample[3] * *cur_weight;
                cur_weight++;
            }
        }
    }
}

