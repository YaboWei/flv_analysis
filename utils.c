#include <stdint.h>

#include "analysis.h"

char* tag_type_name(uint8_t tag_type)
{
    switch (tag_type) {
        case FLV_TAG_TYPE_AUDIO:
            return "audio";
        case FLV_TAG_TYPE_VIDEO:
            return "video";
        case FLV_TAG_TYPE_DATA:
            return "data";
        default:
            return "unknown";
    }

    return "unknown";
}

char* flv_sound_format_name(uint8_t sound_format)
{
    switch (sound_format) {
        case FLV_SOUND_FORMAT_MP3:
            return "MP3";
        case FLV_SOUND_FORMAT_AAC:
            return "AAC";
        case FLV_SOUND_FORMAT_SPEEX:
            return "Speex";
        default:
            return "other";
    }

    return "other";
}

char* flv_sound_rate_name(uint8_t sound_rate)
{
    switch (sound_rate) {
        case 0:
            return "5.5 kHZ";
        case 1:
            return "11 kHZ";
        case 2:
            return "22 kHZ";
        case 3:
            return "44 kHZ";
        default:
            return "unknown";
    }

    return "unknown";
}

char* flv_sound_size_name(uint8_t sound_size)
{
    switch (sound_size) {
        case 0:
            return "8-bit samples";
        case 1:
            return "16-bit samples";
        default:
            return "unknown";
    }

    return "unknown";
}

char* flv_sound_type_name(uint8_t sound_type)
{
    switch (sound_type) {
        case FLV_SOUND_TYPE_MONO:
            return "Mono sound";
        case FLV_SOUND_TYPE_STEREO:
            return "Stereo sound";
        default:
            return "unknown";
    }

    return "unknown";
}

char* flv_aac_packet_type_name(uint8_t aac_packet_type)
{
    switch (aac_packet_type) {
        case 0:
            return "AAC sequence header";
        case 1:
            return "AAC raw";
        default:
            return "unknown";
    }

    return "unknown";
}

char* flv_frame_type_name(uint8_t frame_type)
{
    switch (frame_type) {
        case 1:
            return "key frame (for AVC, a seekable frame)";
        case 2:
            return "inter frame (for AVC, a non-seekable frame)";
        default:
            return "unknown";
    }

    return "unknown";
}

char* flv_codec_name(uint8_t codec_id)
{
    switch (codec_id) {
        case FLV_VIDEO_CODEC_AVC:
            return "AVC";
        default:
            return "other";
    }

    return "other";
}

char* flv_avc_packet_type_name(uint8_t avc_packet_type)
{
    switch (avc_packet_type) {
        case 0:
            return "AVC sequence header";
        case 1:
            return "AVC NALU";
        case 2:
            return "AVC end of sequence";
        default:
            return "unknown";
    }

    return "unknown";
}
char* avc_nal_unit_type_name(uint8_t nal_type)
{
    switch (nal_type) {
        case AVC_NAL_UNIT_TYPE_SLICE_NONIDR:
            return "Coded slice of a non-IDR picture";
        case AVC_NAL_UNIT_TYPE_SLICE_PARTITION_A:
            return "Coded slice data partition A";
        case AVC_NAL_UNIT_TYPE_SLICE_PARTITION_B:
            return "Coded slice data partition B";
        case AVC_NAL_UNIT_TYPE_SLICE_PARTITION_C:
            return "Coded slice data partition C";
        case AVC_NAL_UNIT_TYPE_SLICE_IDR:
            return "Coded slice of an IDR picture";
        case AVC_NAL_UNIT_TYPE_SEI:
            return "Supplemental enhancement information (SEI)";
        case AVC_NAL_UNIT_TYPE_SPS:
            return "Sequence parameter set";
        case AVC_NAL_UNIT_TYPE_PPS:
            return "Picture parameter set";
        case AVC_NAL_UNIT_TYPE_AUD:
            return "Access unit delimiter";
        default:
            return "Unspecified";
    }

    return "Unspecified";
}
