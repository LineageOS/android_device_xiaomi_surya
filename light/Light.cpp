/*
 * Copyright (C) 2018-2022 The LineageOS Project
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

#define LOG_TAG "LightService"

#include <log/log.h>

#include "Light.h"

#include <fstream>

#define NOTIFICATION_LED_LEFT(x) "/sys/class/leds/left/" + x
#define NOTIFICATION_LED_WHITE(x) "/sys/class/leds/white/" + x

#define BREATH              "breath"
#define BRIGHTNESS          "brightness"
#define MAX_BRIGHTNESS      "max_brightness"

namespace {
/*
 * Get the full path to the led node.
 */
static std::string getLedPath(std::string path) {
    std::ifstream left_path(NOTIFICATION_LED_LEFT(path));
    return left_path.good() ? NOTIFICATION_LED_LEFT(path) : NOTIFICATION_LED_WHITE(path);
}

/*
 * Write value to path and close file.
 */
static void set(std::string path, std::string value) {
    std::ofstream file(getLedPath(path));

    if (!file.is_open()) {
        ALOGW("failed to write %s to %s", value.c_str(), path.c_str());
        return;
    }

    file << value;
}

static void set(std::string path, int value) {
    set(path, std::to_string(value));
}

/*
 * Read max brightness from path and close file.
 */
static int getMaxBrightness(std::string path) {
    std::ifstream file(getLedPath(path));
    int value;

    if (!file.is_open()) {
        ALOGW("failed to read from %s", path.c_str());
        return 0;
    }

    file >> value;
    return value;
}

static uint32_t getBrightness(const LightState& state) {
    uint32_t alpha, red, green, blue;

    /*
     * Extract brightness from AARRGGBB.
     */
    alpha = (state.color >> 24) & 0xFF;
    red = (state.color >> 16) & 0xFF;
    green = (state.color >> 8) & 0xFF;
    blue = state.color & 0xFF;

    /*
     * Scale RGB brightness using Alpha brightness.
     */
    red = red * alpha / 0xFF;
    green = green * alpha / 0xFF;
    blue = blue * alpha / 0xFF;

    return (77 * red + 150 * green + 29 * blue) >> 8;
}

static inline uint32_t scaleBrightness(uint32_t brightness, uint32_t maxBrightness) {
    if (brightness == 0) {
        return 0;
    }

    return (brightness - 1) * (maxBrightness - 1) / (0xFF - 1) + 1;
}

static inline uint32_t getScaledBrightness(const LightState& state, uint32_t maxBrightness) {
    return scaleBrightness(getBrightness(state), maxBrightness);
}

static void handleNotification(const LightState& state) {
    uint32_t notificationBrightness = getScaledBrightness(state, getMaxBrightness(MAX_BRIGHTNESS));

    /* Disable breathing or blinking */
    set(BREATH, 0);
    set(BRIGHTNESS, 0);

    if (!notificationBrightness) {
        return;
    }

    switch (state.flashMode) {
        case Flash::HARDWARE:
        case Flash::TIMED:
            /* Breathing */
            set(BREATH, 1);
            break;
        case Flash::NONE:
        default:
            set(BRIGHTNESS, notificationBrightness);
    }
}

static inline bool isStateLit(const LightState& state) {
    return state.color & 0x00ffffff;
}

static inline bool isStateEqual(const LightState& first, const LightState& second) {
    if (first.color == second.color && first.flashMode == second.flashMode &&
            first.flashOnMs == second.flashOnMs &&
            first.flashOffMs == second.flashOffMs &&
            first.brightnessMode == second.brightnessMode) {
        return true;
    }

    return false;
}

/* Keep sorted in the order of importance. */
static std::vector<LightBackend> backends = {
    { Type::ATTENTION, handleNotification },
    { Type::NOTIFICATIONS, handleNotification },
    { Type::BATTERY, handleNotification },
};

static LightStateHandler findHandler(Type type) {
    for (const LightBackend& backend : backends) {
        if (backend.type == type) {
            return backend.handler;
        }
    }

    return nullptr;
}

static LightState findLitState(LightStateHandler handler) {
    LightState emptyState;

    for (const LightBackend& backend : backends) {
        if (backend.handler == handler) {
            if (isStateLit(backend.state)) {
                return backend.state;
            }

            emptyState = backend.state;
        }
    }

    return emptyState;
}

static void updateState(Type type, const LightState& state) {
    for (LightBackend& backend : backends) {
        if (backend.type == type) {
            backend.state = state;
        }
    }
}

}  // anonymous namespace

namespace android {
namespace hardware {
namespace light {
namespace V2_0 {
namespace implementation {

Return<Status> Light::setLight(Type type, const LightState& state) {
    /* Lock global mutex until light state is updated. */
    std::lock_guard<std::mutex> lock(globalLock);

    LightStateHandler handler = findHandler(type);
    if (!handler) {
        /* If no handler has been found, then the type is not supported. */
        return Status::LIGHT_NOT_SUPPORTED;
    }

    /* Find the old state of the current handler. */
    LightState oldState = findLitState(handler);

    /* Update the cached state value for the current type. */
    updateState(type, state);

    /* Find the new state of the current handler. */
    LightState newState = findLitState(handler);

    if (isStateEqual(oldState, newState)) {
        return Status::SUCCESS;
    }

    handler(newState);

    return Status::SUCCESS;
}

Return<void> Light::getSupportedTypes(getSupportedTypes_cb _hidl_cb) {
    std::vector<Type> types;

    for (const LightBackend& backend : backends) {
        types.push_back(backend.type);
    }

    _hidl_cb(types);

    return Void();
}

}  // namespace implementation
}  // namespace V2_0
}  // namespace light
}  // namespace hardware
}  // namespace android
