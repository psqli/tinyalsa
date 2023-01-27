/* tinycap.c
 *
 * Copyright 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 **/

#include <tinyalsa/asoundlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <signal.h>
#include <string.h>
#include <limits.h>

#define OPTPARSE_IMPLEMENTATION
#include "optparse.h"

#define ID_RIFF 0x46464952
#define ID_WAVE 0x45564157
#define ID_FMT  0x20746d66
#define ID_DATA 0x61746164

#define FORMAT_PCM 1

struct wav_header {
    uint32_t riff_id;
    uint32_t riff_sz;
    uint32_t riff_fmt;
    uint32_t fmt_id;
    uint32_t fmt_sz;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    uint32_t data_id;
    uint32_t data_sz;
};

int capturing = 1;
int prinfo = 1;

unsigned int capture_sample(FILE *file, unsigned int card, unsigned int device,
                            bool use_mmap, unsigned int channels, unsigned int rate,
                            enum pcm_format format, unsigned int period_size,
                            unsigned int period_count, unsigned int capture_time);

void sigint_handler(int sig)
{
    if (sig == SIGINT){
        capturing = 0;
    }
}

int main(int argc, char **argv)
{
    FILE *file;
    struct wav_header header;
    unsigned int card = 0;
    unsigned int device = 0;
    unsigned int channels = 2;
    unsigned int rate = 48000;
    unsigned int bits = 16;
    unsigned int frames;
    unsigned int period_size = 1024;
    unsigned int period_count = 4;
    unsigned int capture_time = UINT_MAX;
    bool use_mmap = false;
    enum pcm_format format;
    int no_header = 0, c;
    struct optparse opts;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s {file.wav | --} [-D card] [-d device] [-M] [-c channels] "
                "[-r rate] [-b bits] [-p period_size] [-n n_periods] [-t time_in_seconds]\n\n"
                "Use -- for filename to send raw PCM to stdout\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1],"--") == 0) {
        file = stdout;
        prinfo = 0;
        no_header = 1;
    } else {
        file = fopen(argv[1], "wb");
        if (!file) {
            fprintf(stderr, "Unable to create file '%s'\n", argv[1]);
            return 1;
        }
    }

    /* parse command line arguments */
    optparse_init(&opts, argv + 1);
    while ((c = optparse(&opts, "D:d:c:r:b:p:n:t:M")) != -1) {
        switch (c) {
        case 'd':
            device = atoi(opts.optarg);
            break;
        case 'c':
            channels = atoi(opts.optarg);
            break;
        case 'r':
            rate = atoi(opts.optarg);
            break;
        case 'b':
            bits = atoi(opts.optarg);
            break;
        case 'D':
            card = atoi(opts.optarg);
            break;
        case 'p':
            period_size = atoi(opts.optarg);
            break;
        case 'n':
            period_count = atoi(opts.optarg);
            break;
        case 't':
            capture_time = atoi(opts.optarg);
            break;
        case 'M':
            use_mmap = true;
            break;
        case '?':
            fprintf(stderr, "%s\n", opts.errmsg);
            return EXIT_FAILURE;
        }
    }

    header.riff_id = ID_RIFF;
    header.riff_sz = 0;
    header.riff_fmt = ID_WAVE;
    header.fmt_id = ID_FMT;
    header.fmt_sz = 16;
    header.audio_format = FORMAT_PCM;
    header.num_channels = channels;
    header.sample_rate = rate;

    switch (bits) {
    case 32:
        format = PCM_FORMAT_S32_LE;
        break;
    case 24:
        format = PCM_FORMAT_S24_LE;
        break;
    case 16:
        format = PCM_FORMAT_S16_LE;
        break;
    default:
        fprintf(stderr, "%u bits is not supported.\n", bits);
        fclose(file);
        return 1;
    }

    header.bits_per_sample = pcm_format_to_bits(format);
    header.byte_rate = (header.bits_per_sample / 8) * channels * rate;
    header.block_align = channels * (header.bits_per_sample / 8);
    header.data_id = ID_DATA;

    /* leave enough room for header */
    if (!no_header) {
        fseek(file, sizeof(struct wav_header), SEEK_SET);
    }

    /* install signal handler and begin capturing */
    signal(SIGINT, sigint_handler);
    frames = capture_sample(file, card, device, use_mmap,
                            header.num_channels, header.sample_rate,
                            format, period_size, period_count, capture_time);
    if (prinfo) {
        printf("Captured %u frames\n", frames);
    }

    /* write header now all information is known */
    if (!no_header) {
        header.data_sz = frames * header.block_align;
        header.riff_sz = header.data_sz + sizeof(header) - 8;
        fseek(file, 0, SEEK_SET);
        fwrite(&header, sizeof(struct wav_header), 1, file);
    }

    fclose(file);

    return 0;
}

unsigned int capture_sample(FILE *file, unsigned int card, unsigned int device,
                            bool use_mmap, unsigned int channels, unsigned int rate,
                            enum pcm_format format, unsigned int period_size,
                            unsigned int period_count, unsigned int capture_time)
{
    struct pcm_config config;
    unsigned int pcm_open_flags;
    struct pcm *pcm;
    char *buffer;
    unsigned int size;
    unsigned int frames_read;
    unsigned int total_frames_read;
    unsigned int bytes_per_frame;

    memset(&config, 0, sizeof(config));
    config.channels = channels;
    config.rate = rate;
    config.period_size = period_size;
    config.period_count = period_count;
    config.format = format;
    config.start_threshold = 0;
    config.stop_threshold = 0;
    config.silence_threshold = 0;

    pcm_open_flags = PCM_IN;
    if (use_mmap)
        pcm_open_flags |= PCM_MMAP;

    pcm = pcm_open(card, device, pcm_open_flags, &config);
    if (!pcm || !pcm_is_ready(pcm)) {
        fprintf(stderr, "Unable to open PCM device (%s)\n",
                pcm_get_error(pcm));
        return 0;
    }

    size = pcm_frames_to_bytes(pcm, pcm_get_buffer_size(pcm));
    buffer = malloc(size);
    if (!buffer) {
        fprintf(stderr, "Unable to allocate %u bytes\n", size);
        pcm_close(pcm);
        return 0;
    }

    if (prinfo) {
        printf("Capturing sample: %u ch, %u hz, %u bit\n", channels, rate,
           pcm_format_to_bits(format));
    }

    bytes_per_frame = pcm_frames_to_bytes(pcm, 1);
    total_frames_read = 0;
    frames_read = 0;
    while (capturing) {
        frames_read = pcm_readi(pcm, buffer, pcm_get_buffer_size(pcm));
        total_frames_read += frames_read;
        if ((total_frames_read / rate) >= capture_time) {
            capturing = 0;
        }
        if (fwrite(buffer, bytes_per_frame, frames_read, file) != frames_read) {
            fprintf(stderr,"Error capturing sample\n");
            break;
        }
    }

    free(buffer);
    pcm_close(pcm);
    return total_frames_read;
}
