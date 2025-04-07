// dsp.h
// Digital sound processing header

#pragma once

#include "minimp3.h"
#include "midi.h"

#ifdef K3_BIG_ENDIAN

static const uint32_t K3_DSP_RIFF_CHUNK_ID         = 0x52494646;  // "RIFF" in big endian

// Wave Riff enums
static const uint32_t K3_DSP_WAV_FORMAT_ID         = 0x57415645;  // "WAVE" in big endian
static const uint32_t K3_DSP_WAV_SUBCHUNK_FMT_ID   = 0x666D7420;  // "fmt " in big endian
static const uint32_t K3_DSP_WAV_SUBCHUNK_DATA_ID  = 0x64617461;  // "data" in big endian

// SoundFont2 Riff enums
static const uint32_t K3_DSP_SF2_FORMAT_ID         = 0x7366626B;  // "sfbk" in big endian
static const uint32_t K3_DSP_SF2_SUBCHUNK_LIST_ID  = 0x4C495354;  // "LIST" in big endian
static const uint32_t K3_DSP_SF2_INFO_ID           = 0x494E464F;  // "INFO" in big endian

static const uint32_t K3_DSP_SF2_SAMPLE_DATA_ID    = 0x73647461;  // "sdta" in big endian
static const uint32_t K3_DSP_SF2_SAMPLE_ID         = 0x736D706C;  // "smpl" in big endian
static const uint32_t K3_DSP_SF2_SAMPLE24_ID       = 0x736D3234;  // "sm24" in big endian

static const uint32_t K3_DSP_SF2_PRESET_DATA_ID    = 0x70647461;  // "pdta" in big endian
static const uint32_t K3_DSP_SF2_PRESET_HEADER_ID  = 0x70686472;  // "phdr" in big endian
static const uint32_t K3_DSP_SF2_PRESET_BAG_ID     = 0x70626167;  // "pbag" in big endian
static const uint32_t K3_DSP_SF2_PRESET_MOD_ID     = 0x706D6F64;  // "pmod" in big endian
static const uint32_t K3_DSP_SF2_PRESET_GEN_ID     = 0x7067656E;  // "pgen" in big endian
static const uint32_t K3_DSP_SF2_INST_ID           = 0x696E7374;  // "inst" in big endian
static const uint32_t K3_DSP_SF2_INST_BAG_ID       = 0x69626167;  // "ibag" in big endian
static const uint32_t K3_DSP_SF2_INST_MOD_ID       = 0x696D6F64;  // "imod" in big endian
static const uint32_t K3_DSP_SF2_INST_GEN_ID       = 0x6967656E;  // "igen" in big endian
static const uint32_t K3_DSP_SF2_SAMPLE_HEADER_ID  = 0x73686472;  // "shdr" in big endian

static const uint32_t K3_DSP_ID3_TAG               = 0x49443300;  // "ID3" in little endian, LSB mst be masked off
static const uint32_t K3_DSP_ID3_TAG_MASK          = 0xFFFFFF00;

#else

static const uint32_t K3_DSP_RIFF_CHUNK_ID         = 0x46464952;  // "RIFF" in little endian

// Wave Riff enums
static const uint32_t K3_DSP_WAV_FORMAT_ID         = 0x45564157;  // "WAVE" in little endian
static const uint32_t K3_DSP_WAV_SUBCHUNK_FMT_ID   = 0x20746D66;  // "fmt " in little endian
static const uint32_t K3_DSP_WAV_SUBCHUNK_DATA_ID  = 0x61746164;  // "data" in little endian

// SoundFont2 Riff enums
static const uint32_t K3_DSP_SF2_FORMAT_ID         = 0x6B626673;  // "sfbk" in little endian
static const uint32_t K3_DSP_SF2_SUBCHUNK_LIST_ID  = 0x5453494C;  // "LIST" in little endian
static const uint32_t K3_DSP_SF2_INFO_ID           = 0x4F464E49;  // "INFO" in little endian

static const uint32_t K3_DSP_SF2_SAMPLE_DATA_ID    = 0x61746473;  // "sdta" in little endian
static const uint32_t K3_DSP_SF2_SAMPLE_ID         = 0x6C706D73;  // "smpl" in little endian
static const uint32_t K3_DSP_SF2_SAMPLE24_ID       = 0x34326D73;  // "sm24" in little endian

