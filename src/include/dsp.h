// dsp.h
// Digital sound processing header

#pragma once

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

struct k3_dsp_sf2_t {
    k3sampleData sample;
    k3_dsp_sf2_preset_t presets[129];  // index 128 used for percussion
};

static const uint32_t K3_DSP_VOL_ENV_OFF        = 0;
static const uint32_t K3_DSP_VOL_ENV_DELAY      = 1;
static const uint32_t K3_DSP_VOL_ENV_ATTACK     = 2;
static const uint32_t K3_DSP_VOL_ENV_HOLD       = 3;
static const uint32_t K3_DSP_VOL_ENV_DECAY      = 4;
static const uint32_t K3_DSP_VOL_ENV_SUSTAIN    = 5;
static const uint32_t K3_DSP_VOL_ENV_RELEASE    = 6;

struct ise_dsp_vox_t {
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
    k3llist<k3_dsp_sf2_preset_t>::list_t preset;
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
k3dspWavState k3dspWaveProcess(k3_dsp_wav_t* wav, const uint8_t* in_stream, uint32_t* in_size, int32_t* out_stream, uint32_t* out_size);
