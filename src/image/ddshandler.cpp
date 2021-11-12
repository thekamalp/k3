// k3 graphics library
// functions to load and save DDS files

#include "k3internal.h"
#include "ddshandler.h"

// DDS constants and structures
const uint32_t FOURCC_DDS = FOURCC("DDS ");

const uint32_t DDSD_CAPS = 0x00000001;
const uint32_t DDSD_HEIGHT = 0x00000002;
const uint32_t DDSD_WIDTH = 0x00000004;
const uint32_t DDSD_PITCH = 0x00000008;
const uint32_t DDSD_PIXELFORMAT = 0x00001000;
const uint32_t DDSD_MIPMAPCOUNT = 0x00020000;
const uint32_t DDSD_LINEARSIZE = 0x00080000;
const uint32_t DDSD_DEPTH = 0x00800000;

const uint32_t DDPF_ALPHAPIXELS = 0x00000001;
const uint32_t DDPF_FOURCC = 0x00000004;
const uint32_t DDPF_RGB = 0x00000040;

const uint32_t DDSCAPS_COMPLEX = 0x00000008;
const uint32_t DDSCAPS_TEXTURE = 0x00001000;
const uint32_t DDSCAPS_MIPMAP = 0x00400000;

const uint32_t DDSCAPS2_CUBEMAP = 0x00000200;
const uint32_t DDSCAPS2_CUBEMAP_POSITIVEX = 0x00000400;
const uint32_t DDSCAPS2_CUBEMAP_NEGATIVEX = 0x00000800;
const uint32_t DDSCAPS2_CUBEMAP_POSITIVEY = 0x00001000;
const uint32_t DDSCAPS2_CUBEMAP_NEGATIVEY = 0x00002000;
const uint32_t DDSCAPS2_CUBEMAP_POSITIVEZ = 0x00004000;
const uint32_t DDSCAPS2_CUBEMAP_NEGATIVEZ = 0x00008000;
const uint32_t DDSCAPS2_VOLUME = 0x00200000;

struct k3dds_format {
    uint32_t format_size;
    uint32_t flags;
    uint32_t fourcc;
    uint32_t rgb_bit_count;
    uint32_t rbit_mask;
    uint32_t gbit_mask;
    uint32_t bbit_mask;
    uint32_t abit_mask;
};

struct k3DDSHeader {
    uint32_t id;
    uint32_t header_size;
    uint32_t flags;
    uint32_t height;
    uint32_t width;
    uint32_t pitch_or_linear_size;
    uint32_t depth;
    uint32_t mipmap_count;
    uint32_t reserved1[11];
    k3dds_format pixel_format;
    uint32_t caps1;
    uint32_t caps2;
    uint32_t reserved2[3];
};

