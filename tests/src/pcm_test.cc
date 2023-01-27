/* pcm_out_test.c
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

#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <string_view>
#include <string>
#include <thread>

#include <gtest/gtest.h>

#include "tinyalsa/pcm.h"

#include "pcm_test_device.h"

namespace tinyalsa {
namespace testing {

TEST(PcmTest, FormatToBits) {
    // FIXME: Should we return 16 bits for INVALID?
    ASSERT_EQ(pcm_format_to_bits(PCM_FORMAT_INVALID), 16);

    ASSERT_EQ(pcm_format_to_bits(PCM_FORMAT_S16_LE), 16);
    ASSERT_EQ(pcm_format_to_bits(PCM_FORMAT_S32_LE), 32);
    ASSERT_EQ(pcm_format_to_bits(PCM_FORMAT_S8), 8);
    ASSERT_EQ(pcm_format_to_bits(PCM_FORMAT_S24_LE), 32);
    ASSERT_EQ(pcm_format_to_bits(PCM_FORMAT_S24_3LE), 24);
    ASSERT_EQ(pcm_format_to_bits(PCM_FORMAT_S16_BE), 16);
    ASSERT_EQ(pcm_format_to_bits(PCM_FORMAT_S24_BE), 32);
    ASSERT_EQ(pcm_format_to_bits(PCM_FORMAT_S24_3BE), 24);
    ASSERT_EQ(pcm_format_to_bits(PCM_FORMAT_S32_BE), 32);
    ASSERT_EQ(pcm_format_to_bits(PCM_FORMAT_FLOAT_LE), 32);
    ASSERT_EQ(pcm_format_to_bits(PCM_FORMAT_FLOAT_BE), 32);
}

TEST(PcmTest, OpenAndCloseOutPcm) {
    pcm *pcm_object = pcm_open(1000, 1000, PCM_OUT, &kDefaultConfig);
    ASSERT_FALSE(pcm_is_ready(pcm_object));
    ASSERT_EQ(pcm_close(pcm_object), 0);

    // assume card 0, device 0 is always available
    pcm_object = pcm_open(0, 0, PCM_OUT, &kDefaultConfig);
    ASSERT_TRUE(pcm_is_ready(pcm_object));
    ASSERT_EQ(pcm_close(pcm_object), 0);

    pcm_object = pcm_open(0, 0, PCM_OUT | PCM_MMAP, &kDefaultConfig);
    ASSERT_TRUE(pcm_is_ready(pcm_object));
    ASSERT_EQ(pcm_close(pcm_object), 0);

    pcm_object = pcm_open(0, 0, PCM_OUT | PCM_MMAP | PCM_NOIRQ, &kDefaultConfig);
    ASSERT_TRUE(pcm_is_ready(pcm_object));
    ASSERT_EQ(pcm_close(pcm_object), 0);

    pcm_object = pcm_open(0, 0, PCM_OUT | PCM_NONBLOCK, &kDefaultConfig);
    ASSERT_TRUE(pcm_is_ready(pcm_object));
    ASSERT_EQ(pcm_close(pcm_object), 0);

    pcm_object = pcm_open(0, 0, PCM_OUT | PCM_MONOTONIC, &kDefaultConfig);
    ASSERT_TRUE(pcm_is_ready(pcm_object));
    ASSERT_EQ(pcm_close(pcm_object), 0);

    std::string name = "hw:0,0";
    pcm_object = pcm_open_by_name(name.c_str(), PCM_OUT, &kDefaultConfig);
    ASSERT_TRUE(pcm_is_ready(pcm_object));
    ASSERT_EQ(pcm_close(pcm_object), 0);
}

TEST(PcmTest, OpenWithoutBlocking) {
    char loopback_device_info_path[120] = {};
    snprintf(loopback_device_info_path, sizeof(loopback_device_info_path),
            "/proc/asound/card%d/pcm%dp/info", kLoopbackCard, kLoopbackPlaybackDevice);

    std::ifstream info_file_stream{loopback_device_info_path};
    if (!info_file_stream.is_open()) {
        GTEST_SKIP();
    }

    char buffer[256] = {};
    int32_t subdevice_count = 0;
    while (info_file_stream.good()) {
        info_file_stream.getline(buffer, sizeof(buffer));
        std::cout << buffer << std::endl;
        std::string_view line{buffer};
        if (line.find("subdevices_count") != std::string_view::npos) {
            auto subdevice_count_string = line.substr(line.find(":") + 1);
            std::cout << subdevice_count_string << std::endl;
            subdevice_count = std::stoi(std::string{subdevice_count_string});
        }
    }

    ASSERT_GT(subdevice_count, 0);

    auto pcm_array = std::make_unique<pcm *[]>(subdevice_count);
    std::thread *open_thread = new std::thread{[&pcm_array, subdevice_count] {
        // Occupy all substreams
        for (int32_t i = 0; i < subdevice_count; i++) {
            pcm_array[i] = pcm_open(kLoopbackCard, kLoopbackPlaybackDevice, PCM_OUT,
                    &kDefaultConfig);
            EXPECT_TRUE(pcm_is_ready(pcm_array[i]));
        }

        // Expect that pcm_open is not blocked in the kernel and return a bad_object pointer.
        pcm *pcm_object = pcm_open(kLoopbackCard, kLoopbackPlaybackDevice, PCM_OUT,
                    &kDefaultConfig);
        if (pcm_is_ready(pcm_object)) {
            // open_thread is blocked in kernel because of the substream is all occupied. pcm_open
            // returns because the main thread has released all pcm structures in pcm_array. We just
            // need to close the pcm_object here.
            pcm_close(pcm_object);
            return;
        }

        // Release all substreams
        for (int32_t i = 0; i < subdevice_count; i++) {
            pcm_close(pcm_array[i]);
            pcm_array[i] = nullptr;
        }
    }};

    static constexpr int64_t kTimeoutMs = 500;
    std::this_thread::sleep_for(std::chrono::milliseconds(kTimeoutMs));
    if (pcm_array[0] == nullptr) {
        open_thread->join();
    } else {
        for (int32_t i = 0; i < subdevice_count; i++) {
            pcm_close(pcm_array[i]);
            pcm_array[i] = nullptr;
        }
        open_thread->join();
        FAIL() << "The open_thread is blocked in kernel or the kTimeoutMs(" << kTimeoutMs <<
                ") is too short to complete";
    }
}

} // namespace testing
} // namespace tinyalsa
