#!/bin/bash
#
# SPDX-FileCopyrightText: The LineageOS Project
# SPDX-License-Identifier: Apache-2.0
#

set -e

# Load extract_utils and do some sanity checks
MY_DIR="${BASH_SOURCE%/*}"
if [[ ! -d "${MY_DIR}" ]]; then MY_DIR="${PWD}"; fi

ANDROID_ROOT="${MY_DIR}/../../.."

HELPER="${ANDROID_ROOT}/tools/extract-utils/extract_utils.sh"
if [ ! -f "${HELPER}" ]; then
    echo "Unable to find helper script at ${HELPER}"
    exit 1
fi
source "${HELPER}"

# Default to sanitizing the vendor folder before extraction
CLEAN_VENDOR=true

ONLY_COMMON=
ONLY_TARGET=
KANG=
SECTION=

while [ "${#}" -gt 0 ]; do
    case "${1}" in
        --only-common )
                ONLY_COMMON=true
                ;;
        --only-target )
                ONLY_TARGET=true
                ;;
        -n | --no-cleanup )
                CLEAN_VENDOR=false
                ;;
        -k | --kang )
                KANG="--kang"
                ;;
        -s | --section )
                SECTION="${2}"; shift
                CLEAN_VENDOR=false
                ;;
        * )
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
        # Android 11 vendor audio blobs need the A11 TypeConverter tables
        # (mTable) and helpers (deviceFromString) that Android 14 no longer
        # exports from libmedia_helper. Pull in libaudiotypeconv_shim.so
        # which provides them.
        vendor/lib/hw/android.hardware.audio@*.0-impl.so | \
        vendor/lib64/hw/android.hardware.audio@*.0-impl.so | \
        vendor/lib/libaudiofoundation.so | \
        vendor/lib64/libaudiofoundation.so | \
        vendor/lib/libeffectsconfig.so | \
        vendor/lib64/libeffectsconfig.so)
            "${PATCHELF_0_18}" --add-needed "libaudiotypeconv_shim.so" "${2}"
            ;;
        # AOSP sends bt_wbs=on/off matching the negotiated SCO codec, but
        # the Samsung HAL only understands g_sco_samplerate=<Hz>. Patch the
        # blob so bt_wbs drives the internal wideband flag (16 kHz backend
        # + "-wb" paths for mSBC, 8 kHz narrowband for CVSD).
        vendor/lib/hw/audio.primary.lito.so)
            python3 "${MY_DIR}/patches/patch_audio_primary_lito.py" "${2}"
            ;;
        # These blobs call the old BoringSSL CBS/CBB helpers that current
        # libcrypto no longer exports. Redirect them to libcrypto-v33
        # (hardware/lineage/compat, v33 BoringSSL snapshot).
        vendor/lib64/hw/gatekeeper.mdfpp.so | \
        vendor/lib64/libskeymaster4device.so | \
        vendor/lib64/libkeymaster_helper.so | \
        vendor/lib/libwvhidl.so | \
        vendor/lib/mediadrm/libwvdrmengine.so)
            "${PATCHELF_0_18}" --replace-needed "libcrypto.so" "libcrypto-v33.so" "${2}"
            ;;
        vendor/lib64/vendor.samsung.hardware.light-V1-ndk_platform.so|vendor/bin/hw/vendor.samsung.hardware.light-service)
            "${PATCHELF_0_18}" --replace-needed "android.hardware.light-V1-ndk_platform.so" "android.hardware.light-V1-ndk.so" "${2}"
            ;;
        vendor/bin/hw/macloader)
            sed -i 's/vendor\.wifi\.dualconcurrent\.interface/vnedor\.wiff\.dualconcurreut\.iuterface/g' "${2}"
            sed -i 's/ro\.vendor\.wifi\.sap\.interface/ru\.vnedor\.wiff\.sep\.iuterface/g' "${2}"
            ;;
        vendor/lib64/libdpps.so)
            "${PATCHELF_0_18}" --replace-needed "libtinyxml2.so" "libtinyxml2-v34.so" "${2}"
            ;;
        vendor/lib64/libsec-ril.so)
            sed -i 's/ril\.dds\.call\.ongoing/vendor.calls.ongoing/g' "${2}"
            # Gate the uiccApplicationsEnablementChanged unsol off the
            # OneUI version check so the framework sees SIM re-enable.
            python3 "${MY_DIR}/patches/patch_sec_ril.py" "${2}"
            ;;
        *)
            return 1
            ;;
    esac
}

if [ -z "${ONLY_TARGET}" ]; then
    # Initialize the helper for common device
    setup_vendor "${DEVICE_COMMON}" "${VENDOR}" "${ANDROID_ROOT}" true "${CLEAN_VENDOR}"

    extract "${MY_DIR}/proprietary-files.txt" "${SRC}" "${KANG}" --section "${SECTION}"
fi

if [ -z "${ONLY_COMMON}" ] && [ -s "${MY_DIR}/../${DEVICE}/proprietary-files.txt" ]; then
    # Reinitialize the helper for device
    source "${MY_DIR}/../${DEVICE}/extract-files.sh"
    setup_vendor "${DEVICE}" "${VENDOR}" "${ANDROID_ROOT}" false "${CLEAN_VENDOR}"

    extract "${MY_DIR}/../${DEVICE}/proprietary-files.txt" "${SRC}" "${KANG}" --section "${SECTION}"
fi

"${MY_DIR}/setup-makefiles.sh"