enum class k3DXFmt {
    UNKNOWN = 0,
    R32G32B32A32_TYPELESS = 1,
    R32G32B32A32_FLOAT = 2,
    R32G32B32A32_UINT = 3,
    R32G32B32A32_SINT = 4,
    R32G32B32_TYPELESS = 5,
    R32G32B32_FLOAT = 6,
    R32G32B32_UINT = 7,
    R32G32B32_SINT = 8,
    R16G16B16A16_TYPELESS = 9,
    R16G16B16A16_FLOAT = 10,
    R16G16B16A16_UNORM = 11,
    R16G16B16A16_UINT = 12,
    R16G16B16A16_SNORM = 13,
    R16G16B16A16_SINT = 14,
    R32G32_TYPELESS = 15,
    R32G32_FLOAT = 16,
    R32G32_UINT = 17,
    R32G32_SINT = 18,
    R32G8X24_TYPELESS = 19,
    D32_FLOAT_S8X24_UINT = 20,
    R32_FLOAT_X8X24_TYPELESS = 21,
    X32_TYPELESS_G8X24_UINT = 22,
    R10G10B10A2_TYPELESS = 23,
    R10G10B10A2_UNORM = 24,
    R10G10B10A2_UINT = 25,
    R11G11B10_FLOAT = 26,
    R8G8B8A8_TYPELESS = 27,
    R8G8B8A8_UNORM = 28,
    R8G8B8A8_UNORM_SRGB = 29,
    R8G8B8A8_UINT = 30,
    R8G8B8A8_SNORM = 31,
    R8G8B8A8_SINT = 32,
    R16G16_TYPELESS = 33,
    R16G16_FLOAT = 34,
    R16G16_UNORM = 35,
    R16G16_UINT = 36,
    R16G16_SNORM = 37,
    R16G16_SINT = 38,
    R32_TYPELESS = 39,
    D32_FLOAT = 40,
    R32_FLOAT = 41,
    R32_UINT = 42,
    R32_SINT = 43,
    R24G8_TYPELESS = 44,
    D24_UNORM_S8_UINT = 45,
    R24_UNORM_X8_TYPELESS = 46,
    X24_TYPELESS_G8_UINT = 47,
    R8G8_TYPELESS = 48,
    R8G8_UNORM = 49,
    R8G8_UINT = 50,
    R8G8_SNORM = 51,
    R8G8_SINT = 52,
    R16_TYPELESS = 53,
    R16_FLOAT = 54,
    D16_UNORM = 55,
    R16_UNORM = 56,
    R16_UINT = 57,
    R16_SNORM = 58,
    R16_SINT = 59,
    R8_TYPELESS = 60,
    R8_UNORM = 61,
    R8_UINT = 62,
    R8_SNORM = 63,
    R8_SINT = 64,
    A8_UNORM = 65,
    R1_UNORM = 66,
    R9G9B9E5_SHAREDEXP = 67,
    R8G8_B8G8_UNORM = 68,
    G8R8_G8B8_UNORM = 69,
    BC1_TYPELESS = 70,
    BC1_UNORM = 71,
    BC1_UNORM_SRGB = 72,
    BC2_TYPELESS = 73,
    BC2_UNORM = 74,
    BC2_UNORM_SRGB = 75,
    BC3_TYPELESS = 76,
    BC3_UNORM = 77,
    BC3_UNORM_SRGB = 78,
    BC4_TYPELESS = 79,
    BC4_UNORM = 80,
    BC4_SNORM = 81,
    BC5_TYPELESS = 82,
    BC5_UNORM = 83,
    BC5_SNORM = 84,
    B5G6R5_UNORM = 85,
    B5G5R5A1_UNORM = 86,
    B8G8R8A8_UNORM = 87,
    B8G8R8X8_UNORM = 88,
    R10G10B10_XR_BIAS_A2_UNORM = 89,
    B8G8R8A8_TYPELESS = 90,
    B8G8R8A8_UNORM_SRGB = 91,
    B8G8R8X8_TYPELESS = 92,
    B8G8R8X8_UNORM_SRGB = 93,
    BC6H_TYPELESS = 94,
    BC6H_UF16 = 95,
    BC6H_SF16 = 96,
    BC7_TYPELESS = 97,
    BC7_UNORM = 98,
    BC7_UNORM_SRGB = 99,
    AYUV = 100,
    Y410 = 101,
    Y416 = 102,
    NV12 = 103,
    P010 = 104,
    P016 = 105,
    I420_OPAQUE = 106,
    YUY2 = 107,
    Y210 = 108,
    Y216 = 109,
    NV11 = 110,
    AI44 = 111,
    IA44 = 112,
    P8 = 113,
    A8P8 = 114,
    B4G4R4A4_UNORM = 115,
    P208 = 130,
    V208 = 131,
    V408 = 132,
    ASTC_4X4_UNORM = 134,
    ASTC_4X4_UNORM_SRGB = 135,
    ASTC_5X4_TYPELESS = 137,
    ASTC_5X4_UNORM = 138,
    ASTC_5X4_UNORM_SRGB = 139,
    ASTC_5X5_TYPELESS = 141,
    ASTC_5X5_UNORM = 142,
    ASTC_5X5_UNORM_SRGB = 143,
    ASTC_6X5_TYPELESS = 145,
    ASTC_6X5_UNORM = 146,
    ASTC_6X5_UNORM_SRGB = 147,
    ASTC_6X6_TYPELESS = 149,
    ASTC_6X6_UNORM = 150,
    ASTC_6X6_UNORM_SRGB = 151,
    ASTC_8X5_TYPELESS = 153,
    ASTC_8X5_UNORM = 154,
    ASTC_8X5_UNORM_SRGB = 155,
    ASTC_8X6_TYPELESS = 157,
    ASTC_8X6_UNORM = 158,
    ASTC_8X6_UNORM_SRGB = 159,
    ASTC_8X8_TYPELESS = 161,
    ASTC_8X8_UNORM = 162,
    ASTC_8X8_UNORM_SRGB = 163,
    ASTC_10X5_TYPELESS = 165,
    ASTC_10X5_UNORM = 166,
    ASTC_10X5_UNORM_SRGB = 167,
    ASTC_10X6_TYPELESS = 169,
    ASTC_10X6_UNORM = 170,
    ASTC_10X6_UNORM_SRGB = 171,
    ASTC_10X8_TYPELESS = 173,
    ASTC_10X8_UNORM = 174,
    ASTC_10X8_UNORM_SRGB = 175,
    ASTC_10X10_TYPELESS = 177,
    ASTC_10X10_UNORM = 178,
    ASTC_10X10_UNORM_SRGB = 179,
    ASTC_12X10_TYPELESS = 181,
    ASTC_12X10_UNORM = 182,
    ASTC_12X10_UNORM_SRGB = 183,
    ASTC_12X12_TYPELESS = 185,
    ASTC_12X12_UNORM = 186,
    ASTC_12X12_UNORM_SRGB = 187
};

