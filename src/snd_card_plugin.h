/* snd_utils.h
 *
 * Copyright 2019 The Linux Foundation
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

#ifndef TINYALSA_SRC_SND_CARD_UTILS_H
#define TINYALSA_SRC_SND_CARD_UTILS_H

#include <tinyalsa/plugin.h>

#include <dlfcn.h>

/** Encapsulates the pcm device definition from
 * the sound card definition configuration file.
 */
struct snd_node {
    /** Pointer the card definition */
    void *card_node;
    /** Pointer to device definition, either PCM or MIXER device */
    void *dev_node;
    /** Pointer to the sound card parser library */
    void *dl_hdl;
    /** A pointer to the operations structure. */
    const struct snd_node_ops* ops;
};

enum snd_node_type {
    SND_NODE_TYPE_HW = 0,
    SND_NODE_TYPE_PLUGIN,
    SND_NODE_TYPE_INVALID,
};

enum {
  NODE_PCM,
  NODE_MIXER
};

struct snd_node *snd_utils_open_pcm(unsigned int card, unsigned int device);

struct snd_node *snd_utils_open_mixer(unsigned int card);

void snd_utils_close_dev_node(struct snd_node *node);

enum snd_node_type snd_utils_get_node_type(struct snd_node *node);

int snd_utils_get_int(struct snd_node *node, const char *prop, int *val);

int snd_utils_get_str(struct snd_node *node, const char *prop, char **val);

#endif /* end of TINYALSA_SRC_SND_CARD_UTILS_H */
