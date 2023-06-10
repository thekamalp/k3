// k3 graphics library
// functions to load and save JPG files

#include "k3internal.h"
#include "jpghandler.h"

extern "C" {
#include "jpeg-6b/jpeglib.h"
}

k3image_file_handler_t k3JPGHandler = { k3jpg_LoadHeaderInfo,
                                        k3jpg_LoadData,
                                        k3jpg_SaveData };

struct k3_error_mgr {
    struct jpeg_error_mgr pub;	// "public" fields
    jmp_buf setjmp_buffer;	    // for return to caller
};

typedef struct k3_error_mgr* k3_error_ptr;
METHODDEF(void)
jpeg_error_exit(j_common_ptr cinfo)
{
    // cinfo->err really points to a my_error_mgr struct, so coerce pointer
    k3_error_ptr err = (k3_error_ptr)cinfo->err;

    // Return control to the setjmp point
    longjmp(err->setjmp_buffer, 1);
}

struct jpeg_compress_struct cinfo;
struct jpeg_decompress_struct dinfo;
struct k3_error_mgr jerr;

void K3CALLBACK k3jpg_LoadHeaderInfo(FILE* file_handle,
    uint32_t* width, uint32_t* height, uint32_t* depth, k3fmt* format)
{
    // Mark the start position of file, in case this isn't a jpg, we must rewind
    uint32_t start_pos = ftell(file_handle);

    // We set up the normal JPEG error routines, then override error_exit.
    dinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = jpeg_error_exit;

    jpeg_create_decompress(&dinfo);

    // Establish the setjmp return context for my_error_exit to use.
    if (setjmp(jerr.setjmp_buffer)) {
        // If we get here, the JPEG code has signaled an error.
        // We need to clean up the JPEG object, close the input file, and return.
        jpeg_destroy_decompress(&dinfo);
        fseek(file_handle, start_pos, SEEK_SET);
        *width = 0;
        *height = 0;
        *depth = 0;
        *format = k3fmt::UNKNOWN;
        return;
    }

    jpeg_stdio_src(&dinfo, file_handle);
    jpeg_read_header(&dinfo, TRUE);
    jpeg_start_decompress(&dinfo);

    *width = dinfo.output_width;
    *height = dinfo.output_height;
    *depth = 1;
    switch (dinfo.output_components) {
    case 1: *format = k3fmt::R8_UNORM; break;
    case 2: *format = k3fmt::RG8_UNORM; break;
    case 3: *format = k3fmt::RGBA8_UNORM; break;
    case 4: *format = k3fmt::RGBA8_UNORM; break;
    default: *format = k3fmt::UNKNOWN; break;
    }


}

void K3CALLBACK k3jpg_LoadData(FILE* file_handle, uint32_t pitch, uint32_t slice_pitch, void* data)
{
    JSAMPLE* bitmap = static_cast<JSAMPLE*>(data);
    JSAMPLE** row_ptr;
    unsigned int i;

    if (bitmap) {
        // Create the row pointers
        row_ptr = new JSAMPLE * [dinfo.output_height];
        if (row_ptr == NULL) {
            k3error::Handler("Out of memory", "k3jpg_LoadData");
            return;
        }

        // Get the starting address of each row
        for (i = 0; i < dinfo.output_height; i++) {
            row_ptr[i] = &(bitmap[i * pitch]);
        }

        // Load the image to the bitmap
        while (dinfo.output_scanline < dinfo.output_height) {
            jpeg_read_scanlines(&dinfo, &(row_ptr[dinfo.output_scanline]),
                dinfo.output_height - dinfo.output_scanline);
        }

        if (dinfo.output_components == 3) {
            // 3 component RGB needs to get padded out to 4 components, with alpha set to 1
            int32_t col, row;
            for (row = 0; row < dinfo.output_height; row++) {
                JSAMPLE* cur_row = row_ptr[row];
                for (col = dinfo.output_width - 1; col >= 0; col--) {
                    cur_row[4 * col + 3] = 0xff;
                    cur_row[4 * col + 2] = cur_row[3 * col + 2];
                    cur_row[4 * col + 1] = cur_row[3 * col + 1];
                    cur_row[4 * col + 0] = cur_row[3 * col + 0];
                }
            }
        }

        jpeg_finish_decompress(&dinfo);
        delete[] row_ptr;
    }

    jpeg_destroy_decompress(&dinfo);
}

void K3CALLBACK k3jpg_SaveData(FILE* file_handle,
    uint32_t width, uint32_t height, uint32_t depth,
    uint32_t pitch, uint32_t slice_pitch, k3fmt format,
    const void* data)
{

}
