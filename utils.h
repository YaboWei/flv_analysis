#ifndef FLV_ANALYSIS_UTILS_H
#define FLV_ANALYSIS_UTILS_H

char* tag_type_name(uint8_t tag_type);
char* flv_sound_format_name(uint8_t sound_format);
char* flv_sound_rate_name(uint8_t sound_rate);
char* flv_sound_size_name(uint8_t sound_size);
char* flv_sound_type_name(uint8_t sound_type);
char* flv_aac_packet_type_name(uint8_t aac_packet_type);
char* flv_frame_type_name(uint8_t frame_type);
char* flv_codec_name(uint8_t codec_id);
char* flv_avc_packet_type_name(uint8_t avc_packet_type);
char* avc_nal_unit_type_name(uint8_t nal_type);

#endif //FLV_ANALYSIS_UTILS_H
