#define MINIMP3_IMPLEMENTATION
#include "k3internal.h"

int16_t k3_dsp_blend_sample16(int16_t a, int16_t b)
{
    uint32_t a32 = (a ^ 0x8000) & 0xffff;
    uint32_t b32 = (b ^ 0x8000) & 0xffff;
    if (a32 < 0x8000 && b32 < 0x8000) {
        a32 = (a32 * b32) >> 15;
    } else {
        a32 = 2 * (a32 + b32) - ((a32 * b32) >> 15) - 0x10000;
    }
    if (a32 >= 0x10000) {
        a32 = 0xffff;
    }
    a32 ^= 0x8000;
    return (int16_t)a32;
}

int32_t k3_dsp_blend_sample32(int32_t a, int32_t b)
{
    uint64_t a64 = (a ^ 0x80000000) & 0xffffffff;
    uint64_t b64 = (b ^ 0x80000000) & 0xffffffff;
    if (a64 < 0x80000000 && b64 < 0x80000000) {
        a64 = (a64 * b64) >> 31;
    } else {
        a64 = 2 * (a64 + b64) - ((a64 * b64) >> 31) - 0x100000000ULL;
    }
    if (a64 >= 0x100000000ULL) {
        a64 = 0xffffffff;
    }
    a64 ^= 0x80000000;
    return (int32_t)a64;
}

void k3dspWavReset(k3_dsp_wav_t* wav)
{
    if (wav) {
        memset(wav, 0, sizeof(wav));
    } else {
        k3error::Handler("NULL wav handle", "k3dspWavReset");
    }
}

void k3dspWavSetFlag(k3_dsp_wav_t* wav, uint32_t flag, uint32_t value)
{
    if (wav) {
        if (value) {
            wav->flags |= (1 << flag);
        } else {
            wav->flags &= ~(1 << flag);
        }
    } else {
        k3error::Handler("NULL wav handle", "k3dspWavSetFlag");
    }
}

void k3dspWavOutputChannels(k3_dsp_wav_t* wav, uint32_t num_output_channels)
{
    if (wav) {
        wav->num_output_channels = num_output_channels;
    } else {
        k3error::Handler("NULL wav handle", "k3dspWavOutputChannels");
    }
}
k3dspWavState k3dspWavGetState(const k3_dsp_wav_t* wav)
{
    if (wav) {
        return wav->state;
    } else {
        k3error::Handler("NULL wav handle", "k3dspWavGetState");
        return k3dspWavState::ERROR;
    }
}

