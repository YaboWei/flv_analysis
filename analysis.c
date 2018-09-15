#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "analysis.h"
#include "utils.h"
#include "bytesread.h"

int iflv_fd = 0; // input flv file
int oanls_fd = 1; // output analysis file, default stdout

static int analysis_header()
{
    uint8_t flv_header[FLV_HEADER_SIZE + FLV_TAGSIZE_SIZE];
    if (read(iflv_fd, flv_header, FLV_HEADER_SIZE + FLV_TAGSIZE_SIZE) != FLV_HEADER_SIZE + FLV_TAGSIZE_SIZE) {
        fprintf(stderr, "read flv header failed: %s(%d)\n", strerror(errno), errno);
        return errno;
    }

    if (memcmp(flv_header, "FLV", 3)) {
        fprintf(stderr, "signature 'FLV' not found, found %c%c%c\n", flv_header[0], flv_header[1], flv_header[2]);
        return -1;
    }

    int audio_present = flv_header[4] & 0x04;
    int video_present = flv_header[4] & 0x01;

    char header_desc[100] = {0};
    snprintf(header_desc, sizeof(header_desc),
            "FLV header:\n    FLV version %u, %s %s\n",
            flv_header[3], audio_present ? "has audio" : "no audio",
            video_present ? "has video" : "no video"
            );

    write(oanls_fd, header_desc, strlen(header_desc));

    return 0;
}

static int analysis_audio_tag_data(const uint8_t* tag_data, uint32_t tag_size)
{
    uint8_t SoundFormat = (tag_data[0] & 0xf0) >> 4;
    uint8_t SoundRate   = (tag_data[0] & 0x0c) >> 2;
    uint8_t SoundSize   = (tag_data[0] & 0x02) >> 1;
    uint8_t SoundType   = tag_data[0] & 0x01;

    uint8_t AACPacketType = tag_data[1];

    char tag_header_desc[100];
    snprintf(tag_header_desc, sizeof(tag_header_desc),
            "AudioTagHeader:\n    %s(%u), %s(%u), %s(%u), %s(%u), %s(%u)\n",
            flv_sound_format_name(SoundFormat), SoundFormat,
            flv_sound_rate_name(SoundRate), SoundRate,
            flv_sound_size_name(SoundSize), SoundSize,
            flv_sound_type_name(SoundType), SoundType,
            SoundFormat == 10 ? flv_aac_packet_type_name(SoundType) : "unused", SoundType);

    write(oanls_fd, tag_header_desc, strlen(tag_header_desc));

    return 0;
}

static int analysis_video_tag_data(const uint8_t* tag_data, uint32_t tag_size)
{
    int header_size = 0;
    uint8_t FrameType   = (tag_data[header_size] & 0xf0) >> 4;
    uint8_t CodecID     = tag_data[header_size++] & 0x0f;

    uint8_t AVCPacketType = -1;
    int32_t CompositionTime = -1;
    if (CodecID == FLV_VIDEO_CODEC_AVC) {
        AVCPacketType = tag_data[header_size++];
        CompositionTime = (tag_data[header_size] << 16) + (tag_data[header_size+1] << 8) + tag_data[header_size+2];
        header_size += 3;
    }

    char tag_header_desc[100];
    snprintf(tag_header_desc, sizeof(tag_header_desc),
            "VideoTagHeader:\n    %s(%u), %s(%u), %s(%u), %u\n",
            flv_frame_type_name(FrameType), FrameType,
            flv_codec_name(CodecID), CodecID,
            CodecID == FLV_VIDEO_CODEC_AVC ? flv_avc_packet_type_name(AVCPacketType) : "unused", AVCPacketType,
            CompositionTime);

    write(oanls_fd, tag_header_desc, strlen(tag_header_desc));

    if (CodecID != FLV_VIDEO_CODEC_AVC) {
        return 0;
    }


#define NALU_DATA_MAX_DUMP_SIZE 32
    int index = header_size;
    if (AVCPacketType == 0) {
        //AVCDecoderConfigurationRecord
        write(oanls_fd, "sequence header:\n", strlen("sequence header:\n"));
        int dump_size = (tag_size - header_size) < NALU_DATA_MAX_DUMP_SIZE ? (tag_size - header_size) : NALU_DATA_MAX_DUMP_SIZE;
        char nal_desc[1024] = {0};
        snprintf(nal_desc + strlen(nal_desc), sizeof(nal_desc) - strlen(nal_desc), "    ");
        for (int i = 0; i < dump_size; i++) {
            snprintf(nal_desc + strlen(nal_desc), sizeof(nal_desc) - strlen(nal_desc), "%2x%c",
                    tag_data[index+i], i < dump_size-1 ? ' ' : '\n');
        }
        write(oanls_fd, nal_desc, strlen(nal_desc));
        return 0;
    }

    write(oanls_fd, "nals:\n", strlen("nals:\n"));
    // One or more NALUs (Full frames are required)
    while (index < tag_size - header_size) {
        uint32_t nal_size = (tag_data[index] << 24) + (tag_data[index+1] << 16) + (tag_data[index+2] << 8) + tag_data[index+3];
        uint8_t nal_type = tag_data[index+4] & 0x1f;
        index += 4;
        char nal_desc[1024] = {0};
        snprintf(nal_desc + strlen(nal_desc), sizeof(nal_desc) - strlen(nal_desc), "    tp:%u,size:%u,",
                nal_type, nal_size);

        snprintf(nal_desc + strlen(nal_desc), sizeof(nal_desc) - strlen(nal_desc), "data:");
        int dump_size = nal_size < NALU_DATA_MAX_DUMP_SIZE ? nal_size : NALU_DATA_MAX_DUMP_SIZE;
        for (int i = 1; i < dump_size; i++) {
            snprintf(nal_desc + strlen(nal_desc), sizeof(nal_desc) - strlen(nal_desc), "%2x%c",
                    tag_data[index+i], i < dump_size-1 ? ' ' : '\n');
        }
        write(oanls_fd, nal_desc, strlen(nal_desc));
        index += nal_size;
    }

    return 0;
}

