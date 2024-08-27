#!/bin/bash
#
# SPDX-FileCopyrightText: 2016 The CyanogenMod Project
# SPDX-FileCopyrightText: 2017-2024 The LineageOS Project
# SPDX-License-Identifier: Apache-2.0
#

set -e

DEVICE=surya
VENDOR=xiaomi

# Load extract_utils and do some sanity checks
MY_DIR="${BASH_SOURCE%/*}"
if [[ ! -d "${MY_DIR}" ]]; then MY_DIR="${PWD}"; fi

ANDROID_ROOT="${MY_DIR}/../../.."

# If XML files don't have comments before the XML header, use this flag
# Can still be used with broken XML files by using blob_fixup
export TARGET_DISABLE_XML_FIXING=true

HELPER="${ANDROID_ROOT}/tools/extract-utils/extract_utils.sh"
if [ ! -f "${HELPER}" ]; then
    echo "Unable to find helper script at ${HELPER}"
    exit 1
fi
source "${HELPER}"

# Default to sanitizing the vendor folder before extraction
CLEAN_VENDOR=true

KANG=
ONLY_FIRMWARE=
ONLY_TARGET=
SECTION=

while [ "${#}" -gt 0 ]; do
    case "${1}" in
        --only-firmware)
            ONLY_FIRMWARE=true
            ;;
        --only-target)
            ONLY_TARGET=true
            ;;
        -n | --no-cleanup)
            CLEAN_VENDOR=false
            ;;
        -k | --kang)
            KANG="--kang"
            ;;
        -s | --section)
            SECTION="${2}"
            shift
            CLEAN_VENDOR=false
            ;;
        *)
            SRC="${1}"
            ;;
    esac
    shift
done

if [ -z "${SRC}" ]; then
    SRC="adb"
fi

function blob_fixup() {
    case "${1}" in
        vendor/lib64/camera/components/com.qti.node.watermark.so)
            [ "$2" = "" ] && return 0
            grep -q "libpiex_shim.so" "${2}" || ${PATCHELF} --add-needed "libpiex_shim.so" "${2}"
            ;;

	vendor/lib64/libgoodixhwfingerprint.so)
            [ "$2" = "" ] && return 0
            grep -q "libvendor.goodix.hardware.biometrics.fingerprint@2.1.so" "${2}" && ${PATCHELF_0_17_2} --replace-needed "libvendor.goodix.hardware.biometrics.fingerprint@2.1.so" "vendor.goodix.hardware.biometrics.fingerprint@2.1.so" "${2}"
            ;;

	vendor/lib64/android.hardware.camera.provider@2.4-legacy.so)
            [ "$2" = "" ] && return 0
            grep -q "libcamera_provider_shim.so" "${2}" || "${PATCHELF}" --add-needed "libcamera_provider_shim.so" "${2}"
            ;;
        *)
            return 1
            ;;
    esac

    return 0
}

# Initialize the helper
setup_vendor "${DEVICE}" "${VENDOR}" "${ANDROID_ROOT}" false "${CLEAN_VENDOR}"
if [ -z "${ONLY_FIRMWARE}" ]; then
	extract "${MY_DIR}/proprietary-files.txt" "${SRC}" "${KANG}" --section "${SECTION}"
fi

if [ -z "${ONLY_TARGET}" ]; then
	extract_firmware "${MY_DIR}/proprietary-firmware.txt" "${SRC}"
fi

"${MY_DIR}/setup-makefiles.sh"
