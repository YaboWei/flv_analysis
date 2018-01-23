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
            "FLV version %u, %s %s\n",
            flv_header[3], audio_present ? "has audio" : "no audio",
            video_present ? "has video" : "no video"
            );

    write(oanls_fd, header_desc, strlen(header_desc));

    return 0;
}

static int analysis_audio_tag_header(const uint8_t* tag_header)
{
    uint8_t SoundFormat = (tag_header[0] & 0xf0) >> 4;
    uint8_t SoundRate   = (tag_header[0] & 0x0c) >> 2;
    uint8_t SoundSize   = (tag_header[0] & 0x02) >> 1;
    uint8_t SoundType   = tag_header[0] & 0x01;

    uint8_t AACPacketType = tag_header[1];

    char tag_header_desc[100];
    snprintf(tag_header_desc, sizeof(tag_header_desc),
            "    %s(%u), %s(%u), %s(%u), %s(%u), %s(%u)\n",
            flv_sound_format_name(SoundFormat), SoundFormat,
            flv_sound_rate_name(SoundRate), SoundRate,
            flv_sound_size_name(SoundSize), SoundSize,
            flv_sound_type_name(SoundType), SoundType,
            SoundFormat == 10 ? flv_aac_packet_type_name(SoundType) : "unused", SoundType);

    write(oanls_fd, tag_header_desc, strlen(tag_header_desc));

    return 2;
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
            "    %s(%u), %s(%u), %s(%u), %u\n",
            flv_frame_type_name(FrameType), FrameType,
            flv_codec_name(CodecID), CodecID,
            CodecID == FLV_VIDEO_CODEC_AVC ? flv_avc_packet_type_name(AVCPacketType) : "unused", AVCPacketType,
            CompositionTime);

    write(oanls_fd, tag_header_desc, strlen(tag_header_desc));

    if (CodecID != FLV_VIDEO_CODEC_AVC || AVCPacketType != 1) {
        return 0;
    }

    write(oanls_fd, "    data:\n", strlen("    data:\n"));
    int index = header_size;
    while (index < tag_size - header_size) {
        uint32_t nal_size = (tag_data[index] << 24) + (tag_data[index+1] << 16) + (tag_data[index+2] << 8) + tag_data[index+3];
        uint8_t nal_type = tag_data[index+4] & 0x1f;
        index += nal_size + 4;
        char nal_desc[100] = {0};
        snprintf(nal_desc, sizeof(nal_desc), "        nal_type:%s(%u), nal_size:%u\n",
                avc_nal_unit_type_name(nal_type), nal_type, nal_size);
        write(oanls_fd, nal_desc, strlen(nal_desc));
    }

    return 0;
}

static int analysis_audio_tag_data(const uint8_t* tag_data)
{
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
        fprintf(stderr, "read flv tag header failed: %s(%d), wants:%d, got:%d\n",
                strerror(errno), errno, FLV_TAG_HEADER_SIZE, size);
        return -1;
    }

    uint8_t tag_type = tag_header[0] & 0x1f;
    uint32_t tag_size = (tag_header[1] << 16) + (tag_header[2] << 8) + (tag_header[3]);

    uint8_t* tag_data = (uint8_t*)malloc(tag_size);
    if (tag_data == NULL) {
        fprintf(stderr, "malloc tag data failed: %s(%d)\n", strerror(errno), errno);
        return errno;
    }

    if (read(iflv_fd, tag_data, tag_size) != tag_size) {
        fprintf(stderr, "read flv tag data failed: %s(%d), tag_size:%u\n", strerror(errno), errno, tag_size);
        return errno;
    }

    char tag_header_desc[100] = {0};
    snprintf(tag_header_desc, sizeof(tag_header_desc),
            "header:\n    type:%s, size:%u, pos:0x%x(%u)\n",
            tag_type_name(tag_type), tag_size, (uint32_t)pos, pos);
    write(oanls_fd, tag_header_desc, strlen(tag_header_desc));

    int tag_head_size = 0;
    if (tag_type == FLV_TAG_TYPE_AUDIO) {
        tag_head_size = analysis_audio_tag_header(tag_data);
        analysis_audio_tag_data(tag_data+tag_head_size);
    } else if (tag_type == FLV_TAG_TYPE_VIDEO) {
        analysis_video_tag_data(tag_data, tag_size);
    }

    uint32_t pre_tag_size;
    if (read(iflv_fd, &pre_tag_size, FLV_TAGSIZE_SIZE) != FLV_TAGSIZE_SIZE) {
        fprintf(stderr, "read flv pre tag size failed: %s(%d)\n", strerror(errno), errno);
        return errno;
    }

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
        if ((ret = analysis_tag()) < 0) {
            fprintf(stderr, "analysis flv tag failed\n");
            return ret;
        }
    }

    return ret;
}