static int analysis_data_number(uint8_t** data)
{
    char number[32] = {0};
    snprintf(number, sizeof(number), "%lf", read_8bytes_to_double(*data));
    write(oanls_fd, number, strlen(number));

    *data += 8;
    return 0;
}

static int analysis_data_boolean(uint8_t** data)
{
    uint8_t b = **data + '0';
    *data += 1;
    write(oanls_fd, &b, 1);
    return 0;
}

static int analysis_data_string(uint8_t** data)
{
    uint32_t pro_name_len = read_2bytes_to_uint32(*data);
    *data += 2;
    write(oanls_fd, *data, pro_name_len);
    *data += pro_name_len;
    return 0;
}

static int analysis_data_longstring(uint8_t** data)
{
    uint32_t pro_name_len = (**data << 24) | (**(data+1) << 16) | (**(data+2) << 8) | **(data+3);
    *data += 4;
    write(oanls_fd, *data, pro_name_len);
    *data += pro_name_len;
    return 0;
}

static int analysis_data_ecma(uint8_t* ecma_data, uint32_t ecma_len)
{
    int ecma_id = 0;
    char break_msg[32] = {0};
    for (; ecma_id < ecma_len; ecma_id++) {
        uint32_t pro_name_len = (*ecma_data << 8) | *(ecma_data+1);
        ecma_data += 2;
        write(oanls_fd, ecma_data, pro_name_len);
        write(oanls_fd, ": ", 2);
        ecma_data += pro_name_len;

        uint8_t pro_value_tp = *(ecma_data++);

        if (pro_value_tp == FLV_DATA_VALUE_TYPE_NUMBER) {
            analysis_data_number(&ecma_data);
        } else if (pro_value_tp == FLV_DATA_VALUE_TYPE_BOOLEAN) {
            analysis_data_boolean(&ecma_data);
        } else if (pro_value_tp == FLV_DATA_VALUE_TYPE_LONGSTRING) {
            analysis_data_longstring(&ecma_data);
        } else if (pro_value_tp == FLV_DATA_VALUE_TYPE_STRING) {
            analysis_data_string(&ecma_data);
        } else {
            snprintf(break_msg, sizeof(break_msg),
                    BREAK_UNSUPPORTTED_DATA_TYPE, pro_value_tp);
            break;
        }
        write(oanls_fd, "\n", 1);
    }

    return 0;
}

