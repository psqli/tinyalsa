/* pcm_in_test.c
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

#include "pcm_test_device.h"

#include <chrono>
#include <cstring>
#include <iostream>

#include <gtest/gtest.h>

#include "tinyalsa/pcm.h"

namespace tinyalsa {
namespace testing {

class PcmInTest : public ::testing::Test {
  protected:
    PcmInTest() : pcm_object(nullptr) {}
    virtual ~PcmInTest() = default;

    virtual void SetUp() override {
        pcm_object = pcm_open(kLoopbackCard, kLoopbackCaptureDevice, PCM_IN, &kDefaultConfig);
        ASSERT_NE(pcm_object, nullptr);
        ASSERT_TRUE(pcm_is_ready(pcm_object));
    }

    virtual void TearDown() override {
        ASSERT_EQ(pcm_close(pcm_object), 0);
    }

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
        .start_threshold = 0,
        .stop_threshold = 0,
        .silence_threshold = 0,
        .silence_size = 0,
    };

    pcm* pcm_object;
};

TEST_F(PcmInTest, GetDelay) {
    pcm_prepare(pcm_object);
    long delay = pcm_get_delay(pcm_object);
    std::cout << delay << std::endl;
    ASSERT_GE(delay, 0);
}

TEST_F(PcmInTest, Readi) {
    constexpr uint32_t read_count = 20;

    size_t buffer_size = pcm_frames_to_bytes(pcm_object, kDefaultConfig.period_size);
    auto buffer = std::make_unique<char[]>(buffer_size);

    int read_frames = 0;
    unsigned int frames = pcm_bytes_to_frames(pcm_object, buffer_size);
    auto start = std::chrono::steady_clock::now();
    for (uint32_t i = 0; i < read_count; ++i) {
        read_frames = pcm_readi(pcm_object, buffer.get(), frames);
        ASSERT_EQ(read_frames, frames);
    }

    std::chrono::duration<double> difference = std::chrono::steady_clock::now() - start;
    std::chrono::milliseconds expected_elapsed_time_ms(frames * read_count /
            (kDefaultConfig.rate / 1000));

    std::cout << difference.count() << std::endl;
    std::cout << expected_elapsed_time_ms.count() << std::endl;

    ASSERT_NEAR(difference.count() * 1000, expected_elapsed_time_ms.count(), 100);
}

TEST_F(PcmInTest, Writei) {
    size_t buffer_size = pcm_frames_to_bytes(pcm_object, kDefaultConfig.period_size);
    auto buffer = std::make_unique<char[]>(buffer_size);

    unsigned int frames = pcm_bytes_to_frames(pcm_object, buffer_size);
    ASSERT_EQ(pcm_writei(pcm_object, buffer.get(), frames), -EINVAL);
}

} // namespace testing
} // namespace tinyalsa