k3dspWavState k3dspWaveProcess(k3_dsp_wav_t* wav, const uint8_t* in_stream, uint32_t* in_size, int16_t* out_stream, uint32_t* out_size)
{
    if (wav) {
        const uint8_t* cur_in_stream = in_stream;
        uint8_t* cur_out_stream;
        uint32_t in_bytes_left = *in_size;
        uint32_t written_samples = 0;
        uint32_t length, in_samples, in_bytes_per_sample, out_samples;
        uint32_t out_channels, channel, in_channel, byte;
        int16_t sample = 0;
        uint32_t sample_index;
        bool done = false;
        while (!done) {
            length = 0;
            cur_out_stream = NULL;
            switch (wav->state) {
            case k3dspWavState::HEADER:
                length = sizeof(k3_dsp_riff_header_t);
                cur_out_stream = (uint8_t*)&wav->hdr;
                cur_out_stream += wav->bytes_read;
                break;
            case k3dspWavState::SUBCHUNK_HDR:
                length = sizeof(k3_dsp_riff_subchunk_header_t);
                cur_out_stream = (uint8_t*)&wav->subchunk_hdr;
                cur_out_stream += wav->bytes_read;
                break;
            case k3dspWavState::SUBCHUNK_PAD:
                length = wav->subchunk_hdr.size;
                break;
            case k3dspWavState::FORMAT:
                length = sizeof(k3_dsp_wav_format_t);
                cur_out_stream = (uint8_t*)&wav->fmt;
                cur_out_stream += wav->bytes_read;
                break;
            case k3dspWavState::DATA:
                in_bytes_per_sample = wav->fmt.bits_per_sample / 8;
                in_samples = in_bytes_left / in_bytes_per_sample / wav->fmt.num_channels;
                out_channels = (wav->num_output_channels) ? wav->num_output_channels : wav->fmt.num_channels;
                out_samples = *out_size / out_channels;
                out_samples = (wav->total_samples - wav->samples_output < out_samples) ? wav->total_samples - wav->samples_output : out_samples;
                out_samples = (in_samples < out_samples) ? in_samples : out_samples;
                //length = out_samples * in_bytes_per_sample * wav->fmt.num_channels;
                for (written_samples = 0; written_samples < out_samples; written_samples++) {
                    for (channel = 0; channel < out_channels; channel++) {
                        in_channel = (channel >= wav->fmt.num_channels) ? 0 : channel;
                        switch (in_bytes_per_sample) {
                        case 1:
                            sample = *(cur_in_stream + in_channel);
                            // this is unsigned, so subtract 128 to make it signed
                            sample -= 128;
                            sample <<= 8;
                            break;
                        case 2:
                            sample = *(cur_in_stream + in_channel * 2 + 0);
                            sample |= *(cur_in_stream + in_channel * 2 + 1) << 8;
                            break;
                        case 3:
                            sample = *(cur_in_stream + in_channel * 3 + 1);
                            sample |= *(cur_in_stream + in_channel * 3 + 2) << 8;
                            break;
                        case 4:
                            sample = *(cur_in_stream + in_channel * 2 + 2);
                            sample |= *(cur_in_stream + in_channel * 2 + 3) << 8;
                            break;
                        }
                        sample_index = written_samples * out_channels + channel;
                        if (wav->flags & (1 << K3_DSP_WAV_FLAG_BLEND_OUTPUT)) {
                            /* blended write */
                            int16_t a = out_stream[sample_index];
                            int16_t b = sample;
                            out_stream[sample_index] = k3_dsp_blend_sample16(a, b);
                        } else {
                            out_stream[sample_index] = sample;
                        }
                    }
                    cur_in_stream += wav->fmt.num_channels * in_bytes_per_sample;
                    in_bytes_left -= wav->fmt.num_channels * in_bytes_per_sample;
                }
                written_samples *= out_channels;
                wav->samples_output += written_samples;
                *out_size = written_samples;
                done = true;
                break;
            case k3dspWavState::DONE:
                done = true;
            }
            if (wav->bytes_read > length) {
                wav->state = k3dspWavState::ERROR;
            }
            if(wav->state != k3dspWavState::ERROR) {
                length -= wav->bytes_read;
                bool change_state = false;
                wav->bytes_read += length;
                if (in_bytes_left < length) {
                    length = in_bytes_left;
                } else {
                    change_state = true;
                }
                if (length) {
                    if (cur_out_stream) {
                        memcpy(cur_out_stream, cur_in_stream, length);
                    }
                    cur_in_stream += length;
                    in_bytes_left -= length;
                }
                // if we completed a state, check for errors, and go to next state
                if (change_state) {
                    uint32_t bytes_per_sample;
                    switch (wav->state) {
                    case k3dspWavState::HEADER:
                        if (wav->hdr.chunk_id != K3_DSP_RIFF_CHUNK_ID || wav->hdr.format_id != K3_DSP_WAV_FORMAT_ID) {
                            wav->state = k3dspWavState::ERROR;
                        } else {
                            wav->state = k3dspWavState::SUBCHUNK_HDR;
                            wav->bytes_read = 0;
                        }
                        break;
                    case k3dspWavState::SUBCHUNK_HDR:
                        switch (wav->subchunk_hdr.id) {
                        case K3_DSP_WAV_SUBCHUNK_FMT_ID:
                            wav->state = k3dspWavState::FORMAT;
                            break;
                        case K3_DSP_WAV_SUBCHUNK_DATA_ID:
                            wav->state = k3dspWavState::DATA;
                            wav->samples_output = 0;
                            bytes_per_sample = wav->fmt.bits_per_sample / 8;
                            wav->total_samples = wav->subchunk_hdr.size / bytes_per_sample;
                            break;
                        default:
                            wav->state = k3dspWavState::SUBCHUNK_PAD;
                        }
                        wav->bytes_read = 0;
                        break;
                    case k3dspWavState::SUBCHUNK_PAD:
                        wav->state = k3dspWavState::SUBCHUNK_HDR;
                        wav->bytes_read = 0;
                        break;
                    case k3dspWavState::FORMAT:
                        if (wav->fmt.audio_format != 1 || wav->fmt.num_channels == 0) {
                            wav->state = k3dspWavState::ERROR;
                        } else {
                            wav->state = k3dspWavState::SUBCHUNK_PAD;
                        }
                        break;
                    case k3dspWavState::DATA:
                        if (wav->samples_output == wav->total_samples) {
                            wav->state = k3dspWavState::DONE;
                        }
                    }
                }
            }
            done = done || (wav->state == k3dspWavState::ERROR) || (in_bytes_left == 0);
        }
        *in_size -= in_bytes_left;
    } else {
        k3error::Handler("NULL wav handle", "k3dspWaveProcess");
        return k3dspWavState::ERROR;
    }
}

k3soundFontImpl::k3soundFontImpl()
{
    uint32_t i;
    for (i = 0; i < 129; i++) {
        presets[i] = k3llist<k3_dsp_sf2_preset_t>::Create();
    }
}

k3soundFontImpl::~k3soundFontImpl()
{
    uint32_t i;
    for (i = 0; i < 129; i++) {
        presets[i] = NULL;
    }
}

k3soundFontObj::k3soundFontObj()
{
    _data = new k3soundFontImpl;
}

k3soundFontObj::~k3soundFontObj()
{
    if (_data) {
        delete _data;
        _data = NULL;
    }
}

