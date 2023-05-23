// k3 graphics library
// functions to load and save PNG files

#include "k3internal.h"
#include "pnghandler.h"

// PNG constants and structures
// in network order, will be "\211PNG\r\n \n"
#ifdef K3_BIG_ENDIAN
const uint64_t PNG_SIGNATURE = 0x89504e470d0a1a0a;
#else
const uint64_t PNG_SIGNATURE = 0x0a1a0a0d474e5089;
#endif

uint32_t k3_endian_swap32(uint32_t in)
{
#ifdef K3_BIG_ENDIAN
    return in;
#else
    return ((in & 0xff) << 24) | ((in & 0xff00) << 8) | ((in & 0xff0000) >> 8) | ((in & 0xff000000) >> 24);
#endif
}

uint16_t k3_endian_swap16(uint16_t in)
{
#ifdef K3_BIG_ENDIAN
    return in;
#else
    return ((in & 0xff) << 8) | ((in & 0xff00) >> 8);
#endif
}

struct png_chunk_t {
    uint32_t length;
    uint32_t type;
};

const uint32_t PNG_CHUNK_IHDR = FOURCC("IHDR");
const uint32_t PNG_CHUNK_PLTE = FOURCC("PLTE");
const uint32_t PNG_CHUNK_IDAT = FOURCC("IDAT");
const uint32_t PNG_CHUNK_IEND = FOURCC("IEND");

void png_chunk_endian_swap(png_chunk_t* in)
{
    in->length = k3_endian_swap32(in->length);
    // dont endian swap type; encodings are already endian swapped
}

#pragma pack(push, 1)
struct png_ihdr_t {
    uint64_t sig;
    png_chunk_t ihdr_chunk;
    uint32_t width;
    uint32_t height;
    uint8_t bit_depth;
    uint8_t color_type;
    uint8_t compression_method;
    uint8_t filter_method;
    uint8_t interlace_method;
    uint32_t crc;
};
#pragma pack(pop)

void png_ihdr_endian_swap(png_ihdr_t* in)
{
    png_chunk_endian_swap(&(in->ihdr_chunk));
    in->width = k3_endian_swap32(in->width);
    in->height = k3_endian_swap32(in->height);
    in->crc = k3_endian_swap32(in->crc);
}

const uint8_t PNG_COLOR_TYPE_NO_FLAG = 0x0;
const uint8_t PNG_COLOR_TYPE_PALETTE_FLAG = 0x1;
const uint8_t PNG_COLOR_TYPE_COLOR_FLAG = 0x2;
const uint8_t PNG_COLOR_TYPE_ALPHA_FLAG = 0x4;

// These are the only legal flag  combinations for color type
const uint8_t PNG_COLOR_TYPE_GRAYSCALE = PNG_COLOR_TYPE_NO_FLAG;
const uint8_t PNG_COLOR_TYPE_COLOR_RGB = PNG_COLOR_TYPE_COLOR_FLAG;
const uint8_t PNG_COLOR_TYPE_PALETTE_RGB = PNG_COLOR_TYPE_COLOR_FLAG | PNG_COLOR_TYPE_PALETTE_FLAG;
const uint8_t PNG_COLOR_TYPE_GRAYSCALE_ALPHA = PNG_COLOR_TYPE_ALPHA_FLAG;
const uint8_t PNG_COLOR_TYPE_COLOR_RGBA = PNG_COLOR_TYPE_COLOR_FLAG | PNG_COLOR_TYPE_ALPHA_FLAG;

// This array is indexed by the color type, and the value represents a bit mask of supported bit depths
const uint8_t PNG_LEGAL_BIT_DEPTHS_PER_COLOR_TYPE[7] = {
    0x1f,   // gray scale supports all bit depths
    0x00,   // illegal color type
    0x18,   // color rgb supports 8 or 16 bits
    0x0f,   // palette supports up to 8 bits
    0x18,   // gray scale + alpha support 8 or 16 bits
    0x00,   // illegal color type
    0x18    // color rgba support 8 or 16 bits
};

