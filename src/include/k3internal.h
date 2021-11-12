// k3 graphics library
// internal header file
#pragma once

#ifdef _WIN32
#define K3DECLSPEC __declspec( dllexport )
#else
#define K3DECLSPEC 
#endif

#include "k3.h"

#include <ft2build.h>
#include FT_FREETYPE_H

const uint32_t K3_MATH_STATIC_ARRAY_SIZE = 16;

class k3imageImpl
{
public:
    uint32_t _width;
    uint32_t _height;
    uint32_t _depth;
    k3fmt _fmt;
    uint32_t _size;
    uint32_t _pitch_pad;
    uint32_t _slice_pitch_pad;
    void* _image_data;
};

class k3timerImpl
{
public:
    k3timerImpl();
    virtual ~k3timerImpl();
    uint32_t _start_time;
    uint32_t _start_delta_time;
    uint32_t _pause_time;
    bool _is_paused;
};

class k3fontImpl
{
public:
    static const uint32_t NUM_VERSIONS = 32;
    static const uint32_t NUM_CHARS = 128;
    static FT_Library _ft2_lib;

    k3fontImpl();
    virtual ~k3fontImpl();
    uint32_t _version;
    k3uploadBuffer _upload_cbuf[NUM_VERSIONS];
    k3surf _font_tex;
    k3buffer _font_cbuf;
    k3gfxState _font_state;
    float _ascender;
    float _descender;
    uint16_t _char_width[NUM_CHARS];
    uint16_t _char_height[NUM_CHARS];
    int16_t _char_offset_x[NUM_CHARS][NUM_CHARS];
    int16_t _char_offset_y[NUM_CHARS];
};

struct k3fontCBufferDynamic
{
    char text[64];
    float xform[64 * 4];
    float fg_color[4];
    float bg_color[4];
};

struct k3fontCBuffer {
    k3fontCBufferDynamic dyn;
    float char_scale[k3fontImpl::NUM_CHARS * 4];
};

class k3winImpl
{
public:
    k3winImpl();
    virtual ~k3winImpl();
    k3gfx gfx;
    uint32_t _x_pos;
    uint32_t _y_pos;
    uint32_t _width;
    uint32_t _height;
    bool _is_visible;
    bool _is_cursor_visible;
    bool _is_fullscreen;
    uint32_t _vsync_interval;
    const char* _title;
    k3fmt _color_fmt;
    void* _data;

    //k3surf _back_buffer;
    //k3surf _depth_buffer;
    //k3renderGroup _render_group;

    // Callbacks
    k3win_display_ptr Display;
    k3win_idle_ptr Idle;
    k3win_keyboard_ptr Keyboard;
    k3win_mouse_move_ptr MouseMove;
    k3win_mouse_button_ptr MouseButton;
    k3win_mouse_scroll_ptr MouseScroll;
    k3win_resize_ptr Resize;
    k3win_joystick_added_ptr JoystickAdded;
    k3win_joystick_removed_ptr JoystickRemoved;
    k3win_joystick_move_ptr JoystickMove;
    k3win_joystick_button_ptr JoystickButton;
    k3win_destroy_ptr Destroy;
};