enum class k3DXResource {
    UNKNOWN = 0,
    BUFFER = 1,
    TEXTURE1D = 2,
    TEXTURE2D = 3,
    TEXTURE3D = 4
};

struct k3DDSHeader10 {
    k3DXFmt dx_format;
    k3DXResource dx_dim;
    uint32_t misc_flag;
    uint32_t array_size;
    uint32_t misc_flag2;
};

// Globals
k3image_file_handler_t k3DDSHandler = { k3dds_LoadHeaderInfo,
                                        k3dds_LoadData,
                                        k3dds_SaveData };

k3DDSHeader header;
k3DDSHeader10 header10;
k3fmt dds_format;

// Returns true if the mask has a contiguous set of bits set to 1
// if true, returns the start and end bit positions
bool FindMaskRange(const uint32_t mask, int32_t& start, int32_t& length)
{
    uint32_t cur_mask = mask;
    int32_t i;
    bool in_lit = false;

    // Use a switch for common mask values
    switch (mask) {
    case 0x000000ff: start = 0; length = 8; return true;
    case 0x0000ff00: start = 8; length = 8; return true;
    case 0x00ff0000: start = 16; length = 8; return true;
    case 0xff000000: start = 24; length = 8; return true;
    }

    start = 0;
    length = 0;

    for (i = 0; i <= 32; i++) {
        if (cur_mask & 0x1) {
            // if we see a 1, and we're not in the lit reagion,
            // then mark the start position, and set the lit bit
            if (!in_lit) {
                start = i;
                in_lit = true;
            }
        } else {
            // if we see a 0, and we are in the lit region,
            // then mark the length, and break out of the loop
            if (in_lit) {
                length = i - start;
                break;
            }
        }
        cur_mask = cur_mask >> 1;
    }

    // if the remainder of the mask still is all 0's
    // then we had received a coniguous mask
    return (cur_mask == 0);
}