k3soundFontImpl* k3soundFontObj::getImpl()
{
    return _data;
}

const k3soundFontImpl* k3soundFontObj::getImpl() const
{
    return _data;
}

K3API k3soundFont k3soundFontObj::Create()
{
    k3soundFont sf = new k3soundFontObj;
    return sf;
}

void k3soundFontObj::LoadFromFile(const char* filename)
{
    if (filename == NULL) {
        k3error::Handler("NULL filename", "k3soundFontObj::LoadFromFile");
        return;
    }
    FILE* fh = fopen(filename, "rb");
    LoadFromFileHandle(fh);
    if (fh) {
        fclose(fh);
    }
}

// convert timecents to number of samples, assuming 44100 samples/second
uint32_t k3_dsp_sf2_timecents_to_samples(int16_t tc)
{
    const uint32_t TC_44100 = 18514;
    float norm_tc = (TC_44100 + tc) / 1200.0f;
    uint32_t samples = (norm_tc < 0.0f) ? 0 : pow(2, norm_tc);
    return samples;
}

// Process generator
void k3_dsp_process_sf2_gen(k3_dsp_sf2_preset_t* preset, k3_dsp_sf2_gen_list_t* gen)
{
    switch (gen->gen_op) {
    case K3_DSP_SF2_GEN_START_ADDR_OFFSET:    preset->start_sample += gen->gen_amount.u16; break;
    case K3_DSP_SF2_GEN_END_ADDR_OFFSET:      preset->end_sample += gen->gen_amount.u16; break;
    case K3_DSP_SF2_GEN_START_LOOP_OFFSET:    preset->start_loop_sample += gen->gen_amount.u16; break;
    case K3_DSP_SF2_GEN_END_LOOP_OFFSET:      preset->end_loop_sample += gen->gen_amount.u16; break;
    case K3_DSP_SF2_GEN_START_ADDR_COARSE:    preset->start_sample += gen->gen_amount.u16 << 16; break;
    case K3_DSP_SF2_GEN_END_ADDR_COARSE:      preset->end_sample += gen->gen_amount.u16 << 16; break;
    case K3_DSP_SF2_GEN_KEY_RANGE:            preset->key_hi += gen->gen_amount.range.hi; preset->key_lo += gen->gen_amount.range.lo; break;
    case K3_DSP_SF2_GEN_VEL_RANGE:            preset->vel_hi += gen->gen_amount.range.hi; preset->vel_lo += gen->gen_amount.range.lo; break;
    case K3_DSP_SF2_GEN_START_LOOP_COARSE:    preset->start_loop_sample += gen->gen_amount.u16 << 16; break;
    case K3_DSP_SF2_GEN_KEY_NUM:              preset->key_pitch += (uint8_t)gen->gen_amount.u16; break;
    case K3_DSP_SF2_GEN_END_LOOP_COARSE:      preset->end_loop_sample += gen->gen_amount.u16 << 16; break;
    case K3_DSP_SF2_GEN_SAMPLE_MODE:          preset->sample_mode = (uint8_t)gen->gen_amount.u16; break;
    case K3_DSP_SF2_GEN_OVERRIDE_PITCH:       preset->key_pitch = (uint8_t)gen->gen_amount.u16; break;
    case K3_DSP_SF2_GEN_DELAY_VOL_ENV:        preset->delay_samples = k3_dsp_sf2_timecents_to_samples(gen->gen_amount.s16); break;
    case K3_DSP_SF2_GEN_ATTACK_VOL_ENV:       preset->attack_samples = k3_dsp_sf2_timecents_to_samples(gen->gen_amount.s16); break;
    case K3_DSP_SF2_GEN_HOLD_VOL_ENV:         preset->hold_samples = k3_dsp_sf2_timecents_to_samples(gen->gen_amount.s16); break;
    case K3_DSP_SF2_GEN_DECAY_VOL_ENV:        preset->decay_samples = k3_dsp_sf2_timecents_to_samples(gen->gen_amount.s16); break;
    case K3_DSP_SF2_GEN_SUSTAIN_VOL_ENV:      preset->sustain_level = (0x10000 * gen->gen_amount.u16) / 1000; break;
    case K3_DSP_SF2_GEN_RELEASE_VOL_ENV:      preset->release_samples = k3_dsp_sf2_timecents_to_samples(gen->gen_amount.s16); break;
    }
}

void k3_dsp_process_sf2_shdr(k3_dsp_sf2_preset_t* preset, k3_dsp_sf2_sample_t* shdr)
{
    preset->start_sample += shdr->start;
    preset->end_sample += shdr->end;
    preset->start_loop_sample += shdr->start_loop;
    preset->end_loop_sample += shdr->end_loop;
    preset->sample_rate = shdr->sample_rate;
    if (preset->key_pitch == 0) preset->key_pitch = shdr->orig_pitch;
    preset->pitch_correction += shdr->pitch_correction;
}

