/* pcm_io.h
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

#ifndef TINYALSA_SRC_PCM_IO_H
#define TINYALSA_SRC_PCM_IO_H

#include <poll.h>
#include <sound/asound.h>

struct snd_node;

struct pcm_ops {
    int (*open) (unsigned int card, unsigned int device,
                 unsigned int flags, void **data, struct snd_node *node);
    void (*close) (void *data);
    int (*ioctl) (void *data, unsigned int cmd, ...);
    void *(*mmap) (void *data, void *addr, size_t length, int prot, int flags,
                   off_t offset);
    int (*munmap) (void *data, void *addr, size_t length);
    int (*poll) (void *data, struct pollfd *pfd, nfds_t nfds, int timeout);
};

extern const struct pcm_ops hw_ops;
extern const struct pcm_ops plug_ops;

#endif /* TINYALSA_SRC_PCM_IO_H */