static const uint32_t K3_DSP_SF2_PRESET_DATA_ID    = 0x61746470;  // "pdta" in little endian
static const uint32_t K3_DSP_SF2_PRESET_HEADER_ID  = 0x72646870;  // "phdr" in little endian
static const uint32_t K3_DSP_SF2_PRESET_BAG_ID     = 0x67616270;  // "pbag" in little endian
static const uint32_t K3_DSP_SF2_PRESET_MOD_ID     = 0x646F6D70;  // "pmod" in little endian
static const uint32_t K3_DSP_SF2_PRESET_GEN_ID     = 0x6E656770;  // "pgen" in little endian
static const uint32_t K3_DSP_SF2_INST_ID           = 0x74736E69;  // "inst" in little endian
static const uint32_t K3_DSP_SF2_INST_BAG_ID       = 0x67616269;  // "ibag" in little endian
static const uint32_t K3_DSP_SF2_INST_MOD_ID       = 0x646F6D69;  // "imod" in little endian
static const uint32_t K3_DSP_SF2_INST_GEN_ID       = 0x6E656769;  // "igen" in little endian
static const uint32_t K3_DSP_SF2_SAMPLE_HEADER_ID  = 0x72646873;  // "shdr" in little endian

static const uint32_t K3_DSP_ID3_TAG               = 0x00334449;  // "ID3" in little endian, MSB mst be masked off
static const uint32_t K3_DSP_ID3_TAG_MASK          = 0x00FFFFFF;

#endif

// sound font generators

static const uint16_t K3_DSP_SF2_GEN_START_ADDR_OFFSET = 00;
static const uint16_t K3_DSP_SF2_GEN_END_ADDR_OFFSET   = 01;
static const uint16_t K3_DSP_SF2_GEN_START_LOOP_OFFSET = 02;
static const uint16_t K3_DSP_SF2_GEN_END_LOOP_OFFSET   = 03;
static const uint16_t K3_DSP_SF2_GEN_START_ADDR_COARSE = 04;
static const uint16_t K3_DSP_SF2_GEN_END_ADDR_COARSE   = 12;
static const uint16_t K3_DSP_SF2_GEN_DELAY_VOL_ENV     = 33;
static const uint16_t K3_DSP_SF2_GEN_ATTACK_VOL_ENV    = 34;
static const uint16_t K3_DSP_SF2_GEN_HOLD_VOL_ENV      = 35;
static const uint16_t K3_DSP_SF2_GEN_DECAY_VOL_ENV     = 36;
static const uint16_t K3_DSP_SF2_GEN_SUSTAIN_VOL_ENV   = 37;
static const uint16_t K3_DSP_SF2_GEN_RELEASE_VOL_ENV   = 38;
static const uint16_t K3_DSP_SF2_GEN_INSTRUMENT        = 41;
static const uint16_t K3_DSP_SF2_GEN_KEY_RANGE         = 43;
static const uint16_t K3_DSP_SF2_GEN_VEL_RANGE         = 44;
static const uint16_t K3_DSP_SF2_GEN_START_LOOP_COARSE = 45;
static const uint16_t K3_DSP_SF2_GEN_KEY_NUM           = 46;
static const uint16_t K3_DSP_SF2_GEN_END_LOOP_COARSE   = 50;
static const uint16_t K3_DSP_SF2_GEN_SAMPLE_ID         = 53;
static const uint16_t K3_DSP_SF2_GEN_SAMPLE_MODE       = 54;
static const uint16_t K3_DSP_SF2_GEN_OVERRIDE_PITCH    = 58;

#pragma pack(push, 1)

struct k3_dsp_riff_header_t {
    uint32_t chunk_id;
    uint32_t chunk_size;
    uint32_t format_id;
};

struct k3_dsp_riff_subchunk_header_t {
    uint32_t id;
    uint32_t size;
};

struct k3_dsp_wav_format_t {
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
};

struct k3_dsp_sf2_phdr_t {
    char preset_name[20];
    uint16_t preset;
    uint16_t bank;
    uint16_t preset_bag_ndx;
    uint32_t library;
    uint32_t genre;
    uint32_t morphology;
};

struct k3_dsp_sf2_bag_t {
    uint16_t gen_ndx;
    uint16_t mod_ndx;
};

struct k3_dsp_sf2_mod_list_t {
    uint16_t mod_src_op;
    uint16_t mod_dst_op;
    uint16_t mod_amount;
    uint16_t mod_amt_src_op;
    uint16_t mod_trans_op;
};