void K3CALLBACK k3dds_LoadHeaderInfo(FILE* file_handle,
    uint32_t* width, uint32_t* height, uint32_t* depth, k3fmt* format)
{
    uint32_t start_pos = ftell(file_handle);
    fread(&header, sizeof(k3DDSHeader), 1, file_handle);

    //bool caps_exist          = (header.flags & DDSD_CAPS) ? true : false;
    bool height_exist = (header.flags & DDSD_HEIGHT) ? true : false;
    bool width_exist = (header.flags & DDSD_WIDTH) ? true : false;
    //bool pitch_exist         = (header.flags & DDSD_PITCH) ? true : false;
    //bool pixelformat_exist   = (header.flags & DDSD_PIXELFORMAT) ? true : false;
    //bool mipmap_count_exist  = (header.flags & DDSD_MIPMAPCOUNT) ? true : false;
    bool depth_exist = (header.flags & DDSD_DEPTH) ? true : false;

    // Check all the header info, to make sure it's a legal DDS file
    if (header.id != FOURCC_DDS || header.header_size != sizeof(k3DDSHeader) - 4 ||
        //!caps_exist || !pixelformat_exist ||
        !width_exist || !height_exist ||
        header.pixel_format.format_size != sizeof(k3dds_format)) {
        fseek(file_handle, start_pos, SEEK_SET);
        return;
    }

    bool rgb_exist = (header.pixel_format.flags & DDPF_RGB) ? true : false;
    bool alpha_exist = ( //(header.pixel_format.flags & DDPF_ALPHAPIXELS) &&  // NV dds photoshop plugin doens't set this flag on alpha8 format
        (header.pixel_format.abit_mask != 0)) ? true : false;
    bool fourcc_exist = (header.pixel_format.flags & DDPF_FOURCC) ? true : false;

    if (!depth_exist) header.depth = 1;

    *width = header.width;
    *height = header.height;
    *depth = header.depth;
    *format = k3fmt::UNKNOWN;

    if (fourcc_exist) {
        switch (header.pixel_format.fourcc) {
        case FOURCC_DXT1:    *format = k3fmt::BC1_UNORM; break;
        case FOURCC_DXT3:    *format = k3fmt::BC2_UNORM; break;
        case FOURCC_DXT5:    *format = k3fmt::BC3_UNORM; break;
        case FOURCC_ATI1:    *format = k3fmt::BC4_UNORM; break;
        case FOURCC_ATI2:    *format = k3fmt::BC5_UNORM; break;
        case FOURCC_R16F:    *format = k3fmt::R16_FLOAT; break;
        case FOURCC_RG16F:   *format = k3fmt::RG16_FLOAT; break;
        case FOURCC_RGBA16F: *format = k3fmt::RGBA16_FLOAT; break;
        case FOURCC_R32F:    *format = k3fmt::R32_FLOAT; break;
        case FOURCC_RG32F:   *format = k3fmt::RG32_FLOAT; break;
        case FOURCC_RGBA32F: *format = k3fmt::RGBA32_FLOAT; break;
        }
        if (header.pixel_format.fourcc == FOURCC_DX10) {
            fread(&header10, sizeof(k3DDSHeader10), 1, file_handle);
            switch (header10.dx_format) {
            case k3DXFmt::R32G32B32A32_FLOAT:   *format = k3fmt::RGBA32_FLOAT; break;
            case k3DXFmt::R32G32B32_FLOAT:      *format = k3fmt::RGB32_FLOAT; break;
            case k3DXFmt::R16G16B16A16_FLOAT:   *format = k3fmt::RGBA16_FLOAT; break;
            case k3DXFmt::R16G16B16A16_UNORM:   *format = k3fmt::RGBA16_UNORM; break;
            case k3DXFmt::R32G32_FLOAT:         *format = k3fmt::RG32_FLOAT; break;
            case k3DXFmt::D32_FLOAT_S8X24_UINT: *format = k3fmt::D32_FLOAT_S8X24_UINT; break;
            case k3DXFmt::R10G10B10A2_UNORM:    *format = k3fmt::RGB10A2_UNORM; break;
            case k3DXFmt::R8G8B8A8_UNORM:       *format = k3fmt::RGBA8_UNORM; break;
            case k3DXFmt::R8G8B8A8_UNORM_SRGB:  *format = k3fmt::RGBA8_UNORM; break;
            case k3DXFmt::R16G16_FLOAT:         *format = k3fmt::RG16_FLOAT; break;
            case k3DXFmt::R16G16_UNORM:         *format = k3fmt::RG16_UNORM; break;
            case k3DXFmt::D32_FLOAT:            *format = k3fmt::D32_FLOAT; break;
            case k3DXFmt::R32_FLOAT:            *format = k3fmt::R32_FLOAT; break;
            case k3DXFmt::R32_UINT:             *format = k3fmt::R32_UINT; break;
            case k3DXFmt::D24_UNORM_S8_UINT:    *format = k3fmt::D24_UNORM_S8_UINT; break;
            case k3DXFmt::R8G8_UNORM:           *format = k3fmt::RG8_UNORM; break;
            case k3DXFmt::R16_FLOAT:            *format = k3fmt::R16_FLOAT; break;
            case k3DXFmt::D16_UNORM:            *format = k3fmt::D16_UNORM; break;
            case k3DXFmt::R16_UNORM:            *format = k3fmt::R16_UNORM; break;
            case k3DXFmt::R16_UINT:             *format = k3fmt::R16_UINT; break;
            case k3DXFmt::R8_UNORM:             *format = k3fmt::R8_UNORM; break;
            case k3DXFmt::A8_UNORM:             *format = k3fmt::A8_UNORM; break;
            case k3DXFmt::BC1_UNORM:            *format = k3fmt::BC1_UNORM; break;
            case k3DXFmt::BC2_UNORM:            *format = k3fmt::BC2_UNORM; break;
            case k3DXFmt::BC3_UNORM:            *format = k3fmt::BC3_UNORM; break;
            case k3DXFmt::BC4_UNORM:            *format = k3fmt::BC4_UNORM; break;
            case k3DXFmt::BC5_UNORM:            *format = k3fmt::BC5_UNORM; break;
            case k3DXFmt::B5G6R5_UNORM:         *format = k3fmt::B5G6R5_UNORM; break;
            case k3DXFmt::B5G5R5A1_UNORM:       *format = k3fmt::BGR5A1_UNORM; break;
            case k3DXFmt::B8G8R8A8_UNORM:       *format = k3fmt::BGRA8_UNORM; break;
            case k3DXFmt::B8G8R8X8_UNORM:       *format = k3fmt::BGRX8_UNORM; break;
            case k3DXFmt::R10G10B10_XR_BIAS_A2_UNORM: *format = k3fmt::RGB10A2_UNORM; break;
            case k3DXFmt::BC6H_UF16:            *format = k3fmt::BC6_UNORM; break;
            case k3DXFmt::BC7_UNORM:            *format = k3fmt::BC7_UNORM; break;
            default:                            *format = k3fmt::UNKNOWN; break;
            }
            *depth = header10.array_size;
        }
    } else if (rgb_exist || alpha_exist) {

        bool red_exist = (rgb_exist && (header.pixel_format.rbit_mask != 0)) ? true : false;
        bool green_exist = (rgb_exist && (header.pixel_format.gbit_mask != 0)) ? true : false;
        bool blue_exist = (rgb_exist && (header.pixel_format.bbit_mask != 0)) ? true : false;

        // Find an appropriate format
        switch ((static_cast<int>(alpha_exist) << 3) |
            (static_cast<int>(blue_exist) << 2) |
            (static_cast<int>(green_exist) << 1) |
            (static_cast<int>(red_exist))) {
        case 0x1: // Red only
            if (header.pixel_format.rbit_mask == 0x000000ff)      *format = k3fmt::R8_UNORM;
            else if (header.pixel_format.rbit_mask == 0x0000ffff) *format = k3fmt::R16_UNORM;
            else if (header.pixel_format.rbit_mask == 0xffffffff) *format = k3fmt::R32_UNORM;
            break;
        case 0x8: // alpha only
            if (header.pixel_format.abit_mask == 0x000000ff) *format = k3fmt::A8_UNORM;
            break;
        case 0x3: // red and green
            if (header.pixel_format.rbit_mask == 0x000000ff &&
                header.pixel_format.gbit_mask == 0x0000ff00) *format = k3fmt::RG8_UNORM;
            else if (header.pixel_format.rbit_mask == 0x0000ffff &&
                header.pixel_format.gbit_mask == 0xffff0000) *format = k3fmt::RG16_UNORM;
            break;
        case 0x7: // red, green and blue
            if (header.pixel_format.rbit_mask == 0x000000ff &&
                header.pixel_format.gbit_mask == 0x0000ff00 &&
                header.pixel_format.bbit_mask == 0x00ff0000) *format = (header.pixel_format.rgb_bit_count == 24) ? k3fmt::RGB8_UNORM : k3fmt::RGBX8_UNORM;
            else if (header.pixel_format.rbit_mask == 0x00ff0000 &&
                header.pixel_format.gbit_mask == 0x0000ff00 &&
                header.pixel_format.bbit_mask == 0x000000ff) *format = (header.pixel_format.rgb_bit_count == 24) ? k3fmt::BGR8_UNORM : k3fmt::BGRX8_UNORM;
            else if (header.pixel_format.rbit_mask == 0x0000f800 &&
                header.pixel_format.gbit_mask == 0x000007e0 &&
                header.pixel_format.bbit_mask == 0x0000001f) *format = k3fmt::B5G6R5_UNORM;
            break;
        case 0xf: // red, green, blue, and alpha
            if (header.pixel_format.rbit_mask == 0x000000ff &&
                header.pixel_format.gbit_mask == 0x0000ff00 &&
                header.pixel_format.bbit_mask == 0x00ff0000 &&
                header.pixel_format.abit_mask == 0xff000000) *format = k3fmt::RGBA8_UNORM;
            else if (header.pixel_format.rbit_mask == 0x00ff0000 &&
                header.pixel_format.gbit_mask == 0x0000ff00 &&
                header.pixel_format.bbit_mask == 0x000000ff &&
                header.pixel_format.abit_mask == 0xff000000) *format = k3fmt::BGRA8_UNORM;
            else if (header.pixel_format.rbit_mask == 0xffc00000 &&
                header.pixel_format.gbit_mask == 0x003ff000 &&
                header.pixel_format.bbit_mask == 0x00000ffc &&
                header.pixel_format.abit_mask == 0x00000003) *format = k3fmt::RGB10A2_UNORM;
            else if (header.pixel_format.rbit_mask == 0x0000003e &&
                header.pixel_format.gbit_mask == 0x000007c0 &&
                header.pixel_format.bbit_mask == 0x0000f800 &&
                header.pixel_format.abit_mask == 0x00000001) *format = k3fmt::BGR5A1_UNORM;
            break;
        }

    }

    if (*format == k3fmt::UNKNOWN) {
        fseek(file_handle, start_pos, SEEK_SET);
        return;
    }

    dds_format = *format;
}

