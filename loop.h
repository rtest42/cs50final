#ifndef LOOP_H
#define LOOP_H

// Detailed header for WAVE files
// 44 bytes long
typedef struct wav_header
{
    unsigned char chunk_id[4];
    unsigned int chunk_size;
    unsigned char format[4];

    unsigned char subchunk1_id[4];
    unsigned int subchunk1_size;
    unsigned short int audio_format;
    unsigned short int num_channels;
    unsigned int sample_rate;
    unsigned int byte_rate;
    unsigned short int block_align;
    unsigned short int bits_per_sample;

    unsigned char subchunk2_id[4];
    unsigned int subchunk2_size;
}
wav_header;

#endif
