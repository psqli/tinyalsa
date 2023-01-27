/* pcm_test.h
 *
 * Copyright 2020 The Android Open Source Project
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

#ifndef TINYALSA_TESTS_PCM_TEST_H_
#define TINYALSA_TESTS_PCM_TEST_H_

#include "tinyalsa/pcm.h"

namespace tinyalsa {
namespace testing {

#ifndef TEST_LOOPBACK_CARD
#define TEST_LOOPBACK_CARD 2
#endif

#ifndef TEST_LOOPBACK_PLAYBACK_DEVICE
#define TEST_LOOPBACK_PLAYBACK_DEVICE 0
#endif

#ifndef TEST_LOOPBACK_CAPTURE_DEVICE
#define TEST_LOOPBACK_CAPTURE_DEVICE 1
#endif

static constexpr unsigned int kLoopbackCard = TEST_LOOPBACK_CARD;
static constexpr unsigned int kLoopbackPlaybackDevice = TEST_LOOPBACK_PLAYBACK_DEVICE;
static constexpr unsigned int kLoopbackCaptureDevice = TEST_LOOPBACK_CAPTURE_DEVICE;

static constexpr unsigned int kDefaultChannels = 2;
static constexpr unsigned int kDefaultSamplingRate = 48000;
static constexpr unsigned int kDefaultPeriodSize = 1024;
static constexpr unsigned int kDefaultPeriodCount = 3;
static constexpr pcm_config kDefaultConfig = {
    .channels = kDefaultChannels,
    .rate = kDefaultSamplingRate,
    .period_size = kDefaultPeriodSize,
    .period_count = kDefaultPeriodCount,
    .format = PCM_FORMAT_S16_LE,
    .start_threshold = kDefaultPeriodSize,
    .stop_threshold = kDefaultPeriodSize * kDefaultPeriodCount,
    .silence_threshold = 0,
    .silence_size = 0,
};

} // namespace testing
} // namespace tinyalsa

#endif