struct k3_dsp_sf2_range_t {
    uint8_t lo;
    uint8_t hi;
};

union k3_dsp_sf2_gen_amount_t {
    k3_dsp_sf2_range_t range;
    int16_t s16;
    uint16_t u16;
};

struct k3_dsp_sf2_gen_list_t {
    uint16_t gen_op;
    k3_dsp_sf2_gen_amount_t gen_amount;
};

struct k3_dsp_sf2_inst_t {
    char inst_name[20];
    uint16_t inst_bag_ndx;
};

struct k3_dsp_sf2_sample_t {
    char sample_name[20];
    uint32_t start;
    uint32_t end;
    uint32_t start_loop;
    uint32_t end_loop;
    uint32_t sample_rate;
    uint8_t orig_pitch;
    int8_t pitch_correction;
    uint16_t sample_link;
    uint16_t sample_type;
};

#pragma pack(pop)

struct k3_dsp_pcm_properties_t {
    uint16_t bytes_per_sample;
    uint16_t num_channels;
    uint32_t samples_per_second;
};

struct k3_dsp_sf2_preset_t {
    uint8_t key_lo;
    uint8_t key_hi;
    uint8_t vel_lo;
    uint8_t vel_hi;
    uint8_t key_pitch;
    int8_t pitch_correction;
    uint8_t sample_mode;
    uint8_t reserved0;
    uint32_t start_sample;
    uint32_t end_sample;
    uint32_t start_loop_sample;
    uint32_t end_loop_sample;
    uint32_t sample_rate;
    uint32_t delay_samples;
    uint32_t attack_samples;
    uint32_t hold_samples;
    uint32_t decay_samples;
    uint32_t sustain_level;
    uint32_t release_samples;
    uint32_t reserved1;
};

class k3soundFontImpl
{
public:
    k3soundFontImpl();
    virtual ~k3soundFontImpl();
    k3sampleData sample;
    k3llist<k3_dsp_sf2_preset_t>::list_t presets[129];  // index 128 used for percussion
};

static const uint32_t K3_DSP_VOL_ENV_OFF        = 0;
static const uint32_t K3_DSP_VOL_ENV_DELAY      = 1;
static const uint32_t K3_DSP_VOL_ENV_ATTACK     = 2;
static const uint32_t K3_DSP_VOL_ENV_HOLD       = 3;
static const uint32_t K3_DSP_VOL_ENV_DECAY      = 4;
static const uint32_t K3_DSP_VOL_ENV_SUSTAIN    = 5;
static const uint32_t K3_DSP_VOL_ENV_RELEASE    = 6;

struct k3_dsp_vox_t {
    uint32_t cur_vol_env;
    uint32_t cur_sample_vol_env;
    uint32_t cur_index;
    uint32_t cur_fract;  // 16 lsb for fraction of index
    uint32_t sample_inc;  // fixed point, 16.16 increment
    uint32_t volume;  // full volume at 0x10000, set to 0 to turn off voice
    uint32_t max_volume;            // fixed point, 16.16
    uint32_t sustain_volume;        // fixed point, 16.16
    uint32_t attack_volume_inc;     // fixed point, 16.16
    uint32_t decay_volume_dec;      // fixed point, 16.16
    uint32_t release_volume_dec;    // fixed point, 16.16
    k3llist<k3_dsp_sf2_preset_t>::node_t base_preset;
    k3llist<k3_dsp_sf2_preset_t>::node_t preset;
};

enum class k3dspWavState {
    HEADER        = 0,
    SUBCHUNK_HDR  = 1,
    SUBCHUNK_PAD  = 2,
    FORMAT        = 3,
    DATA          = 4,
    DONE          = 5,
    ERROR         = 0xFF
};

static const uint32_t K3_DSP_WAV_FLAG_BLEND_OUTPUT = 0;

struct k3_dsp_wav_t {
    k3dspWavState state;
    // number of bytes read in the given state
    uint32_t bytes_read;
    uint32_t num_output_channels;
    uint32_t flags;
    uint32_t samples_output;
    uint32_t total_samples;
    k3_dsp_riff_header_t hdr;
    k3_dsp_riff_subchunk_header_t subchunk_hdr;
    k3_dsp_wav_format_t fmt;
};