static int analysis_data_tag_data(uint8_t* tag_data, uint32_t tag_size)
{
    int i = 0;
    uint8_t name_tp = *(tag_data++);
    char name[32] = {0};
    if (name_tp != 2)
        return 0;

    uint32_t str_len = (*tag_data << 8) | *(tag_data+1);
    tag_data += 2;
    memcpy(name, tag_data, str_len);
    tag_data += str_len;
    write(oanls_fd, name, strlen(name));

    uint8_t value_tp = *(tag_data++);
    if (value_tp == FLV_DATA_VALUE_TYPE_ECMA) {
        uint32_t ecma_len = (*tag_data << 24) | (*(tag_data+1) << 16) | (*(tag_data+2) << 8) | *(tag_data+3);
        tag_data += 4;
        analysis_data_ecma(tag_data, ecma_len);
    }

    return 0;
}

static int analysis_tag()
{
    int size = 0;
    write(oanls_fd, SPLIT_LINE, strlen(SPLIT_LINE));
    int32_t pos = lseek(iflv_fd, 0, SEEK_CUR);
    if (pos == -1) {
        fprintf(stderr, "query flv current position failed: %s(%d)\n", strerror(errno), errno);
        return errno;
    }

    uint8_t tag_header[FLV_TAG_HEADER_SIZE];
    if ((size = read(iflv_fd, tag_header, FLV_TAG_HEADER_SIZE)) != FLV_TAG_HEADER_SIZE) {
        if (errno != 0) {
            fprintf(stderr, "read flv tag header failed: %s(%d), wants:%d, got:%d\n",
                strerror(errno), errno, FLV_TAG_HEADER_SIZE, size);
            return errno;
        }
        return ERR_INPUT_EOF;
    }

    uint8_t tag_type = tag_header[0] & 0x1f;
    uint32_t tag_size = read_3bytes_to_uint32(&tag_header[1]);;
    int64_t tag_ts = read_3bytes_to_uint32(&tag_header[4]);

    uint8_t* tag_data = (uint8_t*)malloc(tag_size);
    if (tag_data == NULL) {
        fprintf(stderr, "malloc tag data failed: %s(%d)\n", strerror(errno), errno);
        return errno;
    }

    if (read(iflv_fd, tag_data, tag_size) != tag_size) {
        if (errno != 0) {
            fprintf(stderr, "read flv tag data failed: %s(%d), tag_size:%u\n", strerror(errno), errno, tag_size);
            return errno;
        }
        return ERR_INPUT_EOF;
    }

    char tag_header_desc[100] = {0};
    snprintf(tag_header_desc, sizeof(tag_header_desc),
            "FLV Tag:\n    type:%s, size:%u, ts: %lld, rpos:0x%x(%u)\n",
            tag_type_name(tag_type), tag_size, tag_ts, (uint32_t)pos, pos);
    write(oanls_fd, tag_header_desc, strlen(tag_header_desc));

    if (tag_type == FLV_TAG_TYPE_AUDIO)
        analysis_audio_tag_data(tag_data, tag_size);
    if (tag_type == FLV_TAG_TYPE_VIDEO)
        analysis_video_tag_data(tag_data, tag_size);
    if (tag_type == FLV_TAG_TYPE_DATA)
        analysis_data_tag_data(tag_data, tag_size);

    uint32_t pre_tag_size = 0;
    uint8_t pre_tag_size_data[4] = {0};
    char pre_tag_size_str[100] = {0};
    if ((size = read(iflv_fd, pre_tag_size_data, FLV_TAGSIZE_SIZE)) != FLV_TAGSIZE_SIZE) {
        fprintf(stderr, "read flv pre tag size failed: %s(%d), wants:%d, got:%d\n", strerror(errno), errno, FLV_TAGSIZE_SIZE, size);
        return errno;
    }
    pre_tag_size = read_4bytes_to_uint32(pre_tag_size_data);

    snprintf(pre_tag_size_str, sizeof(pre_tag_size_str), "PreviousTagSize:0x%x(%u)\n", pre_tag_size, pre_tag_size);
    write(oanls_fd, pre_tag_size_str, strlen(pre_tag_size_str));

    return 0;
}

int analysis()
{
    int ret = 0;
    if ((ret = analysis_header()) < 0) {
        fprintf(stderr, "analysis flv header failed\n");
        return ret;
    }

    while (1) {
        if ((ret = analysis_tag()) != 0) {
            if (ret == ERR_INPUT_EOF) {
                ret = 0;
                break;
            }
            fprintf(stderr, "analysis flv tag failed!!!\n");
            return ret;
        }
    }

    return ret;
}
