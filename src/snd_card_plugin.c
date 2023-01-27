/* snd_card_plugin.c
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

#include "snd_card_plugin.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#define SND_DLSYM(h, p, s, err) \
do {                            \
    err = 0;                    \
    p = dlsym(h, s);            \
    if (!p)                        \
        err = -ENODEV;            \
} while(0)

int snd_utils_get_int(struct snd_node *node, const char *prop, int *val)
{
    if (!node || !node->card_node || !node->dev_node)
        return SND_NODE_TYPE_HW;

    return node->ops->get_int(node->dev_node, prop, val);
}

int snd_utils_get_str(struct snd_node *node, const char *prop, char **val)
{
    if (!node || !node->card_node || !node->dev_node)
        return SND_NODE_TYPE_HW;

    return node->ops->get_str(node->dev_node, prop, val);
}

void snd_utils_close_dev_node(struct snd_node *node)
{
    if (!node)
        return;

    if (node->card_node)
        node->ops->close_card(node->card_node);

    if (node->dl_hdl)
        dlclose(node->dl_hdl);

    free(node);
}

enum snd_node_type snd_utils_get_node_type(struct snd_node *node)
{
    int val = SND_NODE_TYPE_HW;

    if (!node || !node->card_node || !node->dev_node)
        return SND_NODE_TYPE_HW;

    node->ops->get_int(node->dev_node, "type", &val);

    return val;
}

static int snd_utils_resolve_symbols(struct snd_node *node)
{
    void *dl = node->dl_hdl;
    int err;
    SND_DLSYM(dl, node->ops, "snd_card_ops", err);
    return err;
}

static struct snd_node *snd_utils_open_dev_node(unsigned int card,
                                                unsigned int device,
                                                int dev_type)
{
    struct snd_node *node;
    int rc = 0;

    node = calloc(1, sizeof(*node));
    if (!node)
        return NULL;

    node->dl_hdl = dlopen("libsndcardparser.so", RTLD_NOW);
    if (!node->dl_hdl) {
        goto err_dl_open;
    }

    rc = snd_utils_resolve_symbols(node);
    if (rc < 0)
        goto err_resolve_symbols;

    node->card_node = node->ops->open_card(card);
    if (!node->card_node)
        goto err_resolve_symbols;

    if (dev_type == NODE_PCM) {
      node->dev_node = node->ops->get_pcm(node->card_node, device);
    } else {
      node->dev_node = node->ops->get_mixer(node->card_node);
    }

    if (!node->dev_node)
        goto err_get_node;

    return node;

err_get_node:
    node->ops->close_card(node->card_node);

err_resolve_symbols:
    dlclose(node->dl_hdl);

err_dl_open:
    free(node);
    return NULL;
}

struct snd_node* snd_utils_open_pcm(unsigned int card,
                                    unsigned int device)
{
  return snd_utils_open_dev_node(card, device, NODE_PCM);
}

struct snd_node* snd_utils_open_mixer(unsigned int card)
{
  return snd_utils_open_dev_node(card, 0, NODE_MIXER);
}
