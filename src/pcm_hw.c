/* pcm_hw.c
 *
 * Copyright 2019 The Linux Foundation
 * Copyright 2021 The Android Open Source Project
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

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <poll.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/ioctl.h>
#include <time.h>
#include <sound/asound.h>
#include <tinyalsa/asoundlib.h>

#include "pcm_io.h"

struct pcm_hw_data {
    /** Card number of the pcm device */
    unsigned int card;
    /** Device number for the pcm device */
    unsigned int device;
    /** File descriptor to the pcm device file node */
    int fd;
    /** Pointer to the pcm node from snd card definiton */
    struct snd_node *node;
};

static void pcm_hw_close(void *data)
{
    struct pcm_hw_data *hw_data = data;

    if (hw_data->fd >= 0)
        close(hw_data->fd);

    free(hw_data);
}

static int pcm_hw_ioctl(void *data, unsigned int cmd, ...)
{
    struct pcm_hw_data *hw_data = data;
    va_list ap;
    void *arg;

    va_start(ap, cmd);
    arg = va_arg(ap, void *);
    va_end(ap);

    return ioctl(hw_data->fd, cmd, arg);
}

static int pcm_hw_poll(void *data __attribute__((unused)),
                        struct pollfd *pfd, nfds_t nfds, int timeout)
{
    return poll(pfd, nfds, timeout);
}

static void *pcm_hw_mmap(void *data, void *addr, size_t length, int prot,
                       int flags, off_t offset)
{
    struct pcm_hw_data *hw_data = data;

    return mmap(addr, length, prot, flags, hw_data->fd, offset);
}

static int pcm_hw_munmap(void *data __attribute__((unused)), void *addr, size_t length)
{
    return munmap(addr, length);
}

static int pcm_hw_open(unsigned int card, unsigned int device,
                unsigned int flags, void **data, struct snd_node *node)
{
    struct pcm_hw_data *hw_data;
    char fn[256];
    int fd;

    hw_data = calloc(1, sizeof(*hw_data));
    if (!hw_data) {
        return -ENOMEM;
    }

    snprintf(fn, sizeof(fn), "/dev/snd/pcmC%uD%u%c", card, device,
             flags & PCM_IN ? 'c' : 'p');
    // Open the device with non-blocking flag to avoid to be blocked in kernel when all of the
    //   substreams of this PCM device are opened by others.
    fd = open(fn, O_RDWR | O_NONBLOCK);

    if (fd < 0) {
        free(hw_data);
        return fd;
    }

    if ((flags & PCM_NONBLOCK) == 0) {
        // Set the file descriptor to blocking mode.
        if (fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) & ~O_NONBLOCK) < 0) {
            fprintf(stderr, "failed to set to blocking mode on %s", fn);
            close(fd);
            free(hw_data);
            return -ENODEV;
        }
    }

    hw_data->card = card;
    hw_data->device = device;
    hw_data->fd = fd;
    hw_data->node = node;

    *data = hw_data;

    return fd;
}

const struct pcm_ops hw_ops = {
    .open = pcm_hw_open,
    .close = pcm_hw_close,
    .ioctl = pcm_hw_ioctl,
    .mmap = pcm_hw_mmap,
    .munmap = pcm_hw_munmap,
    .poll = pcm_hw_poll,
};
