#include "k3internal.h"

// Reads a value of length size in big-endian format froma  file
void k3_midi_fread_bigend(FILE* fh, void* data, uint8_t length)
{
    int i;
    uint8_t* d = (uint8_t*)data;
#ifdef K3_BIG_ENDIAN
    for (i = 0; i < length; i++) {
#else
    for (i = length - 1; i >= 0; i--) {
#endif
        fread(d + i, 1, 1, fh);
    }
}

// Reads a midi-style variable length value
// returns number of bytes read
int k3_midi_fread_var_bigend(FILE* fh, uint32_t* data)
{
    int i;
    bool done = false;
    uint8_t new_data;
    *data = 0;
    for (i = 0; i < 4 && !done; i++) {
        fread(&new_data, 1, 1, fh);
#ifdef K3_BIG_ENDIAN
        *data |= (new_data & 0x7f) << (7 * i);
#else
        *data <<= 7;
        *data |= new_data & 0x7f;
#endif
        if ((new_data & 0x80) == 0) done = true;
    }
    return i;
}

// parses midi track poionted to by file, and stores in midi event array, if non-NULL
// returns the number of events in the current track
// File should be open with the position of the track to be parsed
int k3_midi_parse_track(FILE* fh, k3_midi_header_t* header, k3_midi_event_t* track, int* base_opl_channel)
{
    int num_events = 0;
    int cur_file_pos;
    k3_midi_event_t* cur_event = track;

    uint32_t chunk_type;
    uint32_t chunk_length;
    uint32_t running_length = 0;
    uint32_t next_delta_time, delta_time = 0;
    int time = 0;
    uint8_t i, c0, c1, event_type = 0, meta_event_type, midi_channel, midi_event, ch;
    uint32_t length;
    int num_opl_channel_used = 0;
    uint8_t midi_channel_note[K3_MIDI_NUM_CHANNELS] = { 0 };
    uint8_t midi_channel_mapped[K3_MIDI_NUM_CHANNELS] = { 0 };
    uint8_t midi_channel_voice[K3_MIDI_NUM_CHANNELS] = { 0 };
    uint8_t midi_channel_volume[K3_MIDI_NUM_CHANNELS];
    memset(midi_channel_volume, 0x7f, K3_MIDI_NUM_CHANNELS);
    k3_midi_event_t* op_channel_last_note_off_event[K3_MIDI_NUM_CHANNELS] = { NULL }; // indicate last event where a note was turned off
    int op_channel_last_note_off_time[K3_MIDI_NUM_CHANNELS] = { 0 };  // indicates last time a note was turned off
    int op_channel_reset_threshold_time = 16 * header->division;

    k3_midi_fread_bigend(fh, &chunk_type, sizeof(uint32_t));
    k3_midi_fread_bigend(fh, &chunk_length, sizeof(uint32_t));
    if (chunk_type == K3_MIDI_TRACK_CHUNK_ID) {
        // We have a valid midi track chunk
        while (running_length < chunk_length) {
            running_length += k3_midi_fread_var_bigend(fh, &next_delta_time);
            delta_time += next_delta_time;
            if (delta_time >= 0xFF) {
                if (cur_event) {
                    cur_event->data = 0xFF000000 | delta_time;
                    cur_event++;
                }
                num_events++;
                delta_time = 0;
            }
            time += next_delta_time;
            cur_file_pos = ftell(fh);
            running_length += fread(&c0, 1, 1, fh);
            if (c0 & 0x80) {
                event_type = c0;
                if (event_type != K3_MIDI_EVENT_TYPE_SYSEX && event_type != K3_MIDI_EVENT_TYPE_SYSEX2 && event_type != K3_MIDI_EVENT_TYPE_META) {
                    running_length += fread(&c0, 1, 1, fh);
                }
            }
            //printf("fpos 0x%x (d %d, t %d): ", cur_file_pos, next_delta_time, time);
            switch (event_type) {
            case K3_MIDI_EVENT_TYPE_SYSEX:
            case K3_MIDI_EVENT_TYPE_SYSEX2:
                // first get length, then skip over sysex events
                running_length += k3_midi_fread_var_bigend(fh, &length);
                fseek(fh, length, SEEK_CUR);
                running_length += length;
                //printf("sysex event, length = %d\n", length);
                break;
            case K3_MIDI_EVENT_TYPE_META:
                running_length += fread(&meta_event_type, 1, 1, fh);
                running_length += k3_midi_fread_var_bigend(fh, &length);
                switch (meta_event_type) {
                case K3_MIDI_META_EVENT_TYPE_CHAN_PREFIX:
                case K3_MIDI_META_EVENT_TYPE_END_TRACK:
                case K3_MIDI_META_EVENT_TYPE_SET_TEMPO:
                    if (cur_event) {
                        cur_event->data = 0;
                        cur_event->f.delta_time = (uint8_t)delta_time;
                        cur_event->f.command = meta_event_type;
                        if (length == 1) {
                            running_length += fread(&cur_event->f.data0, 1, 1, fh);
                        }
                        if (meta_event_type == K3_MIDI_META_EVENT_TYPE_END_TRACK) {
                            for (i = 0; i < K3_MIDI_NUM_CHANNELS; i++) {
                                if (op_channel_last_note_off_event[i]) {
                                    op_channel_last_note_off_event[i]->f.data1 = 0xFF;
                                }
                            }
                        }
                        if (meta_event_type == K3_MIDI_META_EVENT_TYPE_SET_TEMPO) {
                            uint32_t usec_per_qtr_note = 0;
                            k3_midi_fread_bigend(fh, &usec_per_qtr_note, 3);
                            running_length += 3;
                            //uint32_t systime_per_tick = usec_per_qtr_note / (215 * header->division);
                            //if (systime_per_tick >= 0x10000) {
                            //    k3error::Handler("Set tempo overflow", "k3_midi_parse_track");
                            //}
                            //cur_event->data |= systime_per_tick & 0xFFFF;
                            const uint32_t systime_per_sample = 10000000 / 44100;
                            uint32_t samples_per_tick = (10 * usec_per_qtr_note) / (header->division * systime_per_sample);
                            if (samples_per_tick >= 0x10000) {
                                k3error::Handler("Set tempo overflow", "k3_midi_parse_track");
                            }
                            cur_event->data |= samples_per_tick & 0xFFFF;
                        }
                        cur_event++;
                    } else {
                        fseek(fh, length, SEEK_CUR);
                        running_length += length;
                    }
                    num_events++;
                    delta_time = 0;
                    break;
                default:
                    fseek(fh, length, SEEK_CUR);
                    running_length += length;
                    break;
                }
                //printf("meta event 0x%x, length = %d\n", meta_event_type, length);
                break;
            default:
                midi_channel = event_type & 0xF;
                midi_event = event_type & 0xF0;
                if (midi_event == K3_MIDI_MSG_NOTE_OFF ||
                    midi_event == K3_MIDI_MSG_NOTE_ON ||
                    midi_event == K3_MIDI_MSG_CONTROL_CHANGE ||
                    midi_event == K3_MIDI_MSG_PITCH_WHEEL ||
                    midi_event == K3_MIDI_MSG_KEY_PRESSURE) {
                    running_length += fread(&c1, 1, 1, fh);
                    if (midi_event == K3_MIDI_MSG_NOTE_ON && c1 == 0) {
                        // if note on and velocity is 0, it's really an off message
                        midi_event = K3_MIDI_MSG_NOTE_OFF;
                    }
                }
                // Midi messages - channel set in lower 4 bits of event_type
                switch (midi_event) {
                case K3_MIDI_MSG_NOTE_OFF:
                    i = K3_MIDI_CHANNEL_REMAP(midi_channel);
                    if (midi_channel_mapped[midi_channel] == 1 && (midi_channel_note[midi_channel] == c0 || i == K3_DSP_PERCUSSIVE_VOICE)) {
                        if (cur_event) {
                            op_channel_last_note_off_event[i] = cur_event;
                            cur_event->f.delta_time = (uint8_t)delta_time;
                            cur_event->f.command = midi_event | i;
                            cur_event->f.data0 = c0;
                            cur_event->f.data1 = 0x0;
                            cur_event++;
                        }
                        op_channel_last_note_off_time[i] = time;
                        midi_channel_note[midi_channel] = -1;
                        num_events++;
                        delta_time = 0;
                    }
                    break;

                case K3_MIDI_MSG_NOTE_ON:
                    ch = K3_MIDI_CHANNEL_REMAP(midi_channel);
                    if ((midi_channel_mapped[midi_channel] == 0) || ((time - op_channel_last_note_off_time[ch]) > op_channel_reset_threshold_time)) {
                        if (op_channel_last_note_off_event[ch]) {
                            op_channel_last_note_off_event[ch]->f.data1 = 0xFF;
                        }
                        if (cur_event) {
                            cur_event->f.delta_time = (uint8_t)delta_time;
                            cur_event->f.command = K3_MIDI_MSG_PROGRAM_CHANGE | ch;
                            cur_event->f.data0 = midi_channel_voice[midi_channel];
                            cur_event++;
                        }
                        midi_channel_mapped[midi_channel] = 1;
                        num_events++;
                        delta_time = 0;
                    }
                    op_channel_last_note_off_event[ch] = NULL;
                    op_channel_last_note_off_time[ch] = 0x7FFFFFFF;
                    if (cur_event) {
                        cur_event->f.delta_time = (uint8_t)delta_time;
                        cur_event->f.command = midi_event | ch;
                        cur_event->f.data0 = c0;
                        cur_event->f.data1 = (c1 * midi_channel_volume[midi_channel]) >> 7;
                        cur_event++;
                    }
                    midi_channel_note[midi_channel] = c0;
                    num_events++;
                    delta_time = 0;
                    if (ch >= num_opl_channel_used) num_opl_channel_used = ch + 1;
                    break;

                    //case K3_MIDI_MSG_CONTROL_CHANGE:
                case K3_MIDI_MSG_PITCH_WHEEL:
                    i = K3_MIDI_CHANNEL_REMAP(midi_channel);
                    if (midi_channel_mapped[midi_channel] == 1) {
                        if (cur_event) {
                            cur_event->f.delta_time = (uint8_t)delta_time;
                            cur_event->f.command = midi_event | i;
                            cur_event->f.data0 = c0;
                            cur_event->f.data1 = c1;
                            cur_event++;
                        }
                        //op_channel_last_note_off_event[i] = NULL;
                        //op_channel_last_note_off_time[i] = 0x7FFFFFFF;
                        num_events++;
                        delta_time = 0;
                    }
                    break;

                case K3_MIDI_MSG_PROGRAM_CHANGE:
                    if (midi_channel != K3_MIDI_PERCUSSION_CHANNEL) {
                        midi_channel_voice[midi_channel] = c0;
                        i = K3_MIDI_CHANNEL_REMAP(midi_channel);
                        midi_channel_mapped[midi_channel] = 0;
                    }
                    break;

                case K3_MIDI_MSG_CONTROL_CHANGE:
                    switch (c0) {
                    case K3_MIDI_CTRL_MAIN_VOLUME:
                        midi_channel_volume[midi_channel] = c1;
                        break;
                    }
                    break;

                case K3_MIDI_MSG_KEY_PRESSURE:
                case K3_MIDI_MSG_CHANNEL_PRESSURE:
                    break;

                default:
                    //printf("fpos 0x%x (d %d, t %d): ", cur_file_pos, next_delta_time, time);
                    //printf("Unknown midi message 0x%x\n", event_type);
                    k3error::Handler("Unknown midi message");
                    //return num_events;
                }
            }
        }
    }
    *base_opl_channel = num_opl_channel_used;
    //printf("chunk length = %d, running length = %d\n", chunk_length, running_length);
    return num_events;
}

uint32_t k3_midi_get_delta_time(const k3_midi_event_t** e)
{
    uint32_t next_delta_time = (*e)->f.delta_time;
    uint32_t delta_time = 0;
    while (next_delta_time == 0xFF) {
        delta_time += (*e)->data & 0xFFFFFF;
        (*e)++;
        next_delta_time = (*e)->f.delta_time;
    }
    delta_time += next_delta_time;
    return delta_time;
}

void k3_midi_set_delta_time(k3_midi_event_t** e, uint32_t delta_time)
{
    if (delta_time >= 0xFF) {
        (*e)->data = delta_time | 0xFF000000;
        (*e)++;
        (*e)->f.delta_time = 0x0;
    } else {
        (*e)->f.delta_time = (uint8_t)delta_time;
    }
}

// combines 2 tracks into a single track
void k3_midi_mix_tracks(k3_midi_event_t* dest, const k3_midi_event_t* src0, const k3_midi_event_t* src1, int* num_channels_used, uint32_t dest_size)
{
    uint32_t src0_delta_time = k3_midi_get_delta_time(&src0);
    uint32_t src1_delta_time = k3_midi_get_delta_time(&src1);
    bool src0_done = (src0->f.command == K3_MIDI_META_EVENT_TYPE_END_TRACK);
    bool src1_done = (src1->f.command == K3_MIDI_META_EVENT_TYPE_END_TRACK);
    uint8_t channel_map[2][16];
    memset(channel_map, 0xFF, 32);
    channel_map[0][K3_DSP_PERCUSSIVE_VOICE] = K3_DSP_PERCUSSIVE_VOICE;  // percussion must use this channel
    channel_map[1][K3_DSP_PERCUSSIVE_VOICE] = K3_DSP_PERCUSSIVE_VOICE;  // percussion must use this channel
    *num_channels_used = 0;
    uint32_t channels_used = 0x0;
    uint32_t dest_written = 0;
    uint32_t src_ch;

    //printf("Start mix...\n");
    while (!src0_done || !src1_done) {
        if ((src0_delta_time <= src1_delta_time && !src0_done) || src1_done) {
            k3_midi_set_delta_time(&dest, src0_delta_time);
            dest->data &= 0xFF000000;
            dest->data |= src0->data & 0xFFFFFF;
            //printf("src0: 0x%x %d\n", src0->data, src0_done);
            src0++;
            src1_delta_time -= src0_delta_time;
            src0_delta_time = k3_midi_get_delta_time(&src0);
            if (src0->f.command == K3_MIDI_META_EVENT_TYPE_END_TRACK) src0_done = true;
            src_ch = 0;
        } else {
            k3_midi_set_delta_time(&dest, src1_delta_time);
            dest->data &= 0xFF000000;
            dest->data |= src1->data & 0xFFFFFF;
            //printf("src1: 0x%x\n", src1->data);
            src1++;
            src0_delta_time -= src1_delta_time;
            src1_delta_time = k3_midi_get_delta_time(&src1);
            if (src1->f.command == K3_MIDI_META_EVENT_TYPE_END_TRACK) src1_done = true;
            src_ch = 1;
        }
        if (dest->f.command & 0x80) {
            // midi event - need to remap channels
            uint8_t channel = dest->f.command & 0xF;
            // check if channel is unmapped
            if (channel_map[src_ch][channel] == 0xFF) {
                // if unmapped, allocate the first available channel
                int i;
                // pick the first available channel
                for (i = 0; i < 16; i++) {
                    if (((channels_used >> i) & 1) == 0) {
                        break;
                    }
                }
                if (i < 16) {
                    // assign the mapping
                    channel_map[src_ch][channel] = i;
                    channels_used |= (1 << i);
                    //printf("alloc ch %d to %d  cmd: 0x%x\n", channel, i, dest->f.command & 0xF0);
                    // if this is the highest channel we've assigned, set num channels used field
                    if (i >= *num_channels_used) *num_channels_used = i + 1;
                } else {
                    k3error::Handler("Out of voices", "k3_midi_mix_tracks");
                }
            }
            dest->f.command &= 0xF0;
            dest->f.command |= channel_map[src_ch][channel] & 0xF;
            // if we have a note off, with the marker indicating we're done with channel,
            // then clear the mapping
            if (((dest->f.command & 0xF0) == K3_MIDI_MSG_NOTE_OFF) && (dest->f.data1 == 0xFF)) {
                //printf("freeing ch %d from %d\n", channel, channel_map[src_ch][channel]);
                channels_used &= ~(1 << channel_map[src_ch][channel]);
                channel_map[src_ch][channel] = 0xFF;
            }
        }
        dest++;
        dest_written++;
        if (dest_written >= dest_size) {
            k3error::Handler("Dest overwritten", "k3_midi_mix_tracks");
        }
    }
    // end the track
    dest->data = 0x0;
    dest->f.command = K3_MIDI_META_EVENT_TYPE_END_TRACK;
}

K3API k3sampleData k3midiLoadFromFile(const char* filename)
{
    if (filename == NULL) {
        k3error::Handler("NULL filename", "k3midiLoadFromFile");
        return NULL;
    }

    FILE* fh = fopen(filename, "rb");
    k3sampleData midi_data = k3midiLoadFromFileHandle(fh);
    fclose(fh);
    return midi_data;
}

K3API k3sampleData k3midiLoadFromFileHandle(FILE* fh)
{
    if (fh == NULL) {
        k3error::Handler("File not found", "k3midiLoadFromFileHandle");
        return NULL;
    }

    k3sampleData final_track_data = NULL;
    k3_midi_header_t midi_header;
    k3_midi_fread_bigend(fh, &midi_header.chunk_type, sizeof(uint32_t));
    k3_midi_fread_bigend(fh, &midi_header.length, sizeof(uint32_t));
    if (midi_header.chunk_type == K3_MIDI_HEADER_CHUNK_ID && midi_header.length == 6) {
        k3_midi_fread_bigend(fh, &midi_header.format, sizeof(uint16_t));
        k3_midi_fread_bigend(fh, &midi_header.num_tracks, sizeof(uint16_t));
        k3_midi_fread_bigend(fh, &midi_header.division, sizeof(uint16_t));
        int i, first_track_pos;
        int max_track_events = 0, total_events = 0;
        int num_channels = 0;
        first_track_pos = ftell(fh);
        //midi_header.num_tracks = 5;
        for (i = 0; i < midi_header.num_tracks; i++) {
            int num_events = k3_midi_parse_track(fh, &midi_header, NULL, &num_channels);
            //printf("number of events on track  %d = %d\n", i, num_events);
            total_events += num_events;
            if (num_events > max_track_events) max_track_events = num_events;
        }
        fseek(fh, first_track_pos, SEEK_SET);
        num_channels = 0;
        k3sampleData new_track_data = k3sampleDataObj::Create();
        k3_midi_event_t* new_track = (k3_midi_event_t*)new_track_data->newWrite((max_track_events + 2) * sizeof(k3_midi_event_t));
        new_track->data = K3_MIDI_HEADER_CHUNK_ID; new_track++;
        new_track->data = K3_MIDI_TRACK_CHUNK_ID; new_track++;
        k3sampleData mix_track0_data = k3sampleDataObj::Create();
        k3_midi_event_t* mix_track0 = (k3_midi_event_t*)mix_track0_data->newWrite((total_events + 2) * sizeof(k3_midi_event_t));
        mix_track0->data = K3_MIDI_HEADER_CHUNK_ID; mix_track0++;
        mix_track0->data = K3_MIDI_TRACK_CHUNK_ID; mix_track0++;
        k3sampleData mix_track1_data = k3sampleDataObj::Create();
        k3_midi_event_t* mix_track1 = (k3_midi_event_t*)mix_track1_data->newWrite((total_events + 2) * sizeof(k3_midi_event_t));
        mix_track1->data = K3_MIDI_HEADER_CHUNK_ID; mix_track1++;
        mix_track1->data = K3_MIDI_TRACK_CHUNK_ID; mix_track1++;
        k3_midi_event_t* final_track = NULL;
        mix_track0->data = 0x0;
        mix_track0->f.command = K3_MIDI_META_EVENT_TYPE_END_TRACK;
        for (i = 0; i < midi_header.num_tracks; i++) {
            int num_events = k3_midi_parse_track(fh, &midi_header, new_track, &num_channels);
            k3_midi_mix_tracks(mix_track1, mix_track0, new_track, &num_channels, total_events);
            final_track = mix_track1;
            mix_track1 = mix_track0;
            mix_track0 = final_track;
        }
        final_track -= 2;  // reverse back to include the header/track ids
        final_track_data = (final_track == mix_track1_data->getData()) ? mix_track1_data : mix_track0_data;
    } else {
        //printf("Bad header - chunk_type 0x%x length %d\n", midi_header.chunk_type, midi_header.length);
        k3error::Handler("Bad midi header", "k3midiLoadFromFileHandle");
    }
    return final_track_data;
}

void k3dspMidiReset(k3_dsp_midi_t* midi)
{
    if (midi == NULL) {
        k3error::Handler("NULL midi object", "k3dspMidiReset");
        return;
    }

    midi->state = k3dspMidiState::HEADER;
    midi->num_output_channels = 1;
    midi->flags = 0;
    midi->midi_delta_tick = 0;
    midi->cur_sample_in_midi_tick = 0;
    midi->samples_per_midi_tick = K3_DSP_DEFAULT_SAMPLES_PER_MIDI_TICK;
    uint32_t i;
    for (i = 0; i < K3_DSP_NUM_VOICES; i++) {
        midi->vox[i].cur_vol_env = K3_DSP_VOL_ENV_OFF;
        midi->vox[i].base_preset = NULL;
        midi->vox[i].preset = NULL;
        midi->vox[i].max_volume = 0;
    }
    for (i = 0; i < K3_DSP_NUM_PERCUSSIVE_VOICES; i++) {
        midi->percussive_note_mapping[i] = 0;
    }
}

void k3dspMidiSetFlag(k3_dsp_midi_t* midi, uint32_t flag, uint32_t value)
{
    if (midi == NULL) {
        k3error::Handler("NULL midi object", "k3dspMidiSetFlag");
        return;
    }

    if (value) {
        midi->flags |= (1 << flag);
    } else {
        midi->flags &= ~(1 << flag);
    }
}

void k3dspMidiOutputChannels(k3_dsp_midi_t* midi, uint32_t num_output_channels)
{
    if (midi == NULL) {
        k3error::Handler("NULL midi object", "k3dspMidiOutputChannels");
        return;
    }

    midi->num_output_channels = num_output_channels;
}

void k3dspMidiSetSoundFont(k3_dsp_midi_t* midi, k3soundFont sf2)
{
    if (midi == NULL) {
        k3error::Handler("NULL midi object", "k3dspMidiSetSoundFont");
        return;
    }

    midi->sf2 = sf2;
}

k3dspMidiState k3dspMidiGetState(k3_dsp_midi_t* midi)
{
    if (midi == NULL) {
        k3error::Handler("NULL midi object", "k3dspMidiGetState");
        return k3dspMidiState::ERROR;
    }

    return midi->state;
}

int16_t k3_dsp_sample_vox(k3_dsp_midi_t* midi, uint32_t v)
{
    // determine which part of volume envelpoe we are in
    if (midi->vox[v].cur_vol_env == K3_DSP_VOL_ENV_DELAY && midi->vox[v].cur_sample_vol_env >= midi->vox[v].preset->data.delay_samples) {
        midi->vox[v].cur_vol_env = K3_DSP_VOL_ENV_ATTACK;
        midi->vox[v].cur_sample_vol_env = 0;
        midi->vox[v].volume = midi->vox[v].attack_volume_inc;
    }
    if (midi->vox[v].cur_vol_env == K3_DSP_VOL_ENV_ATTACK && midi->vox[v].cur_sample_vol_env >= midi->vox[v].preset->data.attack_samples) {
        midi->vox[v].cur_vol_env = K3_DSP_VOL_ENV_HOLD;
        midi->vox[v].cur_sample_vol_env = 0;
        midi->vox[v].volume = midi->vox[v].max_volume;
    }
    if (midi->vox[v].cur_vol_env == K3_DSP_VOL_ENV_HOLD && midi->vox[v].cur_sample_vol_env >= midi->vox[v].preset->data.hold_samples) {
        midi->vox[v].cur_vol_env = K3_DSP_VOL_ENV_DECAY;
        midi->vox[v].cur_sample_vol_env = 0;
        midi->vox[v].volume = midi->vox[v].max_volume - midi->vox[v].decay_volume_dec;
    }
    if (midi->vox[v].cur_vol_env == K3_DSP_VOL_ENV_DECAY && midi->vox[v].cur_sample_vol_env >= midi->vox[v].preset->data.decay_samples) {
        midi->vox[v].cur_vol_env = K3_DSP_VOL_ENV_SUSTAIN;
        midi->vox[v].cur_sample_vol_env = 0;
        midi->vox[v].volume = midi->vox[v].sustain_volume;
    }
    if (midi->vox[v].cur_vol_env == K3_DSP_VOL_ENV_RELEASE) {
        if (midi->vox[v].cur_sample_vol_env == 0) midi->vox[v].volume = midi->vox[v].sustain_volume - midi->vox[v].release_volume_dec;
        if (midi->vox[v].cur_sample_vol_env >= midi->vox[v].preset->data.release_samples) {
            midi->vox[v].cur_vol_env = K3_DSP_VOL_ENV_OFF;
            midi->vox[v].volume = 0;
        }
    }

    int32_t sample;
    k3soundFontImpl* sf2 = midi->sf2->getImpl();
    const int16_t* sample_data = (const int16_t*)sf2->sample->getData();
    sample = sample_data[midi->vox[v].cur_index] * (0x10000 - midi->vox[v].cur_fract);
    sample += sample_data[midi->vox[v].cur_index + 1] * midi->vox[v].cur_fract;
    sample >>= 16;
    sample *= (midi->vox[v].volume >> 16);
    midi->vox[v].cur_fract += midi->vox[v].sample_inc;
    midi->vox[v].cur_index += midi->vox[v].cur_fract >> 16;
    midi->vox[v].cur_fract &= 0xFFFF;
    if (midi->vox[v].cur_index >= midi->vox[v].preset->data.end_sample) {
        midi->vox[v].cur_index = midi->vox[v].preset->data.start_sample;
        midi->vox[v].cur_vol_env = K3_DSP_VOL_ENV_OFF;
    }
    if (midi->vox[v].cur_index >= midi->vox[v].preset->data.end_loop_sample) {
        midi->vox[v].cur_index = midi->vox[v].preset->data.start_loop_sample + (midi->vox[v].cur_index - midi->vox[v].preset->data.end_loop_sample);
    }

    midi->vox[v].cur_sample_vol_env++;
    switch (midi->vox[v].cur_vol_env) {
    case K3_DSP_VOL_ENV_ATTACK:  midi->vox[v].volume += midi->vox[v].attack_volume_inc; break;
    case K3_DSP_VOL_ENV_DECAY:   midi->vox[v].volume -= midi->vox[v].decay_volume_dec; break;
    case K3_DSP_VOL_ENV_RELEASE: midi->vox[v].volume -= midi->vox[v].release_volume_dec; break;
    }
    return sample >> 16;
}

k3dspMidiState k3dspMidiProcess(k3_dsp_midi_t* midi, const uint8_t* in_stream, uint32_t* in_size, int16_t* out_stream, uint32_t* out_size)
{
    if (midi == NULL) {
        k3error::Handler("NULL midi object", "k3dspMidiProcess");
        return k3dspMidiState::ERROR;
    }
    if (midi->sf2 == NULL) {
        k3error::Handler("No sound font attached", "k3dspMidiProcess");
        midi->state = k3dspMidiState::ERROR;
        return k3dspMidiState::ERROR;
    }
    const k3_midi_event_t* cur_event = (const k3_midi_event_t*)(in_stream);
    uint32_t event_delta_tick;
    uint32_t bytes_read = 0;
    uint32_t samples_written = 0;
    bool done = false;
    bool midi_done;
    k3soundFontImpl* sf2 = midi->sf2->getImpl();
    int16_t sample;
    uint32_t v;
    int16_t* cur_out_stream = out_stream;

    while (!done) {
        switch (midi->state) {
        case k3dspMidiState::HEADER:
            if (midi->cur_sample_in_midi_tick + *in_size < 8) {
                midi->cur_sample_in_midi_tick += *in_size;
                bytes_read += *in_size;
            } else {
                bytes_read += 8 - midi->cur_sample_in_midi_tick;
                midi->cur_sample_in_midi_tick = 0;
                cur_event = (const k3_midi_event_t*)(in_stream + bytes_read);
                midi->state = k3dspMidiState::EVENTS;
            }
            break;
        case k3dspMidiState::EVENTS:
            // First process any midi events
            midi_done = (cur_event == NULL) ? true : false;
            while (!midi_done) {
                event_delta_tick = k3_midi_get_delta_time(&cur_event);
                if (midi->midi_delta_tick >= event_delta_tick) {
                    midi->midi_delta_tick -= event_delta_tick;
                    uint8_t channel = cur_event->f.command & 0xF;
                    uint8_t note = cur_event->f.data0;
                    uint8_t velocity = cur_event->f.data1 + 1;
                    uint8_t octave;
                    uint16_t delta_key;
                    // Channel events
                    switch (cur_event->f.command & 0xF0) {
                    case K3_MIDI_MSG_NOTE_OFF:
                        if (channel != K3_DSP_PERCUSSIVE_VOICE) {
                            // non-percussive
                            if (midi->vox[channel].cur_vol_env != K3_DSP_VOL_ENV_OFF) {
                                midi->vox[channel].cur_vol_env = K3_DSP_VOL_ENV_RELEASE;
                                midi->vox[channel].cur_sample_vol_env = 0;
                            }
                        } else {
                            bool found = false;
                            for (channel = 0; !found && channel < K3_DSP_NUM_PERCUSSIVE_VOICES; channel++) {
                                if (midi->percussive_note_mapping[channel] == note) {
                                    found = true;
                                    break;
                                }
                            }
                            if (found) {
                                midi->percussive_note_mapping[channel] = 0;
                                channel += K3_DSP_NUM_MELODIC_VOICES;
                                //printf("note off: %d ch %d\n", note, channel);
                                if (midi->vox[channel].cur_vol_env != K3_DSP_VOL_ENV_OFF) {
                                    midi->vox[channel].cur_vol_env = K3_DSP_VOL_ENV_RELEASE;
                                    midi->vox[channel].cur_sample_vol_env = 0;
                                }
                            }
                        }
                        break;
                    case K3_MIDI_MSG_NOTE_ON:
                        if (channel != K3_DSP_PERCUSSIVE_VOICE) {
                            // non-percussive
                            // find a preset for the note being played
                            for (midi->vox[channel].preset = midi->vox[channel].base_preset; midi->vox[channel].preset != NULL; midi->vox[channel].preset = midi->vox[channel].preset->next) {
                                if (note >= midi->vox[channel].preset->data.key_lo && note <= midi->vox[channel].preset->data.key_hi) break;
                            }
                            // make sure a preset is assigned to the voice
                            if (midi->vox[channel].preset != NULL) {
                                int pitch_correction = midi->vox[channel].preset->data.pitch_correction;
                                delta_key = 8 * 12;  // base frequencies are pre-multiplied by 8 octaves
                                delta_key -= (note - midi->vox[channel].preset->data.key_pitch);
                                if (pitch_correction > 0) {
                                    delta_key--;
                                    pitch_correction = 100 - pitch_correction;
                                } else {
                                    pitch_correction = -pitch_correction;
                                }
                                pitch_correction /= 4;
                                octave = delta_key / 12;
                                delta_key = delta_key % 12;
                                delta_key *= 25;
                                delta_key += pitch_correction;

                                if (midi->vox[channel].preset->data.sample_rate < 30000) octave++;  // should be 22050, or less
                                if (midi->vox[channel].preset->data.sample_rate < 15000) octave++;  // should be 11025, or less
                                if (midi->vox[channel].preset->data.sample_rate < 7500) octave++;   // should be 5512
                                midi->vox[channel].cur_vol_env = K3_DSP_VOL_ENV_DELAY;
                                midi->vox[channel].cur_sample_vol_env = 0;
                                midi->vox[channel].cur_index = midi->vox[channel].preset->data.start_sample;
                                midi->vox[channel].cur_fract = 0;
                                midi->vox[channel].sample_inc = k3_dsp_freq_table[delta_key] >> octave;
                                midi->vox[channel].volume = 0;
                                midi->vox[channel].max_volume = (velocity << 23) | (velocity << 16) | (velocity << 9) | (velocity << 2);
                                midi->vox[channel].sustain_volume = (midi->vox[channel].max_volume >> 16) * midi->vox[channel].preset->data.sustain_level;
                                midi->vox[channel].sustain_volume = (midi->vox[channel].sustain_volume < midi->vox[channel].max_volume) ? midi->vox[channel].max_volume - midi->vox[channel].sustain_volume : 0;
                                midi->vox[channel].attack_volume_inc = midi->vox[channel].max_volume / (midi->vox[channel].preset->data.attack_samples + 1);
                                midi->vox[channel].decay_volume_dec = (midi->vox[channel].max_volume - midi->vox[channel].sustain_volume) / (midi->vox[channel].preset->data.decay_samples + 1);
                                midi->vox[channel].release_volume_dec = midi->vox[channel].sustain_volume / (midi->vox[channel].preset->data.release_samples + 1);
                            }
                        } else {
                            bool found = false;
                            for (channel = 0; !found && channel < K3_DSP_NUM_PERCUSSIVE_VOICES; channel++) {
                                if (midi->vox[K3_DSP_NUM_MELODIC_VOICES + channel].cur_vol_env == K3_DSP_VOL_ENV_OFF) {
                                    found = true;
                                    break;
                                }
                            }
                            if (!found) {
                                int best_channel;
                                uint32_t best_vol_env = 0;
                                for (channel = 0; channel < K3_DSP_NUM_PERCUSSIVE_VOICES; channel++) {
                                    if (midi->vox[K3_DSP_NUM_MELODIC_VOICES + channel].cur_vol_env >= best_vol_env) {
                                        best_vol_env = midi->vox[K3_DSP_NUM_MELODIC_VOICES + channel].cur_vol_env;
                                        best_channel = channel;
                                    }
                                }
                                channel = best_channel;
                            }
                            midi->percussive_note_mapping[channel] = note;
                            channel += K3_DSP_NUM_MELODIC_VOICES;
                            octave = 0;
                            // find a preset for the note being played
                            for (midi->vox[channel].preset = sf2->presets[128]->GetHead(); midi->vox[channel].preset != NULL; midi->vox[channel].preset = midi->vox[channel].preset->next) {
                                if (note >= midi->vox[channel].preset->data.key_lo && note <= midi->vox[channel].preset->data.key_hi) break;
                            }
                            // make sure a preset is assigned to the voice
                            if (midi->vox[channel].preset != NULL) {
                                if (midi->vox[channel].preset->data.sample_rate < 30000) octave++;  // should be 22050, or less
                                if (midi->vox[channel].preset->data.sample_rate < 15000) octave++;  // should be 11025, or less
                                if (midi->vox[channel].preset->data.sample_rate < 7500) octave++;   // should be 5512
                                midi->vox[channel].cur_vol_env = K3_DSP_VOL_ENV_DELAY;
                                midi->vox[channel].cur_sample_vol_env = 0;
                                midi->vox[channel].cur_index = midi->vox[channel].preset->data.start_sample;
                                midi->vox[channel].cur_fract = 0;
                                midi->vox[channel].sample_inc = 0x10000 >> octave;
                                midi->vox[channel].volume = 0;
                                midi->vox[channel].max_volume = (velocity << 23) | (velocity << 16) | (velocity << 9) | (velocity << 2);
                                midi->vox[channel].sustain_volume = (midi->vox[channel].max_volume >> 16) * midi->vox[channel].preset->data.sustain_level;
                                midi->vox[channel].sustain_volume = (midi->vox[channel].sustain_volume < midi->vox[channel].max_volume) ? midi->vox[channel].max_volume - midi->vox[channel].sustain_volume : 0;
                                midi->vox[channel].attack_volume_inc = midi->vox[channel].max_volume / (midi->vox[channel].preset->data.attack_samples + 1);
                                midi->vox[channel].decay_volume_dec = (midi->vox[channel].max_volume - midi->vox[channel].sustain_volume) / (midi->vox[channel].preset->data.decay_samples + 1);
                                midi->vox[channel].release_volume_dec = midi->vox[channel].sustain_volume / (midi->vox[channel].preset->data.release_samples + 1);
                                //printf("note on: %d ch %d lo %d hi %d st %d end %d\n", note, channel, ise_dsp_vox[channel].preset->key_lo, ise_dsp_vox[channel].preset->key_hi, ise_dsp_vox[channel].preset->start_sample, ise_dsp_vox[channel].preset->end_sample);
                            }
                        }
                        break;
                    case K3_MIDI_MSG_PROGRAM_CHANGE:
                        if (channel != K3_DSP_PERCUSSIVE_VOICE) {
                            // non-precussive
                            midi->vox[channel].base_preset = sf2->presets[note]->GetHead();
                        }
                        break;
                    }
                    // Global events
                    switch (cur_event->f.command) {
                    case K3_MIDI_META_EVENT_TYPE_END_TRACK:
                        cur_event = NULL;
                        midi_done = true;
                        // if we're done, consume the rest of the bytes
                        bytes_read = *in_size;
                        break;
                    case K3_MIDI_META_EVENT_TYPE_SET_TEMPO:
                        //midi->samples_per_midi_tick = ((cur_event->data & 0xFFFF) * 38755) >> 12;
                        midi->samples_per_midi_tick = (cur_event->data & 0xFFFF);
                    default:
                        cur_event++;
                        bytes_read += 4;
                        break;
                    }
                } else {
                    midi_done = true;
                }
            }
            // Load in all samples
            while (midi->cur_sample_in_midi_tick < midi->samples_per_midi_tick && samples_written < *out_size) {
                sample = (midi->flags & (1 << K3_DSP_MIDI_FLAG_BLEND_OUTPUT)) ? *cur_out_stream : 0;
                for (v = 0; v < K3_DSP_NUM_VOICES; v++) {
                    if (midi->vox[v].cur_vol_env != K3_DSP_VOL_ENV_OFF) {
                        sample = k3_dsp_blend_sample16(sample, k3_dsp_sample_vox(midi, v));
                    }
                }
                if (sample > 0x7FFF) sample = 0x7FFF;
                if (sample < -0x8000) sample = -0x8000;
                for (v = 0; v < midi->num_output_channels; v++) {
                    *cur_out_stream = sample;
                    cur_out_stream++;
                    samples_written++;
                }
                midi->cur_sample_in_midi_tick++;
            }

            // increment the midi tick, if needed
            if (midi->cur_sample_in_midi_tick >= midi->samples_per_midi_tick) {
                midi->cur_sample_in_midi_tick -= midi->samples_per_midi_tick;
                midi->midi_delta_tick++;
            }
            break;
        case k3dspMidiState::ERROR:
            done = true;
            break;
        }
        done = done || (*in_size - bytes_read < sizeof(k3_midi_event_t)) || (samples_written == *out_size);
    }
    *in_size = bytes_read;
    *out_size = samples_written;
    return midi->state;
}