const uint8_t PNG_INTERLACE_NONE = 0;
const uint8_t PNG_INTERLACE_ADAM7 = 1;

const uint8_t PNG_FILTER_NONE = 0;
const uint8_t PNG_FILTER_SUB = 1;
const uint8_t PNG_FILTER_UP = 2;
const uint8_t PNG_FILTER_AVG = 3;
const uint8_t PNG_FILTER_PAETH = 4;

struct png_palette_entry_t {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

k3image_file_handler_t k3PNGHandler = { k3png_LoadHeaderInfo,
                                        k3png_LoadData,
                                        k3png_SaveData };

png_ihdr_t header;
k3fmt img_format;

void K3CALLBACK k3png_LoadHeaderInfo(FILE* file_handle,
    uint32_t* width, uint32_t* height, uint32_t* depth, k3fmt* format)
{
    uint32_t start_pos = ftell(file_handle);
    fread(&header, sizeof(png_ihdr_t), 1, file_handle);
    png_ihdr_endian_swap(&header);

    // Do error checking on the header
    if (header.sig != PNG_SIGNATURE ||              // Do we have the PNG signature
        header.ihdr_chunk.type != PNG_CHUNK_IHDR || // is the first chunk IHDR
        header.width == 0 || header.height == 0 ||  // width/height non-zero
        header.color_type > 6 ||                    // check for legal color type
        ((PNG_LEGAL_BIT_DEPTHS_PER_COLOR_TYPE[header.color_type] & header.bit_depth) == 0x0) || // Check for legal bit depth & color type conbimation
        header.compression_method != 0 ||           // 0 is only legal compression method
        header.filter_method != 0 ||                // 0 is only legal filter method
        !(header.interlace_method == PNG_INTERLACE_NONE ||
            header.interlace_method == PNG_INTERLACE_ADAM7)  // 0 or 1 are only legal interlace methods
        ) {
        fseek(file_handle, start_pos, SEEK_SET);
        return;
    }

    *width = header.width;
    *height = header.height;
    *depth = 1;
    switch (header.color_type) {
    case PNG_COLOR_TYPE_GRAYSCALE:
        img_format = (header.bit_depth == 16) ? k3fmt::R16_UNORM : k3fmt::R8_UNORM;
        break;
    case PNG_COLOR_TYPE_COLOR_RGB:
    case PNG_COLOR_TYPE_COLOR_RGBA:
        img_format = (header.bit_depth == 16) ? k3fmt::RGBA16_UNORM : k3fmt::RGBA8_UNORM;
        break;
    case PNG_COLOR_TYPE_PALETTE_RGB:
        img_format = k3fmt::RGBA8_UNORM;
        break;
    case PNG_COLOR_TYPE_GRAYSCALE_ALPHA:
        img_format = (header.bit_depth == 16) ? k3fmt::RG16_UNORM : k3fmt::RG8_UNORM;
        break;
    default:
        img_format = k3fmt::UNKNOWN;
    }
    *format = img_format;
}

uint8_t k3png_paeth(uint8_t a, uint8_t b, uint8_t c)
{
    int32_t pab = a + b;
    int32_t c2 = c * 2;
    int32_t pa = (b > c) ? b - c : c - b;
    int32_t pb = (a > c) ? a - c : c - a;
    int32_t pc = (pab > c2) ? pab - c2 : c2 - pab;
    if (pb < pa) {
        pa = pb;
        a = b;
    }
    return (pc < pa) ? c : a;
}

void k3png_defilter_scanline(uint8_t* dst, uint8_t* prev_dst, uint32_t pix_pitch, uint32_t row_pitch, uint8_t filter_type)
{
    uint32_t col;
    uint8_t prev_col, prev_row, prev_row_col;
    switch (filter_type) {
    case PNG_FILTER_SUB:
        for (col = pix_pitch; col < row_pitch; col++) {
            *(dst + col) += *(dst + col - pix_pitch);
        }
        break;
    case PNG_FILTER_UP:
        for (col = 0; col < row_pitch; col++) {
            *(dst + col) += *(prev_dst + col);
        }
        break;
    case PNG_FILTER_AVG:
        for (col = 0; col < row_pitch; col++) {
            prev_col = *(dst + col - pix_pitch);
            prev_row = *(prev_dst + col);
            *(dst + col) += (prev_row + prev_col) >> 1;
        }
        break;
    case PNG_FILTER_PAETH:
        for (col = 0; col < row_pitch; col++) {
            prev_col = *(dst + col - pix_pitch);
            prev_row = *(prev_dst + col);
            prev_row_col = *(prev_dst + col - pix_pitch);
            *(dst + col) += k3png_paeth(prev_col, prev_row, prev_row_col);
        }
        break;
    }
}

void k3png_defilter(uint8_t* dst, uint8_t* src, uint32_t pix_pitch, uint32_t row_pitch, uint32_t height)
{
    uint32_t row, col;
    uint8_t filter_type;
    uint8_t* cur_src = src;
    uint8_t* cur_dst = dst;
    uint8_t* prev_dst = NULL;
    for (row = 0; row < height; row++) {
        filter_type = *cur_src;
        if (filter_type > 4) {
            // bad filter type
            filter_type = 0;
        }
        cur_src++;
        memcpy(cur_dst, cur_src, row_pitch);
        k3png_defilter_scanline(cur_dst, prev_dst, pix_pitch, row_pitch, filter_type);
        prev_dst = cur_dst;
        cur_dst += row_pitch;
        cur_src += row_pitch;
    }

}

void K3CALLBACK k3png_LoadData(FILE* file_handle, uint32_t pitch, uint32_t slice_pitch, void* data)
{
    png_chunk_t chunk;
    bool palette_found = false;
    png_palette_entry_t palette[256] = { 0 };
    uint32_t i, p, b, length, crc;
    z_stream zs = { 0 };
    // stride and offset for each plane in x and y directions
    // represented in this grid for png spec
    // (plane 0 represents uninterlaced mode)
    //    1 6 4 6 2 6 4 6
    //    7 7 7 7 7 7 7 7
    //    5 6 5 6 5 6 5 6
    //    7 7 7 7 7 7 7 7
    //    3 6 4 6 3 6 4 6
    //    7 7 7 7 7 7 7 7
    //    5 6 5 6 5 6 5 6
    //    7 7 7 7 7 7 7 7
    uint32_t stride_x[8] = { 1, 8, 8, 4, 4, 2, 2, 1 };
    uint32_t stride_y[8] = { 1, 8, 8, 8, 4, 4, 2, 2 };
    uint32_t offset_x[8] = { 0, 0, 4, 0, 2, 0, 1, 0 };
    uint32_t offset_y[8] = { 0, 0, 0, 4, 0, 2, 0, 1 };
    // source pitch in bytes; round up to nearest byte
    // index 0 for base image
    // 1-7 for planes 1 to 7 in interlave mode
    uint32_t pix_per_row[8];
    uint32_t src_pitch[8];
    uint32_t src_height[8];
    uint32_t src_size[8];
    // pitch for scanline buffer
    // prepend 1 pixel pad to handle the first pixel's prior neightbor
    uint32_t scanline_pitch = ((header.bit_depth * header.width) + 64) / 8;

    for (i = 0; i < 8; i++) {
        pix_per_row[i] = ((header.width + stride_x[i] - 1 - offset_x[i]) / stride_x[i]);
        src_pitch[i] = ((header.bit_depth * pix_per_row[i]) + 7) / 8;
        src_height[i] = ((header.height + stride_y[i] - 1 - offset_y[i]) / stride_y[i]);
    }

    uint32_t dst_pix_pitch = k3imageObj::GetFormatSize(img_format);
    uint32_t pix_pitch = header.bit_depth;

    // for color data, there are either 3 components (RGB) or 4 (RGBA)
    // Grayscale can have 1 or 2
    uint32_t num_comp = (header.color_type & PNG_COLOR_TYPE_COLOR_FLAG) ? 3 : 1;
    if (header.color_type & PNG_COLOR_TYPE_ALPHA_FLAG) num_comp++;

    if (!(header.color_type & PNG_COLOR_TYPE_PALETTE_FLAG)) {
        scanline_pitch *= num_comp;
        pix_pitch *= num_comp;
        for (i = 0; i < 8; i++) {
            src_pitch[i] *= num_comp;
        }
    }

    // round up to nearest byte
    pix_pitch = (pix_pitch + 7) / 8;
    for (i = 0; i < 8; i++) {
        src_size[i] = (src_pitch[i] + 1) * src_height[i];  // add 1 byte for filter mode
    }

    uint32_t dst_size = pitch * header.height;
    uint32_t read_size = src_size[0];
    if (header.interlace_method == PNG_INTERLACE_ADAM7) {
        // interlaced mode
        read_size = 0;
        for (i = 1; i < 8; i++) {
            read_size += src_size[i];
        }
    }

    uint8_t* src_buffer = new uint8_t[3 * scanline_pitch];
    uint8_t* src0 = (uint8_t*)src_buffer;
    uint8_t* src1 = src0 + scanline_pitch;
    uint8_t* raw_read = src1 + scanline_pitch;
    uint32_t bytes_remaining;

    // clear the scanline to 0
    memset(src_buffer, 0, 2 * scanline_pitch);

    uint8_t filter_type;
    uint32_t row = 0;
    uint32_t plane = (header.interlace_method == PNG_INTERLACE_ADAM7) ? 1 : 0;
    uint8_t* cur_dst = (uint8_t*)data;
    bool done = false;
    zs.zalloc = Z_NULL;
    zs.zfree = Z_NULL;
    zs.opaque = Z_NULL;
    inflateInit(&zs);
    zs.avail_out = src_pitch[plane] + 1;
    zs.next_out = src1 + 7;
    bool img_done = false;
#ifdef K3_BIG_ENDIAN
    uint32_t endian_swap = 0;
#else
    uint32_t endian_swap = (header.bit_depth == 16) ? 0x1 : 0x0;
#endif
    while (!done) {
        fread(&chunk, sizeof(png_chunk_t), 1, file_handle);
        png_chunk_endian_swap(&chunk);
        switch (chunk.type) {
        case PNG_CHUNK_PLTE:
            length = chunk.length / 3;
            if (length > 256) length = 256;
            fread(palette, sizeof(png_palette_entry_t), length, file_handle);
            length = chunk.length - 3 * length;
            if (length) fseek(file_handle, length, SEEK_CUR);
            palette_found = true;
            break;
        case PNG_CHUNK_IDAT:
            // read and decompress data
            if (zs.avail_out) {
                bytes_remaining = chunk.length;
                while (bytes_remaining) {
                    zs.avail_in = (bytes_remaining > scanline_pitch) ? scanline_pitch : bytes_remaining;
                    zs.next_in = raw_read;
                    fread(raw_read, 1, zs.avail_in, file_handle);
                    // TODO: at this point, should compute running CRC
                    bytes_remaining -= zs.avail_in;
                    while (zs.avail_in) {
                        int err = inflate(&zs, Z_NO_FLUSH);

                        if (zs.avail_out == 0 && !img_done) {
                            // We're done reading scanline, so start processing it
                            uint8_t filter_type = *(src1 + 7);
                            *(src1 + 7) = 0;
                            k3png_defilter_scanline(src1 + 8, src0 + 8, pix_pitch, src_pitch[plane], filter_type);
                            if (plane || (header.bit_depth < 8) || (header.color_type & PNG_COLOR_TYPE_PALETTE_FLAG)) {
                                // Take the slow path if we have interlaced image, sub-byte bit depth or palette
                                if (plane < 8) {
                                    uint32_t pix_per_byte = (header.bit_depth < 8) ? 8 / header.bit_depth : 1;
                                    uint8_t mask = (1 << header.bit_depth) - 1;
                                    p = 0;
                                    b = 0;
                                    uint32_t pix_left = pix_per_row[plane];
                                    uint32_t subbyte, end_subbyte;
                                    uint8_t src, d, j;
                                    for (i = 0; i < src_pitch[plane]; i++) {
                                        src = *(src1 + 8 + (i ^ endian_swap));
                                        end_subbyte = (pix_left < pix_per_byte) ? pix_left : pix_per_byte;
                                        for (subbyte = 0; subbyte < end_subbyte; subbyte++) {
                                            d = (src >> ((pix_per_byte - 1 - subbyte) * header.bit_depth)) & mask;
                                            if (header.color_type & PNG_COLOR_TYPE_PALETTE_FLAG) {
                                                for (j = 0; j < dst_pix_pitch; j++) {
                                                    *(cur_dst + p + b + (dst_pix_pitch * stride_x[plane] * subbyte) + j) = (j < 3) ? *((&(palette[d].r)) + j) : 0xff;
                                                }
                                            } else {
                                                d |= (header.bit_depth < 8) ? (d << 4) : 0;
                                                d |= (header.bit_depth < 4) ? (d << 2) : 0;
                                                d |= (header.bit_depth < 2) ? (d << 1) : 0;
                                                *(cur_dst + p + b + (dst_pix_pitch * stride_x[plane] * subbyte)) = d;
                                            }
                                            if(pix_pitch == 1) pix_left--;
                                        }
                                        b++;
                                        if (b >= pix_pitch) {
                                            if (pix_pitch > 1) pix_left--;
                                            b = 0;
                                            p += dst_pix_pitch * stride_x[plane] * pix_per_byte;
                                        }
                                    }
                                    cur_dst += pitch * stride_y[plane];
                                }
                            } else {
                                if (pix_pitch == dst_pix_pitch) {
                                    if (endian_swap) {
                                        // for 16bpc, can't just do a memcpy...need to do endian swap
                                        for (i = 0; i < src_pitch[0]; i++) {
                                            *(cur_dst + i) = *(src1 + 8 + (i ^ 0x1));
                                        }
                                    } else {
                                        memcpy(cur_dst, src1 + 8, src_pitch[0]);
                                    }
                                } else {
                                    b = 0;
                                    p = 0;
                                    for (i = 0; i < pitch; i++) {
                                        *(cur_dst + i) = (b < pix_pitch) ? *(src1 + 8 + (p ^ endian_swap)) : 0xff;
                                        if (b < pix_pitch) p++;
                                        b++;
                                        if (b >= dst_pix_pitch) b = 0;
                                    }
                                }
                                cur_dst += pitch;
                            }
                            uint8_t* temp = src0;
                            src0 = src1;
                            src1 = temp;
                            row++;
                            zs.next_out = src1 + 7;
                            if (row < src_height[plane]) {
                                zs.avail_out = src_pitch[plane] + 1;
                            } else if (plane > 0) {
                                row = 0;
                                do {
                                    plane++;
                                } while (plane < 8 && (pix_per_row[plane] == 0 || src_height[plane] == 0));
                                // reset cur_dst, but offset it by row for the last plane
                                cur_dst = (uint8_t*)data + offset_x[plane] * dst_pix_pitch + offset_y[plane] * pitch;
                                if (plane < 8) {
                                    zs.avail_out = src_pitch[plane] + 1;
                                } else {
                                    img_done = true;
                                }
                            } else {
                                img_done = true;
                            }
                        }
                    }
                }
            } else {
                // this shouldn't be...we have more data than expected for the image
                fseek(file_handle, chunk.length, SEEK_CUR);
            }
            break;
        case PNG_CHUNK_IEND:
            done = true;
            break;
        default:
            fseek(file_handle, chunk.length, SEEK_CUR);
            break;
        }
        fread(&crc, sizeof(crc), 1, file_handle);
        // TODO: compute and check CRC
        if (feof(file_handle)) done = true;
    }

    delete[] src_buffer;
}

void K3CALLBACK k3png_SaveData(FILE* file_handle,
    uint32_t width, uint32_t height, uint32_t depth,
    uint32_t pitch, uint32_t slice_pitch, k3fmt format,
    const void* data)
{

}
