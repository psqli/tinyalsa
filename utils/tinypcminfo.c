/* tinypcminfo.c
 *
 * Copyright 2012 The Android Open Source Project
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
#include <string.h>

#define OPTPARSE_IMPLEMENTATION
#include "optparse.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))
#endif

/* The format_lookup is in order of SNDRV_PCM_FORMAT_##index and
 * matches the grouping in sound/asound.h.  Note this is not
 * continuous and has an empty gap from (25 - 30).
 */
static const char *format_lookup[] = {
        /*[0] =*/ "S8",
        "U8",
        "S16_LE",
        "S16_BE",
        "U16_LE",
        "U16_BE",
        "S24_LE",
        "S24_BE",
        "U24_LE",
        "U24_BE",
        "S32_LE",
        "S32_BE",
        "U32_LE",
        "U32_BE",
        "FLOAT_LE",
        "FLOAT_BE",
        "FLOAT64_LE",
        "FLOAT64_BE",
        "IEC958_SUBFRAME_LE",
        "IEC958_SUBFRAME_BE",
        "MU_LAW",
        "A_LAW",
        "IMA_ADPCM",
        "MPEG",
        /*[24] =*/ "GSM",
        [31] = "SPECIAL",
        "S24_3LE",
        "S24_3BE",
        "U24_3LE",
        "U24_3BE",
        "S20_3LE",
        "S20_3BE",
        "U20_3LE",
        "U20_3BE",
        "S18_3LE",
        "S18_3BE",
        "U18_3LE",
        /*[43] =*/ "U18_3BE",
#if 0
        /* recent additions, may not be present on local asound.h */
        "G723_24",
        "G723_24_1B",
        "G723_40",
        "G723_40_1B",
        "DSD_U8",
        "DSD_U16_LE",
#endif
};

/* Returns a human readable name for the format associated with bit_index,
 * NULL if bit_index is not known.
 */
static inline const char *pcm_get_format_name(unsigned bit_index)
{
    return bit_index < ARRAY_SIZE(format_lookup) ? format_lookup[bit_index] : NULL;
}

int main(int argc, char **argv)
{
    unsigned int device = 0;
    unsigned int card = 0;
    int i;
    struct optparse opts;
    struct optparse_long long_options[] = {
        { "help",   'h', OPTPARSE_NONE     },
        { "card",   'D', OPTPARSE_REQUIRED },
        { "device", 'd', OPTPARSE_REQUIRED },
        { 0, 0, 0 }
    };

    (void)argc; /* silence -Wunused-parameter */
    /* parse command line arguments */
    optparse_init(&opts, argv);
    while ((i = optparse_long(&opts, long_options, NULL)) != -1) {
        switch (i) {
        case 'D':
            card = atoi(opts.optarg);
            break;
        case 'd':
            device = atoi(opts.optarg);
            break;
        case 'h':
            fprintf(stderr, "Usage: %s -D card -d device\n", argv[0]);
            return 0;
        case '?':
            fprintf(stderr, "%s\n", opts.errmsg);
            return EXIT_FAILURE;
        }
    }

    printf("Info for card %u, device %u:\n", card, device);

    for (i = 0; i < 2; i++) {
        struct pcm_params *params;
        const struct pcm_mask *m;
        unsigned int min;
        unsigned int max;

        printf("\nPCM %s:\n", i == 0 ? "out" : "in");

        params = pcm_params_get(card, device, i == 0 ? PCM_OUT : PCM_IN);
        if (params == NULL) {
            printf("Device does not exist.\n");
            continue;
        }

        m = pcm_params_get_mask(params, PCM_PARAM_ACCESS);
        if (m) { /* bitmask, refer to SNDRV_PCM_ACCESS_*, generally interleaved */
            printf("      Access:\t%#08x\n", m->bits[0]);
        }
        m = pcm_params_get_mask(params, PCM_PARAM_FORMAT);
        if (m) { /* bitmask, refer to: SNDRV_PCM_FORMAT_* */
            unsigned j, k, count = 0;
            const unsigned bitcount = sizeof(m->bits[0]) * 8;

            /* we only check first two format masks (out of 8) - others are zero. */
            printf("   Format[0]:\t%#08x\n", m->bits[0]);
            printf("   Format[1]:\t%#08x\n", m->bits[1]);

            /* print friendly format names, if they exist */
            for (k = 0; k < 2; ++k) {
                for (j = 0; j < bitcount; ++j) {
                    const char *name;

                    if (m->bits[k] & (1 << j)) {
                        name = pcm_get_format_name(j + k*bitcount);
                        if (name) {
                            if (count++ == 0) {
                                printf(" Format Name:\t");
                            } else {
                                printf (", ");
                            }
                            printf("%s", name);
                        }
                    }
                }
            }
            if (count) {
                printf("\n");
            }
        }
        m = pcm_params_get_mask(params, PCM_PARAM_SUBFORMAT);
        if (m) { /* bitmask, should be 1: SNDRV_PCM_SUBFORMAT_STD */
            printf("   Subformat:\t%#08x\n", m->bits[0]);
        }
        min = pcm_params_get_min(params, PCM_PARAM_RATE);
        max = pcm_params_get_max(params, PCM_PARAM_RATE);
        printf("        Rate:\tmin=%uHz\tmax=%uHz\n", min, max);
        min = pcm_params_get_min(params, PCM_PARAM_CHANNELS);
        max = pcm_params_get_max(params, PCM_PARAM_CHANNELS);
        printf("    Channels:\tmin=%u\t\tmax=%u\n", min, max);
        min = pcm_params_get_min(params, PCM_PARAM_SAMPLE_BITS);
        max = pcm_params_get_max(params, PCM_PARAM_SAMPLE_BITS);
        printf(" Sample bits:\tmin=%u\t\tmax=%u\n", min, max);
        min = pcm_params_get_min(params, PCM_PARAM_PERIOD_SIZE);
        max = pcm_params_get_max(params, PCM_PARAM_PERIOD_SIZE);
        printf(" Period size:\tmin=%u\t\tmax=%u\n", min, max);
        min = pcm_params_get_min(params, PCM_PARAM_PERIODS);
        max = pcm_params_get_max(params, PCM_PARAM_PERIODS);
        printf("Period count:\tmin=%u\t\tmax=%u\n", min, max);

        pcm_params_free(params);
    }

    return 0;
}
