/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <string>

#include <android-base/logging.h>
#include <bootloader_message/bootloader_message.h>
#include <misc_writer/misc_writer.h>
#include <recovery_ui/device.h>
#include <recovery_ui/screen_ui.h>

using android::hardware::google::pixel::MiscWriter;
using android::hardware::google::pixel::MiscWriterActions;

// Wipes the dark theme flag as part of data wipe.
static bool WipeDarkThemeFlag() {
    // The magic is at 0x3048 in vendor space, or (0x800 + 0x3048) since the start of /misc.
    constexpr size_t kProvisionedFlagOffsetInVendorSpace = 10814;
    MiscWriter misc_writer(MiscWriterActions::kClearDarkThemeFlag);
    if (!misc_writer.PerformAction(kProvisionedFlagOffsetInVendorSpace)) {
        LOG(ERROR) << "Failed to clear the dark theme flag";
        return false;
    }
    LOG(INFO) << "Dark theme flag wiped successfully";
    return true;
}

class Nanohub_Device : public Device {
  public:
    Nanohub_Device(ScreenRecoveryUI* ui) : Device(ui) {}
    bool PostWipeData();
};

bool Nanohub_Device::PostWipeData() {
    int fd = open("/sys/class/nanohub/nanohub/erase_shared", O_WRONLY);
    if (fd < 0) {
        PLOG(ERROR) << "open erase_shared failed";
    } else {
        if (write(fd, "1\n", 2) != 2) {
            PLOG(ERROR) << "write to erase_shared failed";
        } else {
            LOG(INFO) << "Successfully erased nanoapps";
        }
        close(fd);
    }

    WipeDarkThemeFlag();

    // open/write failure caused by permissions issues would persist across
    // reboots, so always return true to prevent a factory reset failure loop.
    return true;
}

Device* make_device() {
    return new Nanohub_Device(new ScreenRecoveryUI);
}
