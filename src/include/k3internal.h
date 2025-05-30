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

#include "fbx.h"
#include "dsp.h"
#include "../flac/foxen-flac.h"

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

class k3bitTrackerImpl
{
public:
    k3bitTrackerImpl();
    virtual ~k3bitTrackerImpl();
    uint32_t _size;
    uint64_t* _array;
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

class k3sampleDataImpl
{
public:
    k3sampleDataImpl();
    virtual ~k3sampleDataImpl();
    uint32_t _total_size;
    void* _sample_data;
};

inline uint32_t k3_endian_swap32(uint32_t in)
{
#ifdef K3_BIG_ENDIAN
    return in;
#else
    return ((in & 0xff) << 24) | ((in & 0xff00) << 8) | ((in & 0xff0000) >> 8) | ((in & 0xff000000) >> 24);
#endif
}

inline uint16_t k3_endian_swap16(uint16_t in)
{
#ifdef K3_BIG_ENDIAN
    return in;
#else
    return ((in & 0xff) << 8) | ((in & 0xff00) >> 8);
#endif
}


#ifdef K3_BIG_ENDIAN
static const uint32_t K3_STREAM_KEY_FLAC = 0x664c6143;  // "fLaC"
#else
static const uint32_t K3_STREAM_KEY_FLAC = 0x43614c66;  // "fLaC"
#endif

enum class k3streamType {
    NONE,
    WAV,
    FLAC,
    MP3,
    MIDI
};

struct k3streamDecoder
{
    k3_dsp_wav_t wav;
    fx_flac_t* flac;
    k3_dsp_midi_t midi;
    mp3dec_t mp3;
    k3streamType stype;
    k3sampleData sample;
    uint32_t sample_offset;
    uint32_t data_left;
};

class k3soundBufImpl
{
public:
    k3soundBufImpl();
    virtual ~k3soundBufImpl();

    uint32_t _write_pos;
    uint32_t _buf_size;
    bool _is_playing;

    uint32_t _num_streams;
    k3streamDecoder* _stream;
    uint32_t _bits_per_sample;
    uint32_t _num_channels;
    k3soundFont sf2;
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

struct k3meshModel {
    static const uint32_t FLAG_DYNAMIC = 0x1;

    float world_xform[16];
    char name[K3_FBX_MAX_NAME_LENGTH];
    uint32_t parent;
    uint32_t mesh_index;
    uint32_t prim_start;
    uint32_t num_prims;
    float position[3];
    uint32_t padding0;
    float rotation[3];
    uint32_t padding1;
    float scaling[3];
    uint32_t padding2;
    float diffuse_color[3];
    float emissive_factor;
    float visibility;
    uint32_t diffuse_map_index;
    uint32_t normal_map_index;
    uint32_t flags;
};

struct k3camera {
    char name[K3_FBX_MAX_NAME_LENGTH];
    float translation[3];
    float look_at[3];
    float up[3];
    k3projType proj_type;
    uint32_t res_x;
    uint32_t res_y;
    float fovy;
    float ortho_scale;
    float near_plane;
    float far_plane;
};

struct k3light {
    static const uint32_t FLAG_DYNAMIC = 0x1;
    static const uint32_t FLAG_CAST_SHADOWS = 0x2;

    char name[K3_FBX_MAX_NAME_LENGTH];
    float position[4];
    float color[3];
    float intensity;
    float rot_quat[4];
    uint32_t parent;
    uint32_t light_type;
    uint32_t decay_type;
    uint32_t flags;
    float decay_start;
    float base_hsv[3];
    float base_intensity;
    uint32_t padding[3];
};

struct k3emptyModel {
    char name[K3_FBX_MAX_NAME_LENGTH];
    float world_xform[16];
    float visibility;
};

struct k3bone {
    char name[K3_FBX_MAX_NAME_LENGTH];
    float rot_quat[4];  // quaternion representation of bone rotation
    float scaling[3];   // bone scaling
    uint32_t parent;    // parent bone id, or ~0x0 if root
    float position[3];  // bone location
    uint32_t dummy;
    float inv_bind_pose[16];  // inverse of the global bind pose
};

struct k3boneData {
    float rot_quat[4];  // quaternion representation of bone rotation
    float scaling[3];     // bone scaling
    uint32_t padding0;
    float position[3];  // bone location
    uint32_t padding1;
};

static const uint32_t K3_BONE_FLAG_NONE = 0x0;
static const uint32_t K3_BONE_FLAG_MORPH = 0x1;

enum class k3fbxObjType {
    NONE,
    MESH,
    LIGHT,
    CAMERA,
    LIMB_NODE
};

struct k3animObj {
    k3fbxObjType obj_type;
    uint32_t id;
};

struct k3anim {
    char name[K3_FBX_MAX_NAME_LENGTH];
    uint32_t num_keyframes;
    uint32_t keyframe_delta_msec;
    k3boneData* bone_data;  // array of num_bones * num_keyframes; for model animations, just num_keyframes
    uint32_t* bone_flag;  // per bone attributes
    k3animObj model;
    uint32_t num_anim_objs;
    k3animObj* anim_objs;
};

class k3meshImpl
{
public:
    k3meshImpl();
    virtual ~k3meshImpl();
    uint32_t _num_meshes;
    uint32_t _num_models;
    uint32_t _num_model_custom_props;
    uint32_t _num_custom_prop_strings;
    uint32_t _num_tris;
    uint32_t _num_verts;
    uint32_t _num_textures;
    uint32_t _num_cameras;
    uint32_t _num_lights;
    uint32_t _num_empties;
    uint32_t _num_bones;
    uint32_t _num_anims;
    uint32_t _num_anim_custom_props;
    uint32_t _num_static_models;
    uint32_t _num_static_lights;
    float* _geom_data;
    uint32_t* _mesh_start;
    k3meshModel* _model;
    k3flint32* _model_custom_props;
    k3flint32* _empty_custom_props;
    k3flint32* _anim_custom_props;
    char** _custom_prop_strings;
    k3surf* _textures;
    k3camera* _cameras;
    k3light* _lights;
    k3emptyModel* _empties;
    k3bone* _bones;
    k3anim* _anim;
    k3buffer _ib;
    k3buffer _vb;  // vertex buffer; cotains only positions
    k3buffer _ab;  // attribute buffer; contains normals, tangents and uv
    k3buffer _sb;  // skin buffer; contains bone ids and weights (4 per vertex)
    k3buffer _lb;  // light buffer; contains all light attributes
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