void k3dspWavReset(k3_dsp_wav_t* wav);
void k3dspWavSetFlag(k3_dsp_wav_t* wav, uint32_t flag, uint32_t value);
void k3dspWavOutputChannels(k3_dsp_wav_t* wav, uint32_t num_output_channels);
k3dspWavState k3dspWavGetState(const k3_dsp_wav_t* wav);
k3dspWavState k3dspWaveProcess(k3_dsp_wav_t* wav, const uint8_t* in_stream, uint32_t* in_size, int16_t* out_stream, uint32_t* out_size);

enum class k3dspMidiState {
    HEADER      = 0,
    EVENTS      = 1,
    ERROR       = 2
};

static const uint32_t K3_DSP_MIDI_FLAG_BLEND_OUTPUT = 0;
static const uint32_t K3_DSP_NUM_VOICES = 20;
static const uint32_t K3_DSP_NUM_MELODIC_VOICES = 15;
static const uint32_t K3_DSP_NUM_PERCUSSIVE_VOICES = K3_DSP_NUM_VOICES - K3_DSP_NUM_MELODIC_VOICES;
static const uint32_t K3_DSP_DEFAULT_SAMPLES_PER_MIDI_TICK = 240;

static const uint32_t k3_dsp_freq_table[300] = {
   0x1000000, 0xFF68C1, 0xFED1DB, 0xFE3B4F, 0xFDA51B, 0xFD0F41, 0xFC79BF, 0xFBE495, 0xFB4FC3, 0xFABB49, 0xFA2727, 0xF9935D, 0xF8FFEA, 0xF86CCE, 0xF7DA08, 0xF7479A, 0xF6B582, 0xF623C0, 0xF59255, 0xF5013F, 0xF4707F, 0xF3E015, 0xF35000, 0xF2C040, 0xF230D5,
    0xF1A1BF, 0xF112FD, 0xF08490, 0xEFF676, 0xEF68B1, 0xEEDB40, 0xEE4E22, 0xEDC157, 0xED34E0, 0xECA8BB, 0xEC1CEA, 0xEB916A, 0xEB063E, 0xEA7B63, 0xE9F0DB, 0xE966A5, 0xE8DCC0, 0xE8532C, 0xE7C9EA, 0xE740F9, 0xE6B859, 0xE6300A, 0xE5A80B, 0xE5205C, 0xE498FE,
    0xE411F0, 0xE38B31, 0xE304C2, 0xE27EA3, 0xE1F8D2, 0xE17351, 0xE0EE1F, 0xE0693B, 0xDFE4A6, 0xDF605F, 0xDEDC66, 0xDE58BC, 0xDDD55F, 0xDD524F, 0xDCCF8D, 0xDC4D19, 0xDBCAF1, 0xDB4917, 0xDAC789, 0xDA4647, 0xD9C552, 0xD944A9, 0xD8C44D, 0xD8443B, 0xD7C476,
    0xD744FC, 0xD6C5CE, 0xD646EA, 0xD5C852, 0xD54A04, 0xD4CC01, 0xD44E49, 0xD3D0DA, 0xD353B6, 0xD2D6DC, 0xD25A4B, 0xD1DE04, 0xD16207, 0xD0E653, 0xD06AE8, 0xCFEFC5, 0xCF74EC, 0xCEFA5B, 0xCE8013, 0xCE0612, 0xCD8C5A, 0xCD12EA, 0xCC99C1, 0xCC20E0, 0xCBA847,
    0xCB2FF5, 0xCAB7E9, 0xCA4025, 0xC9C8A8, 0xC95171, 0xC8DA80, 0xC863D6, 0xC7ED72, 0xC77754, 0xC7017C, 0xC68BE9, 0xC6169C, 0xC5A194, 0xC52CD1, 0xC4B853, 0xC4441A, 0xC3D026, 0xC35C76, 0xC2E90A, 0xC275E3, 0xC20300, 0xC19060, 0xC11E05, 0xC0ABEC, 0xC03A18,
    0xBFC886, 0xBF5738, 0xBEE62C, 0xBE7564, 0xBE04DE, 0xBD949A, 0xBD2499, 0xBCB4DA, 0xBC455D, 0xBBD622, 0xBB6728, 0xBAF871, 0xBA89FA, 0xBA1BC5, 0xB9ADD1, 0xB9401E, 0xB8D2AC, 0xB8657A, 0xB7F889, 0xB78BD8, 0xB71F67, 0xB6B337, 0xB64746, 0xB5DB96, 0xB57024,
    0xB504F3, 0xB49A00, 0xB42F4D, 0xB3C4D9, 0xB35AA4, 0xB2F0AD, 0xB286F5, 0xB21D7C, 0xB1B441, 0xB14B44, 0xB0E285, 0xB07A04, 0xB011C1, 0xAFA9BB, 0xAF41F3, 0xAEDA68, 0xAE731A, 0xAE0C09, 0xADA535, 0xAD3E9E, 0xACD844, 0xAC7226, 0xAC0C44, 0xABA69F, 0xAB4135,
    0xAADC08, 0xAA7716, 0xAA1260, 0xA9ADE5, 0xA949A6, 0xA8E5A2, 0xA881D9, 0xA81E4B, 0xA7BAF8, 0xA757E0, 0xA6F502, 0xA6925E, 0xA62FF5, 0xA5CDC6, 0xA56BD1, 0xA50A16, 0xA4A894, 0xA4474C, 0xA3E63E, 0xA38569, 0xA324CD, 0xA2C46A, 0xA26440, 0xA2044F, 0xA1A497,
    0xA14517, 0xA0E5D0, 0xA086C1, 0xA027EA, 0x9FC94B, 0x9F6AE4, 0x9F0CB5, 0x9EAEBD, 0x9E50FD, 0x9DF375, 0x9D9623, 0x9D3909, 0x9CDC26, 0x9C7F7A, 0x9C2304, 0x9BC6C5, 0x9B6ABC, 0x9B0EEA, 0x9AB34E, 0x9A57E9, 0x99FCB9, 0x99A1BF, 0x9946FB, 0x98EC6C, 0x989213,
    0x9837F0, 0x97DE01, 0x978448, 0x972AC4, 0x96D175, 0x96785A, 0x961F74, 0x95C6C3, 0x956E46, 0x9515FD, 0x94BDE8, 0x946608, 0x940E5B, 0x93B6E2, 0x935F9D, 0x93088C, 0x92B1AE, 0x925B03, 0x92048B, 0x91AE47, 0x915835, 0x910256, 0x90ACAA, 0x905731, 0x9001EA,
    0x8FACD6, 0x8F57F3, 0x8F0343, 0x8EAEC5, 0x8E5A79, 0x8E065F, 0x8DB276, 0x8D5EBF, 0x8D0B3A, 0x8CB7E5, 0x8C64C2, 0x8C11D0, 0x8BBF0F, 0x8B6C7F, 0x8B1A20, 0x8AC7F2, 0x8A75F4, 0x8A2426, 0x89D289, 0x89811C, 0x892FDF, 0x88DED2, 0x888DF5, 0x883D48, 0x87ECCA,
    0x879C7C, 0x874C5D, 0x86FC6E, 0x86ACAE, 0x865D1D, 0x860DBB, 0x85BE88, 0x856F84, 0x8520AF, 0x84D208, 0x84838F, 0x843545, 0x83E729, 0x83993B, 0x834B7C, 0x82FDEA, 0x82B086, 0x826350, 0x821647, 0x81C96C, 0x817CBE, 0x81303E, 0x80E3EB, 0x8097C5, 0x804BCC
};

struct k3_dsp_midi_t {
    k3dspMidiState state;
    uint32_t num_output_channels;
    uint32_t flags;
    k3soundFont sf2;
    uint32_t midi_delta_tick;
    uint32_t cur_sample_in_midi_tick;
    uint32_t samples_per_midi_tick;
    k3_dsp_vox_t vox[K3_DSP_NUM_VOICES];
    uint8_t percussive_note_mapping[K3_DSP_NUM_PERCUSSIVE_VOICES];
};

void k3dspMidiReset(k3_dsp_midi_t* midi);
void k3dspMidiSetFlag(k3_dsp_midi_t* midi, uint32_t flag, uint32_t value);
void k3dspMidiOutputChannels(k3_dsp_midi_t* midi, uint32_t num_output_channels);
void k3dspMidiSetSoundFont(k3_dsp_midi_t* midi, k3soundFont sf2);
k3dspMidiState k3dspMidiGetState(k3_dsp_midi_t* midi);
k3dspMidiState k3dspMidiProcess(k3_dsp_midi_t* midi, const uint8_t* in_stream, uint32_t* in_size, int16_t* out_stream, uint32_t* out_size);
