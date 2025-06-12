#
# Copyright (C) 2020-2025 The LineageOS Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# Inherit from sm6150-common
include device/xiaomi/sm6150-common/BoardConfigCommon.mk

DEVICE_PATH := device/xiaomi/surya

# Assert
TARGET_OTA_ASSERT_DEVICE := surya,karna

# Audio
TARGET_PROVIDES_AUDIO_EXTNS := true

# HIDL
ODM_MANIFEST_SKUS += surya
ODM_MANIFEST_SURYA_FILES := \
    $(DEVICE_PATH)/configs/hidl/manifest-nfc.xml

# Kernel
TARGET_KERNEL_CONFIG := surya_defconfig
TARGET_KERNEL_SOURCE := kernel/xiaomi/surya

# Partitions
BOARD_BOOTIMAGE_PARTITION_SIZE := 134217728
BOARD_CACHEIMAGE_PARTITION_SIZE := 402653184
BOARD_DTBOIMG_PARTITION_SIZE := 25165824
BOARD_RECOVERYIMAGE_PARTITION_SIZE := 134217728
BOARD_SUPER_PARTITION_SIZE := 8589934592

BOARD_QTI_DYNAMIC_PARTITIONS_SIZE := 8585740288 # (BOARD_SUPER_PARTITION_SIZE - 4194304) 4MiB overhead

# Properties
TARGET_ODM_PROP += $(DEVICE_PATH)/properties/odm.prop
TARGET_VENDOR_PROP += $(DEVICE_PATH)/properties/vendor.prop

# Screen density
TARGET_SCREEN_DENSITY := 440

# Vendor security patch level
VENDOR_SECURITY_PATCH := 2023-08-17

# Inherit from proprietary files
include vendor/xiaomi/surya/BoardConfigVendor.mk
