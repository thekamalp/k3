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
