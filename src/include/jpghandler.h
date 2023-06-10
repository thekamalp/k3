// k3 graphics library
// functions to load and save jpg image files
#pragma once

extern k3image_file_handler_t k3JPGHandler;

void K3CALLBACK k3jpg_LoadHeaderInfo(FILE* file_handle,
    uint32_t* width, uint32_t* height, uint32_t* depth, k3fmt* format);

void K3CALLBACK k3jpg_LoadData(FILE* file_handle, uint32_t pitch, uint32_t slice_pitch, void* data);

void K3CALLBACK k3jpg_SaveData(FILE* file_handle,
    uint32_t width, uint32_t height, uint32_t depth,
    uint32_t pitch, uint32_t slice_pitch, k3fmt format,
    const void* data);
