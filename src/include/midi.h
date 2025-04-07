// midi.h
// Midi related functions

#pragma once

#ifdef K3_BIG_ENDIAN
#define K3_MIDI_HEADER_CHUNK_ID     0x6468544D   // "MThd"
#define K3_MIDI_TRACK_CHUNK_ID      0x6B72544D   // "MTrk"
#else
#define K3_MIDI_HEADER_CHUNK_ID     0x4D546864   // "MThd"
#define K3_MIDI_TRACK_CHUNK_ID      0x4D54726B   // "MTrk"
#endif

#define K3_MIDI_EVENT_TYPE_SYSEX    0xF0
#define K3_MIDI_EVENT_TYPE_SYSEX2   0xF7
#define K3_MIDI_EVENT_TYPE_META     0xFF

#define K3_MIDI_META_EVENT_TYPE_CHAN_PREFIX     0x20
#define K3_MIDI_META_EVENT_TYPE_END_TRACK       0x2F
#define K3_MIDI_META_EVENT_TYPE_SET_TEMPO       0x51

#define K3_MIDI_MSG_NOTE_OFF                    0x80
#define K3_MIDI_MSG_NOTE_ON                     0x90
#define K3_MIDI_MSG_KEY_PRESSURE                0xA0
#define K3_MIDI_MSG_CONTROL_CHANGE              0xB0
#define K3_MIDI_MSG_PROGRAM_CHANGE              0xC0
#define K3_MIDI_MSG_CHANNEL_PRESSURE            0xD0
#define K3_MIDI_MSG_PITCH_WHEEL                 0xE0

#define K3_MIDI_CTRL_MAIN_VOLUME                0x07

#pragma pack(push, 1)

typedef struct {
    uint32_t chunk_type;
    uint32_t length;
    uint16_t format;
    uint16_t num_tracks;
    uint16_t division;
} k3_midi_header_t;

#pragma pack(pop)

typedef struct {
    int8_t note; // -1 if off
    int8_t midi_channel; // -1 if none assigned
    int8_t voice;  // -1 if none assigned
    int8_t reserved;
} k3_midi_opl_channel_t;

typedef struct {
    uint8_t data0;
    uint8_t data1;
    uint8_t command;
    uint8_t delta_time;
} k3_midi_event_field_t;

typedef union {
    k3_midi_event_field_t f;
    uint32_t data;
} k3_midi_event_t;

void k3_midi_fread_bigend(FILE* fh, void* data, uint8_t length);
int k3_midi_fread_var_bigend(FILE* fh, uint32_t* data);
uint32_t k3_midi_get_delta_time(const k3_midi_event_t** e);

