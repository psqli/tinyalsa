/* mixer_hw.c
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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <poll.h>

#include <sys/ioctl.h>

#include <linux/ioctl.h>
#include <time.h>
#include <sound/asound.h>

#include "mixer_io.h"

/** Store the hardware (kernel interface) mixer data */
struct mixer_hw_data {
    /* Card number for the mixer */
    unsigned int card;
    /* File descriptor of the mixer device node */
    int fd;
};

static void mixer_hw_close(void *data)
{
    struct mixer_hw_data *hw_data = data;

    if (!hw_data)
        return;

    if (hw_data->fd >= 0)
        close(hw_data->fd);

    hw_data->fd = -1;
    free(hw_data);
    hw_data = NULL;
}

static int mixer_hw_ioctl(void *data, unsigned int cmd, ...)
{
    struct mixer_hw_data *hw_data = data;
    va_list ap;
    void *arg;

    va_start(ap, cmd);
    arg = va_arg(ap, void *);
    va_end(ap);

    return ioctl(hw_data->fd, cmd, arg);
}

static ssize_t mixer_hw_read_event(void *data, struct snd_ctl_event *ev,
                                   size_t size)
{
    struct mixer_hw_data *hw_data = data;

    return read(hw_data->fd, ev, size);
}

static const struct mixer_ops mixer_hw_ops = {
    .close = mixer_hw_close,
    .ioctl = mixer_hw_ioctl,
    .read_event = mixer_hw_read_event,
};

int mixer_hw_open(unsigned int card, void **data,
                  const struct mixer_ops **ops)
{
    struct mixer_hw_data *hw_data;
    int fd;
    char fn[256];

    snprintf(fn, sizeof(fn), "/dev/snd/controlC%u", card);
    fd = open(fn, O_RDWR);
    if (fd < 0)
        return fd;

    hw_data = calloc(1, sizeof(*hw_data));
    if (!hw_data)
        return -1;

    hw_data->card = card;
    hw_data->fd = fd;
    *data = hw_data;
    *ops = &mixer_hw_ops;

    return fd;
}
