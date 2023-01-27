/* sample_sndcardparser.c
 *
 * Copyright 2021 The Linux Foundation
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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#define VIRTUAL_SND_CARD_ID 100
#define MAX_PATH 256
#define BUF_SIZE 1024

enum snd_node_type {
    NODE_TYPE_HW = 0,
    NODE_TYPE_PLUGIN,
    NODE_TYPE_INVALID,
};

enum {
    NODE_PCM,
    NODE_COMPR,
    NODE_MIXER,
    NODE_MAX,
};

struct snd_node_ops {
    /** Function pointer to get card definition */
    void* (*open_card)(unsigned int card);
    /** Function pointer to release card definition */
    void (*close_card)(void *card);
    /** Get interger type properties from device definition */
    int (*get_int)(void *node, const char *prop, int *val);
    /** Get string type properties from device definition */
    int (*get_str)(void *node, const char *prop, char **val);
    /** Function pointer to get mixer definition */
    void* (*get_mixer)(void *card);
    /** Function pointer to get PCM definition */
    void* (*get_pcm)(void *card, unsigned int id);
    /** Function pointer to get COMPRESS definition */
    void* (*get_compress)(void *card, unsigned int id);
};

struct snd_dev_def {
    unsigned int device;
    int type;
    const char *name;
    const char *so_name;
    int playback; //used only for pcm node
    int capture;  //used only for pcm node
    /* add custom props here */
};

struct snd_dev_def_card {
    unsigned int card;
    char *name;

    /* child device details */
    int num_pcm_nodes;
    struct snd_dev_def *pcm_dev_def;

    struct snd_dev_def *mixer_dev_def;
};

struct snd_dev_def pcm_devs[] = {
    {100, NODE_TYPE_PLUGIN, "PCM100", "libtinyalsav2_example_plugin_pcm.so", 1, 0},
    /* Add other plugin info here */
};

struct snd_dev_def mixer_dev =
    {VIRTUAL_SND_CARD_ID, NODE_TYPE_PLUGIN, "virtual-snd-card", "libtinyalsav2_example_plugin_mixer.so", 0, 0};

void *snd_card_def_open_card(unsigned int card)
{
    struct snd_dev_def_card *card_def = NULL;
    struct snd_dev_def *pcm_dev_def = NULL;
    struct snd_dev_def *mixer_dev_def = NULL;
    int num_pcm = ARRAY_SIZE(pcm_devs);
    int i;

    if (card != VIRTUAL_SND_CARD_ID)
        return NULL;

    card_def = calloc(1, sizeof(struct snd_dev_def_card));
    if (!card_def)
        return card_def;

    card_def->card = card;
    card_def->name = strdup("virtual-snd-card");

    /* fill pcm device node info */
    card_def->num_pcm_nodes = num_pcm;
    pcm_dev_def = calloc(num_pcm, sizeof(struct snd_dev_def));
    if (!pcm_dev_def)
        goto free_card_def;

    for (i = 0; i < num_pcm; i++)
        memcpy(&pcm_dev_def[i], &pcm_devs[i], sizeof(struct snd_dev_def));

    card_def->pcm_dev_def = pcm_dev_def;

    /* fill mixer device node info */
    mixer_dev_def = calloc(1, sizeof(struct snd_dev_def));
    if (!mixer_dev_def)
        goto free_pcm_dev;

    memcpy(mixer_dev_def, &mixer_dev, sizeof(struct snd_dev_def));

    card_def->mixer_dev_def = mixer_dev_def;
    return card_def;
free_pcm_dev:
    free(pcm_dev_def);
free_card_def:
    free(card_def->name);
    free(card_def);
    return NULL;
}

void snd_card_def_close_card(void *card_node)
{
    struct snd_dev_def_card *defs = (struct snd_dev_def_card *)card_node;
    struct snd_dev_def *pcm_dev_def = NULL;
    struct snd_dev_def *mixer_dev_def = NULL;

    if (!defs)
        return;

    pcm_dev_def = defs->pcm_dev_def;
    if (pcm_dev_def)
        free(pcm_dev_def);

    mixer_dev_def = defs->mixer_dev_def;
    if (!mixer_dev_def)
         goto free_defs;

    free(mixer_dev_def);
free_defs:
    free(defs->name);
    free(defs);
}

void *snd_card_def_get_node(void *card_node, unsigned int id, int type)
{
    struct snd_dev_def_card *card_def = (struct snd_dev_def_card *)card_node;
    struct snd_dev_def *dev_def = NULL;
    int i;

    if (!card_def)
        return NULL;

    if (type >= NODE_MAX)
        return NULL;

    if (type == NODE_MIXER)
        return card_def->mixer_dev_def;

    if (type == NODE_PCM)
        dev_def = card_def->pcm_dev_def;

    for (i = 0; i < card_def->num_pcm_nodes; i++) {
        if (dev_def[i].device == id) {
            return &dev_def[i];
        }
    }

    return NULL;
}

int snd_card_def_get_int(void *node, const char *prop, int *val)
{
    struct snd_dev_def *dev_def = (struct snd_dev_def *)node;
    int ret = -EINVAL;

    if (!dev_def || !prop || !val)
        return ret;

    if (!strcmp(prop, "type")) {
        *val = dev_def->type;
        return 0;
    } else if (!strcmp(prop, "id")) {
        *val = dev_def->device;
        return 0;
    } else if (!strcmp(prop, "playback")) {
        *val = dev_def->playback;
        return 0;
    } else if (!strcmp(prop, "capture")) {
        *val = dev_def->capture;
        return 0;
    }

    return ret;
}

int snd_card_def_get_str(void *node, const char *prop, char **val)
{
    struct snd_dev_def *dev_def = (struct snd_dev_def *)node;
    int ret = -EINVAL;

    if (!dev_def || !prop)
        return ret;

    if (!strcmp(prop, "so-name")) {
        if (dev_def->so_name) {
            *val = (char *)dev_def->so_name;
            return 0;
        }
    }

    if (!strcmp(prop, "name")) {
        if (dev_def->name) {
            *val = (char *)dev_def->name;
            return 0;
        }
    }

    return ret;
}

void *snd_card_def_get_pcm(void *card_node, unsigned int id)
{
    return snd_card_def_get_node(card_node, id, NODE_PCM);
}

void *snd_card_def_get_compress(void *card_node, unsigned int id)
{
    return snd_card_def_get_node(card_node, id, NODE_COMPR);
}

void *snd_card_def_get_mixer(void *card_node)
{
    return snd_card_def_get_node(card_node, 1, NODE_MIXER);
}

struct snd_node_ops snd_card_ops = {
    .open_card = snd_card_def_open_card,
    .close_card = snd_card_def_close_card,
    .get_int = snd_card_def_get_int,
    .get_str = snd_card_def_get_str,
    .get_pcm = snd_card_def_get_pcm,
    .get_compress = snd_card_def_get_compress,
    .get_mixer = snd_card_def_get_mixer,
};