bool k3_dsp_sf2_presets_match(k3_dsp_sf2_preset_t* p0, k3_dsp_sf2_preset_t* p1)
{
    //printf("Comparing 0x%x to 0x%x ", p0, p1);
    bool match = p0->key_lo == p1->key_hi + 1;
    //if(!match) {printf("ky %x %x\n", p1->key_lo, p1->key_hi); return match;}
    if (p0->vel_lo != p1->vel_lo) match = false;
    //if(!match) {printf("vl\n"); return match;}
    if (p0->vel_hi != p1->vel_hi) match = false;
    //if(!match) {printf("vh\n"); return match;}
    if (p0->key_pitch != p1->key_pitch) match = false;
    //if(!match) {printf("kp\n"); return match;}
    if (p0->pitch_correction != p1->pitch_correction) match = false;
    //if(!match) {printf("pc\n"); return match;}
    if (p0->sample_mode != p1->sample_mode) match = false;
    //if(!match) {printf("sm\n"); return match;}
    if (p0->start_sample != p1->start_sample) match = false;
    //if(!match) {printf("ss\n"); return match;}
    if (p0->end_sample != p1->end_sample) match = false;
    //if(!match) {printf("es\n"); return match;}
    if (p0->start_loop_sample != p1->start_loop_sample) match = false;
    //if(!match) {printf("sl\n"); return match;}
    if (p0->end_loop_sample != p1->end_loop_sample) match = false;
    //if(!match) {printf("el\n"); return match;}
    if (p0->sample_rate != p1->sample_rate) match = false;
    //if(!match) {printf("sr\n"); return match;}
    //printf(" match\n");
    if (p0->delay_samples != p1->delay_samples) match = false;
    if (p0->attack_samples != p1->attack_samples) match = false;
    if (p0->hold_samples != p1->hold_samples) match = false;
    if (p0->decay_samples != p1->decay_samples) match = false;
    if (p0->sustain_level != p1->sustain_level) match = false;
    if (p0->release_samples != p1->release_samples) match = false;
    return match;
}