void K3CALLBACK k3dds_LoadData(FILE* file_handle, uint32_t pitch, uint32_t slice_pitch, void* data)
{
    uint8_t* bitmap = static_cast<uint8_t*>(data);
    uint8_t* bitmap_row;
    uint32_t format_size = k3imageObj::GetFormatSize(dds_format);
    uint32_t row_size = header.width * format_size;
    //uint32_t image_size = k3imageObj::GetImageSize(header.width, header.height, header.depth, dds_format);

    uint32_t slice, row;
    for (slice = 0; slice < header.depth; slice++) {
        bitmap_row = bitmap;
        for (row = 0; row < header.height; row++) {
            fread(bitmap_row, 1, row_size, file_handle);
            bitmap_row += pitch;
        }
        bitmap += slice_pitch;
    }
}

void K3CALLBACK k3dds_SaveData(FILE* file_handle,
    uint32_t width, uint32_t height, uint32_t depth, 
    uint32_t pitch, uint32_t slice_pitch, k3fmt format,
    const void* data)
{
    k3DDSHeader wheader = { 0 };
    k3fmt outputformat = k3fmt::UNKNOWN;
    const uint8_t* bitmap;
    uint32_t image_size;

    // Common DDS header fields
    wheader.id = FOURCC_DDS;
    wheader.header_size = sizeof(k3DDSHeader) - 4;
    wheader.flags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    wheader.height = height;
    wheader.width = width;
    if (depth > 1) {
        wheader.flags |= DDSD_DEPTH;
        wheader.depth = depth;
    }
    wheader.pixel_format.format_size = sizeof(k3dds_format);
    wheader.caps1 = DDSCAPS_TEXTURE;

    switch (format) {
    case k3fmt::RGBA8_UNORM:
    case k3fmt::RGBA16_UNORM:
    case k3fmt::RGBA32_UNORM:
        outputformat = k3fmt::RGBA8_UNORM;
        wheader.pixel_format.flags = DDPF_RGB | DDPF_ALPHAPIXELS;
        wheader.pixel_format.rbit_mask = 0x000000ff;
        wheader.pixel_format.gbit_mask = 0x0000ff00;
        wheader.pixel_format.bbit_mask = 0x00ff0000;
        wheader.pixel_format.abit_mask = 0xff000000;
        break;
    case k3fmt::BGRA8_UNORM:
        outputformat = k3fmt::BGRA8_UNORM;
        wheader.pixel_format.flags = DDPF_RGB | DDPF_ALPHAPIXELS;
        wheader.pixel_format.rbit_mask = 0x00ff0000;
        wheader.pixel_format.gbit_mask = 0x0000ff00;
        wheader.pixel_format.bbit_mask = 0x000000ff;
        wheader.pixel_format.abit_mask = 0xff000000;
        break;
    case k3fmt::RGBX8_UNORM:
        outputformat = k3fmt::RGBX8_UNORM;
        wheader.pixel_format.flags = DDPF_RGB;
        wheader.pixel_format.rbit_mask = 0x000000ff;
        wheader.pixel_format.gbit_mask = 0x0000ff00;
        wheader.pixel_format.bbit_mask = 0x00ff0000;
        break;
    case k3fmt::BGRX8_UNORM:
        outputformat = k3fmt::BGRX8_UNORM;
        wheader.pixel_format.flags = DDPF_RGB;
        wheader.pixel_format.rbit_mask = 0x00ff0000;
        wheader.pixel_format.gbit_mask = 0x0000ff00;
        wheader.pixel_format.bbit_mask = 0x000000ff;
        break;
    case k3fmt::RGB10A2_UNORM:
        outputformat = k3fmt::RGB10A2_UNORM;
        wheader.pixel_format.flags = DDPF_RGB | DDPF_ALPHAPIXELS;
        wheader.pixel_format.rbit_mask = 0xffc00000;
        wheader.pixel_format.gbit_mask = 0x003ff000;
        wheader.pixel_format.bbit_mask = 0x00000ffc;
        wheader.pixel_format.abit_mask = 0x00000003;
        break;
    case k3fmt::BGR5A1_UNORM:
        outputformat = k3fmt::BGR5A1_UNORM;
        wheader.pixel_format.flags = DDPF_RGB | DDPF_ALPHAPIXELS;
        wheader.pixel_format.rbit_mask = 0x0000003e;
        wheader.pixel_format.gbit_mask = 0x000007c0;
        wheader.pixel_format.bbit_mask = 0x0000f800;
        wheader.pixel_format.abit_mask = 0x00000001;
        break;

    case k3fmt::RGB8_UNORM:
        // I guess DX doesn't support this format; so use BGR8 instead
    //    outputformat = k3fmt::RGB8_UNORM;
    //    wheader.pixel_format.flags = DDPF_RGB;
    //    wheader.pixel_format.rbit_mask = 0x000000ff;
    //    wheader.pixel_format.gbit_mask = 0x0000ff00;
    //    wheader.pixel_format.bbit_mask = 0x00ff0000;
    //    break;
    case k3fmt::BGR8_UNORM:
        outputformat = k3fmt::BGR8_UNORM;
        wheader.pixel_format.flags = DDPF_RGB;
        wheader.pixel_format.rbit_mask = 0x00ff0000;
        wheader.pixel_format.gbit_mask = 0x0000ff00;
        wheader.pixel_format.bbit_mask = 0x000000ff;
        break;
    case k3fmt::B5G6R5_UNORM:
        outputformat = k3fmt::RGB8_UNORM;
        wheader.pixel_format.flags = DDPF_RGB;
        wheader.pixel_format.rbit_mask = 0x0000001f;
        wheader.pixel_format.gbit_mask = 0x000007e0;
        wheader.pixel_format.bbit_mask = 0x0000f800;
        break;

    case k3fmt::RG8_UNORM:
        outputformat = k3fmt::RG8_UNORM;
        wheader.pixel_format.flags = DDPF_RGB;
        wheader.pixel_format.rbit_mask = 0x000000ff;
        wheader.pixel_format.gbit_mask = 0x0000ff00;
        wheader.pixel_format.bbit_mask = 0x00000000;
        break;
    case k3fmt::RG16_UNORM:
    case k3fmt::RG32_UNORM:
        outputformat = k3fmt::RG16_UNORM;
        wheader.pixel_format.flags = DDPF_RGB;
        wheader.pixel_format.rbit_mask = 0x0000ffff;
        wheader.pixel_format.gbit_mask = 0xffff0000;
        wheader.pixel_format.bbit_mask = 0x00000000;
        break;

    case k3fmt::R8_UNORM:
        outputformat = k3fmt::R8_UNORM;
        wheader.pixel_format.flags = DDPF_RGB;
        wheader.pixel_format.rbit_mask = 0x000000ff;
        wheader.pixel_format.gbit_mask = 0x00000000;
        wheader.pixel_format.bbit_mask = 0x00000000;
        break;
    case k3fmt::A8_UNORM:
        outputformat = k3fmt::A8_UNORM;
        wheader.pixel_format.flags = DDPF_RGB | DDPF_ALPHAPIXELS;
        wheader.pixel_format.rbit_mask = 0x00000000;
        wheader.pixel_format.gbit_mask = 0x00000000;
        wheader.pixel_format.bbit_mask = 0x00000000;
        wheader.pixel_format.abit_mask = 0x000000ff;
        break;
    case k3fmt::R16_UNORM:
        outputformat = k3fmt::R16_UNORM;
        wheader.pixel_format.flags = DDPF_RGB;
        wheader.pixel_format.rbit_mask = 0x0000ffff;
        wheader.pixel_format.gbit_mask = 0x00000000;
        wheader.pixel_format.bbit_mask = 0x00000000;
        break;
    case k3fmt::R32_UNORM:
        outputformat = k3fmt::R32_UNORM;
        wheader.pixel_format.flags = DDPF_RGB;
        wheader.pixel_format.rbit_mask = 0xffffffff;
        wheader.pixel_format.gbit_mask = 0x00000000;
        wheader.pixel_format.bbit_mask = 0x00000000;
        break;

        // FOURCC formats
    case k3fmt::RGBA16_FLOAT:
        outputformat = k3fmt::RGBA16_FLOAT;
        wheader.pixel_format.flags = DDPF_FOURCC;
        wheader.pixel_format.fourcc = FOURCC_RGBA16F;
        break;
    case k3fmt::RGBA32_FLOAT:
    case k3fmt::RGB32_FLOAT:
        outputformat = k3fmt::RGBA32_FLOAT;
        wheader.pixel_format.flags = DDPF_FOURCC;
        wheader.pixel_format.fourcc = FOURCC_RGBA32F;
        break;
    case k3fmt::RG16_FLOAT:
        outputformat = k3fmt::RG16_FLOAT;
        wheader.pixel_format.flags = DDPF_FOURCC;
        wheader.pixel_format.fourcc = FOURCC_RG16F;
        break;
    case k3fmt::RG32_FLOAT:
        outputformat = k3fmt::RG32_FLOAT;
        wheader.pixel_format.flags = DDPF_FOURCC;
        wheader.pixel_format.fourcc = FOURCC_RG32F;
        break;
    case k3fmt::R16_FLOAT:
        outputformat = k3fmt::R16_FLOAT;
        wheader.pixel_format.flags = DDPF_FOURCC;
        wheader.pixel_format.fourcc = FOURCC_R16F;
        break;
    case k3fmt::R32_FLOAT:
        outputformat = k3fmt::R32_FLOAT;
        wheader.pixel_format.flags = DDPF_FOURCC;
        wheader.pixel_format.fourcc = FOURCC_R32F;
        break;

    case k3fmt::BC1_UNORM:
        outputformat = k3fmt::BC1_UNORM;
        wheader.pixel_format.flags = DDPF_FOURCC;
        wheader.pixel_format.fourcc = FOURCC_DXT1;
        break;
    case k3fmt::BC2_UNORM:
        outputformat = k3fmt::BC2_UNORM;
        wheader.pixel_format.flags = DDPF_FOURCC;
        wheader.pixel_format.fourcc = FOURCC_DXT3;
        break;
    case k3fmt::BC3_UNORM:
        outputformat = k3fmt::BC3_UNORM;
        wheader.pixel_format.flags = DDPF_FOURCC;
        wheader.pixel_format.fourcc = FOURCC_DXT5;
        break;
    case k3fmt::BC4_UNORM:
        outputformat = k3fmt::BC4_UNORM;
        wheader.pixel_format.flags = DDPF_FOURCC;
        wheader.pixel_format.fourcc = FOURCC_ATI1;
        break;
    case k3fmt::BC5_UNORM:
        outputformat = k3fmt::BC5_UNORM;
        wheader.pixel_format.flags = DDPF_FOURCC;
        wheader.pixel_format.fourcc = FOURCC_ATI2;
        break;
    default:
        break;
    }

    if (wheader.pixel_format.flags & DDPF_RGB) {
        uint32_t format_size = k3imageObj::GetFormatSize(outputformat);
        wheader.pixel_format.rgb_bit_count = 8 * format_size;
        wheader.flags |= DDSD_PITCH;
        wheader.pitch_or_linear_size = width * format_size;
    }

    if (outputformat == k3fmt::UNKNOWN) {
        k3error::Handler("Unsupported DDS format", "k3dds_SaveData");
        return;
    }

    uint32_t format_size = k3imageObj::GetFormatSize(outputformat);
    uint32_t dest_pitch = width * format_size;
    uint32_t dest_slice_pitch = height * dest_pitch;
    image_size = k3imageObj::GetImageSize(width, height, depth, outputformat);

    if (outputformat != format || pitch != dest_pitch || slice_pitch != dest_slice_pitch) {
        uint8_t* bm = new uint8_t[image_size];
        bitmap = bm;
        if (bitmap == NULL) {
            k3error::Handler("Out of memory", "k3dds_SaveData");
            return;
        }

        k3imageObj::ReformatBuffer(width, height, depth, pitch, slice_pitch, format, data,
            width, height, depth, dest_pitch, dest_slice_pitch, outputformat, static_cast<void*>(bm),
            NULL);
    } else {
        bitmap = static_cast<const uint8_t*>(data);
    }

    fwrite(&wheader, 1, sizeof(k3DDSHeader), file_handle);
    fwrite(bitmap, 1, image_size, file_handle);

    if (format != outputformat) delete[] bitmap;

}
