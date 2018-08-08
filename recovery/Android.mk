LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := recovery_updater.cpp
LOCAL_MODULE := librecovery_updater_xiaomi
LOCAL_STATIC_LIBRARIES := libedify libotautil
include $(BUILD_STATIC_LIBRARY)