void k3soundFontObj::LoadFromFileHandle(FILE* fh)
{
    if (fh == NULL) {
        k3error::Handler("File not found", "k3soundFontObj::LoadFromFileHandle");
        return;
    }

    uint32_t read_size;
    k3_dsp_riff_header_t header;
    k3_dsp_riff_subchunk_header_t subchunk;
    static const uint32_t ERROR_MSG_MAX_SIZE = 256;
    char error_msg[ERROR_MSG_MAX_SIZE];

    // read RIFF header
    fread(&header, sizeof(k3_dsp_riff_header_t), 1, fh);
    if (header.chunk_id != K3_DSP_RIFF_CHUNK_ID || header.format_id != K3_DSP_SF2_FORMAT_ID) {
        snprintf(error_msg, ERROR_MSG_MAX_SIZE, "Bad RIFF header: 0x%x", ftell(fh));
        k3error::Handler(error_msg, "k3soundFontObj::LoadFromFileHandle");
        return;
    }

    // Read Info subchunk
    fread(&header, sizeof(k3_dsp_riff_header_t), 1, fh);
    if (header.chunk_id != K3_DSP_SF2_SUBCHUNK_LIST_ID || header.format_id != K3_DSP_SF2_INFO_ID) {
        snprintf(error_msg, ERROR_MSG_MAX_SIZE, "Bad INFO subchunk: 0x%x", ftell(fh));
        k3error::Handler(error_msg, "k3soundFontObj::LoadFromFileHandle");
        return;
    }
    // skip over info subchunk
    // need to subtract 4, since we already read the "INFO" id
    fseek(fh, header.chunk_size - 4, SEEK_CUR);

    // Read sample data subchunk
    fread(&header, sizeof(k3_dsp_riff_header_t), 1, fh);
    if (header.chunk_id != K3_DSP_SF2_SUBCHUNK_LIST_ID || header.format_id != K3_DSP_SF2_SAMPLE_DATA_ID) {
        snprintf(error_msg, ERROR_MSG_MAX_SIZE, "Bad sample data: 0x%x\n", ftell(fh));
        k3error::Handler(error_msg, "k3soundFontObj::LoadFromFileHandle");
        return;
    }

    // Read sample subchunk
    fread(&subchunk, sizeof(k3_dsp_riff_subchunk_header_t), 1, fh);
    if (subchunk.id != K3_DSP_SF2_SAMPLE_ID) {
        snprintf(error_msg, ERROR_MSG_MAX_SIZE, "Bad sample: 0x%x\n", ftell(fh));
        k3error::Handler(error_msg, "k3soundFontObj::LoadFromFileHandle");
        return;
    }

    // allocate and read the sound sample data
    if (_data->sample == NULL) {
        _data->sample = k3sampleDataObj::Create();
    }
    read_size = _data->sample->LoadFromFileHandle(fh, subchunk.size);
    if (read_size != subchunk.size) {
        snprintf(error_msg, ERROR_MSG_MAX_SIZE, "Error reading sample data; sample size=%d, read size=%d\n", subchunk.size, read_size);
        k3error::Handler(error_msg, "k3soundFontObj::LoadFromFileHandle");
        return;
    }

    // read the next chunk
    fread(&header, sizeof(k3_dsp_riff_header_t), 1, fh);
    if (header.chunk_id == K3_DSP_SF2_SAMPLE24_ID) {
        // it may be sm24, and if so, skip it
        // need to subtract 4, since we read the format field
        fseek(fh, header.chunk_size - 4, SEEK_CUR);
        // read the next chunk
        fread(&header, sizeof(k3_dsp_riff_header_t), 1, fh);
    }

    // Check for preset data
    if (header.chunk_id != K3_DSP_SF2_SUBCHUNK_LIST_ID || header.format_id != K3_DSP_SF2_PRESET_DATA_ID) {
        snprintf(error_msg, ERROR_MSG_MAX_SIZE, "Bad preset header: 0x%x\n", ftell(fh));
        k3error::Handler(error_msg, "k3soundFontObj::LoadFromFileHandle");
        return;
    }

    // Read preset header subchunk
    fread(&subchunk, sizeof(k3_dsp_riff_subchunk_header_t), 1, fh);
    if (subchunk.id != K3_DSP_SF2_PRESET_HEADER_ID) {
        snprintf(error_msg, ERROR_MSG_MAX_SIZE, "Bad preset header: 0x%x\n", ftell(fh));
        k3error::Handler(error_msg, "k3soundFontObj::LoadFromFileHandle");
        return;
    }
    uint32_t num_preset_headers = subchunk.size / sizeof(k3_dsp_sf2_phdr_t);
    k3_dsp_sf2_phdr_t* phdr = new k3_dsp_sf2_phdr_t[num_preset_headers];
    if (phdr == NULL) {
        snprintf(error_msg, ERROR_MSG_MAX_SIZE, "Could not allocate phdr\n");
        k3error::Handler(error_msg, "k3soundFontObj::LoadFromFileHandle");
        return;
    }
    read_size = fread(phdr, sizeof(k3_dsp_sf2_phdr_t), num_preset_headers, fh);
    if (read_size != num_preset_headers) {
        snprintf(error_msg, ERROR_MSG_MAX_SIZE, "Incorrect preset headers read: %d expected: %d\n", read_size, num_preset_headers);
        k3error::Handler(error_msg, "k3soundFontObj::LoadFromFileHandle");
        delete[] phdr;
        return;
    }

    // Read pbag
    fread(&subchunk, sizeof(k3_dsp_riff_subchunk_header_t), 1, fh);
    if (subchunk.id != K3_DSP_SF2_PRESET_BAG_ID) {
        snprintf(error_msg, ERROR_MSG_MAX_SIZE, "Bad preset bag: 0x%x\n", ftell(fh));
        k3error::Handler(error_msg, "k3soundFontObj::LoadFromFileHandle");
        delete[] phdr;
        return;
    }
    uint32_t num_preset_bags = subchunk.size / sizeof(k3_dsp_sf2_bag_t);
    k3_dsp_sf2_bag_t* pbag = new k3_dsp_sf2_bag_t[num_preset_bags];
    if (pbag == NULL) {
        snprintf(error_msg, ERROR_MSG_MAX_SIZE, "Could not allocate pbag\n");
        k3error::Handler(error_msg, "k3soundFontObj::LoadFromFileHandle");
        delete[] phdr;
        return;
    }
    read_size = fread(pbag, sizeof(k3_dsp_sf2_bag_t), num_preset_bags, fh);
    if (read_size != num_preset_bags) {
        snprintf(error_msg, ERROR_MSG_MAX_SIZE, "Incorrect preset bags read: %d expected: %d\n", read_size, num_preset_bags);
        k3error::Handler(error_msg, "k3soundFontObj::LoadFromFileHandle");
        delete[] pbag;
        delete[] phdr;
        return;
    }

    // Skip over pmod
    fread(&subchunk, sizeof(k3_dsp_riff_subchunk_header_t), 1, fh);
    if (subchunk.id != K3_DSP_SF2_PRESET_MOD_ID) {
        snprintf(error_msg, ERROR_MSG_MAX_SIZE, "Bad preset mod: 0x%x\n", ftell(fh));
        k3error::Handler(error_msg, "k3soundFontObj::LoadFromFileHandle");
        delete[] pbag;
        delete[] phdr;
        return;
    }
    fseek(fh, subchunk.size, SEEK_CUR);

    // Read pgen
    fread(&subchunk, sizeof(k3_dsp_riff_subchunk_header_t), 1, fh);
    if (subchunk.id != K3_DSP_SF2_PRESET_GEN_ID) {
        snprintf(error_msg, ERROR_MSG_MAX_SIZE, "Bad preset gen: 0x%x\n", ftell(fh));
        k3error::Handler(error_msg, "k3soundFontObj::LoadFromFileHandle");
        delete[] pbag;
        delete[] phdr;
        return;
    }
    uint32_t num_preset_gens = subchunk.size / sizeof(k3_dsp_sf2_gen_list_t);
    k3_dsp_sf2_gen_list_t* pgen = new k3_dsp_sf2_gen_list_t[num_preset_gens];
    if (pgen == NULL) {
        snprintf(error_msg, ERROR_MSG_MAX_SIZE, "Could not allocate pgen\n");
        k3error::Handler(error_msg, "k3soundFontObj::LoadFromFileHandle");
        delete[] pbag;
        delete[] phdr;
        return;
    }
    read_size = fread(pgen, sizeof(k3_dsp_sf2_gen_list_t), num_preset_gens, fh);
    if (read_size != num_preset_gens) {
        snprintf(error_msg, ERROR_MSG_MAX_SIZE, "Incorrect preset gens read: %d expected: %d\n", read_size, num_preset_gens);
        k3error::Handler(error_msg, "k3soundFontObj::LoadFromFileHandle");
        delete[] pgen;
        delete[] pbag;
        delete[] phdr;
        return;
    }

    // Read inst
    fread(&subchunk, sizeof(k3_dsp_riff_subchunk_header_t), 1, fh);
    if (subchunk.id != K3_DSP_SF2_INST_ID) {
        snprintf(error_msg, ERROR_MSG_MAX_SIZE, "Bad inst: 0x%x\n", ftell(fh));
        k3error::Handler(error_msg, "k3soundFontObj::LoadFromFileHandle");
        delete[] pgen;
        delete[] pbag;
        delete[] phdr;
        return;
    }
    uint32_t num_insts = subchunk.size / sizeof(k3_dsp_sf2_inst_t);
    k3_dsp_sf2_inst_t* inst = new k3_dsp_sf2_inst_t[num_insts];
    if (inst == NULL) {
        snprintf(error_msg, ERROR_MSG_MAX_SIZE, "Could not allocate inst\n");
        k3error::Handler(error_msg, "k3soundFontObj::LoadFromFileHandle");
        delete[] pgen;
        delete[] pbag;
        delete[] phdr;
        return;
    }
    read_size = fread(inst, sizeof(k3_dsp_sf2_inst_t), num_insts, fh);
    if (read_size != num_insts) {
        snprintf(error_msg, ERROR_MSG_MAX_SIZE, "Incorrect inst read: %d expected: %d\n", read_size, num_insts);
        k3error::Handler(error_msg, "k3soundFontObj::LoadFromFileHandle");
        delete[] inst;
        delete[] pgen;
        delete[] pbag;
        delete[] phdr;
        return;
    }

    // read ibag
    fread(&subchunk, sizeof(k3_dsp_riff_subchunk_header_t), 1, fh);
    if (subchunk.id != K3_DSP_SF2_INST_BAG_ID) {
        snprintf(error_msg, ERROR_MSG_MAX_SIZE, "Bad inst bag: 0x%x\n", ftell(fh));
        k3error::Handler(error_msg, "k3soundFontObj::LoadFromFileHandle");
        delete[] inst;
        delete[] pgen;
        delete[] pbag;
        delete[] phdr;
        return;
    }
    uint32_t num_inst_bags = subchunk.size / sizeof(k3_dsp_sf2_bag_t);
    k3_dsp_sf2_bag_t* ibag = new k3_dsp_sf2_bag_t[num_inst_bags];
    if (ibag == NULL) {
        snprintf(error_msg, ERROR_MSG_MAX_SIZE, "Could not allocate ibag\n");
        k3error::Handler(error_msg, "k3soundFontObj::LoadFromFileHandle");
        delete[] inst;
        delete[] pgen;
        delete[] pbag;
        delete[] phdr;
        return;
    }
    read_size = fread(ibag, sizeof(k3_dsp_sf2_bag_t), num_inst_bags, fh);
    if (read_size != num_inst_bags) {
        snprintf(error_msg, ERROR_MSG_MAX_SIZE, "Incorrect inst bags read: %d expected: %d\n", read_size, num_inst_bags);
        k3error::Handler(error_msg, "k3soundFontObj::LoadFromFileHandle");
        delete[] ibag;
        delete[] inst;
        delete[] pgen;
        delete[] pbag;
        delete[] phdr;
        return;
    }

    // skip over imod
    fread(&subchunk, sizeof(k3_dsp_riff_subchunk_header_t), 1, fh);
    if (subchunk.id != K3_DSP_SF2_INST_MOD_ID) {
        snprintf(error_msg, ERROR_MSG_MAX_SIZE, "Bad inst mod: 0x%x\n", ftell(fh));
        k3error::Handler(error_msg, "k3soundFontObj::LoadFromFileHandle");
        delete[] ibag;
        delete[] inst;
        delete[] pgen;
        delete[] pbag;
        delete[] phdr;
        return;
    }
    fseek(fh, subchunk.size, SEEK_CUR);

    // read igen
    fread(&subchunk, sizeof(k3_dsp_riff_subchunk_header_t), 1, fh);
    if (subchunk.id != K3_DSP_SF2_INST_GEN_ID) {
        snprintf(error_msg, ERROR_MSG_MAX_SIZE, "Bad inst gen: 0x%x\n", ftell(fh));
        k3error::Handler(error_msg, "k3soundFontObj::LoadFromFileHandle");
        delete[] ibag;
        delete[] inst;
        delete[] pgen;
        delete[] pbag;
        delete[] phdr;
        return;
    }
    uint32_t num_inst_gens = subchunk.size / sizeof(k3_dsp_sf2_gen_list_t);
    k3_dsp_sf2_gen_list_t* igen = new k3_dsp_sf2_gen_list_t[num_inst_gens];
    if (igen == NULL) {
        snprintf(error_msg, ERROR_MSG_MAX_SIZE, "Could not allocate inst gen\n");
        k3error::Handler(error_msg, "k3soundFontObj::LoadFromFileHandle");
        delete[] ibag;
        delete[] inst;
        delete[] pgen;
        delete[] pbag;
        delete[] phdr;
        return;
    }
    read_size = fread(igen, sizeof(k3_dsp_sf2_gen_list_t), num_inst_gens, fh);
    if (read_size != num_inst_gens) {
        snprintf(error_msg, ERROR_MSG_MAX_SIZE, "Incorrect inst gens read: %d expected: %d\n", read_size, num_inst_gens);
        k3error::Handler(error_msg, "k3soundFontObj::LoadFromFileHandle");
        delete[] igen;
        delete[] ibag;
        delete[] inst;
        delete[] pgen;
        delete[] pbag;
        delete[] phdr;
        return;
    }

    // Read shdr
    fread(&subchunk, sizeof(k3_dsp_riff_subchunk_header_t), 1, fh);
    if (subchunk.id != K3_DSP_SF2_SAMPLE_HEADER_ID) {
        snprintf(error_msg, ERROR_MSG_MAX_SIZE, "Bad sample header: 0x%x\n", ftell(fh));
        k3error::Handler(error_msg, "k3soundFontObj::LoadFromFileHandle");
        delete[] igen;
        delete[] ibag;
        delete[] inst;
        delete[] pgen;
        delete[] pbag;
        delete[] phdr;
        return;
    }
    uint32_t num_sample_headers = subchunk.size / sizeof(k3_dsp_sf2_sample_t);
    k3_dsp_sf2_sample_t* shdr = new k3_dsp_sf2_sample_t[num_sample_headers];
    if (shdr == NULL) {
        snprintf(error_msg, ERROR_MSG_MAX_SIZE, "Could not allocate shdr\n");
        k3error::Handler(error_msg, "k3soundFontObj::LoadFromFileHandle");
        delete[] igen;
        delete[] ibag;
        delete[] inst;
        delete[] pgen;
        delete[] pbag;
        delete[] phdr;
        return;
    }
    read_size = fread(shdr, sizeof(k3_dsp_sf2_sample_t), num_sample_headers, fh);
    if (read_size != num_sample_headers) {
        snprintf(error_msg, ERROR_MSG_MAX_SIZE, "Incorrect sample headers read: %d expected: %d\n", read_size, num_sample_headers);
        k3error::Handler(error_msg, "k3soundFontObj::LoadFromFileHandle");
        delete[] shdr;
        delete[] igen;
        delete[] ibag;
        delete[] inst;
        delete[] pgen;
        delete[] pbag;
        delete[] phdr;
        return;
    }
    //printf("SF2 file succesfully parsed!\n");

    //printf("Num_presets=%d\n", num_preset_headers);
    uint32_t pi, zi, num_zones, gi, num_gens;
    uint32_t pii, zii, num_izones, gii, num_igens;  // for instruments
    uint32_t si;
    uint32_t next_preset_bag_ndx, next_gen_ndx, next_inst_ndx, next_inst_bag_ndx;
    k3llist<k3_dsp_sf2_preset_t>::node_t preset_node;
    k3_dsp_sf2_preset_t next_preset;
    memset(&next_preset, 0, sizeof(k3_dsp_sf2_preset_t));
    for (pi = 0; pi < num_preset_headers; pi++) {
        next_preset_bag_ndx = (pi < num_preset_headers - 1) ? phdr[pi + 1].preset_bag_ndx : num_preset_bags;
        num_zones = next_preset_bag_ndx - phdr[pi].preset_bag_ndx;
        //printf("Index %d, preset name %s preset id %d preset zone %d num zones %d\n", pi,
        //    phdr[pi].preset_name, phdr[pi].preset, phdr[pi].preset_bag_ndx, num_zones);
        for (zi = phdr[pi].preset_bag_ndx; zi < phdr[pi].preset_bag_ndx + num_zones; zi++) {
            next_gen_ndx = (zi < num_preset_bags - 1) ? pbag[zi + 1].gen_ndx : num_preset_bags;
            num_gens = next_gen_ndx - pbag[zi].gen_ndx;
            //printf("  Zindex %d first gen %d num gens %d\n", zi, pbag[zi].gen_ndx, num_gens);
            for (gi = pbag[zi].gen_ndx; gi < pbag[zi].gen_ndx + num_gens; gi++) {
                //printf("    Gindex %d op %d amount %d(0x%x)\n", gi, pgen[gi].gen_op, pgen[gi].gen_amount.u16, pgen[gi].gen_amount.u16);
                k3_dsp_process_sf2_gen(&next_preset, &pgen[gi]);
                if (pgen[gi].gen_op == K3_DSP_SF2_GEN_KEY_RANGE) {
                    if (pgen[gi].gen_amount.range.lo != 0x0 || pgen[gi].gen_amount.range.hi != 0x7f) {
                        snprintf(error_msg, ERROR_MSG_MAX_SIZE, "Unsupported pgen range 0x%x 0x%x\n", pgen[gi].gen_amount.range.lo, pgen[gi].gen_amount.range.hi);
                        k3error::Handler(error_msg, "k3soundFontObj::LoadFromFileHandle");
                    }
                    next_preset.key_lo = 0;
                    next_preset.key_hi = 0;
                }
                if (pgen[gi].gen_op == K3_DSP_SF2_GEN_INSTRUMENT) {
                    // instrument - should be last
                    pii = pgen[gi].gen_amount.u16;
                    next_inst_ndx = (pii < num_insts - 1) ? inst[pii + 1].inst_bag_ndx : num_insts;
                    num_izones = next_inst_ndx - inst[pii].inst_bag_ndx;
                    //printf("  Iindex %d inst name %s inst zone %d num zones %d\n",
                    //    pii, inst[pii].inst_name, inst[pii].inst_bag_ndx, num_izones);
                    for (zii = inst[pii].inst_bag_ndx; zii < inst[pii].inst_bag_ndx + num_izones; zii++) {
                        next_inst_bag_ndx = (zii < num_inst_bags - 1) ? ibag[zii + 1].gen_ndx : num_inst_bags;
                        num_igens = next_inst_bag_ndx - ibag[zii].gen_ndx;
                        //if(phdr[pi].bank == 128) printf("    ZIindex %d igen %d num igens %d\n", zii, ibag[zii].gen_ndx, num_igens);
                        for (gii = ibag[zii].gen_ndx; gii < ibag[zii].gen_ndx + num_igens; gii++) {
                            //if(phdr[pi].bank == 128) printf("      GIindex %d op %d amount %d(0x%x)\n", gii, igen[gii].gen_op, igen[gii].gen_amount.u16, igen[gii].gen_amount.u16);
                            k3_dsp_process_sf2_gen(&next_preset, &igen[gii]);
                            if (igen[gii].gen_op == K3_DSP_SF2_GEN_SAMPLE_ID) {
                                si = igen[gii].gen_amount.u16;
                                //if(phdr[pi].bank == 128) printf("      Sindex %d name %s start %d end %d stloop %d endloop %d rate %d pitch %d corr %d type %d link %d\n",
                                //    si, shdr[si].sample_name, shdr[si].start, shdr[si].end, shdr[si].start_loop, shdr[si].end_loop,
                                //    shdr[si].sample_rate, shdr[si].orig_pitch, shdr[si].pitch_correction, shdr[si].sample_type, shdr[si].sample_link);
                                k3_dsp_process_sf2_shdr(&next_preset, &shdr[si]);
                                // loop through presets, and look for a match
                                bool match_found = false;
                                bool already_exists = false;
                                int preset = (phdr[pi].bank == 128) ? 128 : phdr[pi].preset;
                                for (preset_node = _data->presets[preset]->GetHead(); preset_node != NULL && !match_found; preset_node = preset_node->next) {
                                    if (k3_dsp_sf2_presets_match(&next_preset, &preset_node->data)) {
                                        preset_node->data.key_hi = next_preset.key_hi;
                                        match_found = true;
                                    }
                                    if (next_preset.key_lo == preset_node->data.key_lo && next_preset.key_hi == preset_node->data.key_hi) {
                                        already_exists = true;
                                    }
                                }
                                if (!already_exists) {
                                    if (!match_found) {
                                        _data->presets[preset]->AddTail(next_preset);
                                        //} else {
                                        //    printf("Match found\n");
                                    }
                                    //} else {
                                    //    printf("Already exists\n");
                                }
                                memset(&next_preset, 0, sizeof(k3_dsp_sf2_preset_t));
                            }
                        }
                    }
                }
            }
        }
    }

    delete[] shdr;
    delete[] igen;
    delete[] ibag;
    delete[] inst;
    delete[] pgen;
    delete[] pbag;
    delete[] phdr;
}
