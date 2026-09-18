/*
 * ABI-compat shim for legacy (Android 11) vendor audio blobs on Android 14.
 *
 * The stock vendor audio HAL (libaudiofoundation.so / audio.primary.lito)
 * was built against Android 11's libmedia_helper, which exported the static
 * data members:
 *   android::TypeConverter<android::OutputDeviceTraits>::mTable
 *   android::TypeConverter<android::InputDeviceTraits>::mTable
 *   android::TypeConverter<android::FormatTraits>::mTable
 *
 * Android 14 replaced those device/format TypeConverter tables with
 * toStringImpl/fromStringImpl specialisations and no longer emits mTable,
 * so the blobs fail to dlopen with "cannot locate symbol ...mTable".
 *
 * This shim reproduces the exact type hierarchy so the mangled names match,
 * and provides the enum->literal tables the blobs use for conversion.
 */

#include <system/audio.h>
#include <set>
#include <string>
#include <vector>

namespace android {

template <typename T>
struct DefaultTraits {
    typedef T Type;
    typedef std::vector<Type> Collection;
    static void add(Collection &collection, Type value) { collection.push_back(value); }
};

template <typename T>
struct SetTraits {
    typedef T Type;
    typedef std::set<Type> Collection;
    static void add(Collection &collection, Type value) { collection.insert(value); }
};

using DeviceTraits = DefaultTraits<audio_devices_t>;
struct OutputDeviceTraits : public DeviceTraits {};
struct InputDeviceTraits : public DeviceTraits {};
using FormatTraits = DefaultTraits<audio_format_t>;

template <class Traits>
class TypeConverter {
public:
    struct Table {
        const char *literal;
        typename Traits::Type value;
    };
    static const Table mTable[];
};

/* Output devices (audio_devices_t) */
template <>
const TypeConverter<OutputDeviceTraits>::Table
    TypeConverter<OutputDeviceTraits>::mTable[] = {
        { "AUDIO_DEVICE_OUT_EARPIECE", AUDIO_DEVICE_OUT_EARPIECE },
        { "AUDIO_DEVICE_OUT_SPEAKER", AUDIO_DEVICE_OUT_SPEAKER },
        { "AUDIO_DEVICE_OUT_WIRED_HEADSET", AUDIO_DEVICE_OUT_WIRED_HEADSET },
        { "AUDIO_DEVICE_OUT_WIRED_HEADPHONE", AUDIO_DEVICE_OUT_WIRED_HEADPHONE },
        { "AUDIO_DEVICE_OUT_BLUETOOTH_SCO", AUDIO_DEVICE_OUT_BLUETOOTH_SCO },
        { "AUDIO_DEVICE_OUT_BLUETOOTH_SCO_HEADSET", AUDIO_DEVICE_OUT_BLUETOOTH_SCO_HEADSET },
        { "AUDIO_DEVICE_OUT_BLUETOOTH_SCO_CARKIT", AUDIO_DEVICE_OUT_BLUETOOTH_SCO_CARKIT },
        { "AUDIO_DEVICE_OUT_BLUETOOTH_A2DP", AUDIO_DEVICE_OUT_BLUETOOTH_A2DP },
        { "AUDIO_DEVICE_OUT_BLUETOOTH_A2DP_HEADPHONES", AUDIO_DEVICE_OUT_BLUETOOTH_A2DP_HEADPHONES },
        { "AUDIO_DEVICE_OUT_BLUETOOTH_A2DP_SPEAKER", AUDIO_DEVICE_OUT_BLUETOOTH_A2DP_SPEAKER },
        { "AUDIO_DEVICE_OUT_HDMI", AUDIO_DEVICE_OUT_HDMI },
        { "AUDIO_DEVICE_OUT_AUX_DIGITAL", AUDIO_DEVICE_OUT_AUX_DIGITAL },
        { "AUDIO_DEVICE_OUT_ANLG_DOCK_HEADSET", AUDIO_DEVICE_OUT_ANLG_DOCK_HEADSET },
        { "AUDIO_DEVICE_OUT_DGTL_DOCK_HEADSET", AUDIO_DEVICE_OUT_DGTL_DOCK_HEADSET },
        { "AUDIO_DEVICE_OUT_USB_ACCESSORY", AUDIO_DEVICE_OUT_USB_ACCESSORY },
        { "AUDIO_DEVICE_OUT_USB_DEVICE", AUDIO_DEVICE_OUT_USB_DEVICE },
        { "AUDIO_DEVICE_OUT_REMOTE_SUBMIX", AUDIO_DEVICE_OUT_REMOTE_SUBMIX },
        { "AUDIO_DEVICE_OUT_TELEPHONY_TX", AUDIO_DEVICE_OUT_TELEPHONY_TX },
        { "AUDIO_DEVICE_OUT_LINE", AUDIO_DEVICE_OUT_LINE },
        { "AUDIO_DEVICE_OUT_HDMI_ARC", AUDIO_DEVICE_OUT_HDMI_ARC },
        { "AUDIO_DEVICE_OUT_HDMI_EARC", AUDIO_DEVICE_OUT_HDMI_EARC },
        { "AUDIO_DEVICE_OUT_SPDIF", AUDIO_DEVICE_OUT_SPDIF },
        { "AUDIO_DEVICE_OUT_FM", AUDIO_DEVICE_OUT_FM },
        { "AUDIO_DEVICE_OUT_AUX_LINE", AUDIO_DEVICE_OUT_AUX_LINE },
        { "AUDIO_DEVICE_OUT_SPEAKER_SAFE", AUDIO_DEVICE_OUT_SPEAKER_SAFE },
        { "AUDIO_DEVICE_OUT_IP", AUDIO_DEVICE_OUT_IP },
        { "AUDIO_DEVICE_OUT_BUS", AUDIO_DEVICE_OUT_BUS },
        { "AUDIO_DEVICE_OUT_PROXY", AUDIO_DEVICE_OUT_PROXY },
        { "AUDIO_DEVICE_OUT_USB_HEADSET", AUDIO_DEVICE_OUT_USB_HEADSET },
        { "AUDIO_DEVICE_OUT_HEARING_AID", AUDIO_DEVICE_OUT_HEARING_AID },
        { "AUDIO_DEVICE_OUT_ECHO_CANCELLER", AUDIO_DEVICE_OUT_ECHO_CANCELLER },
        { "AUDIO_DEVICE_OUT_BLE_HEADSET", AUDIO_DEVICE_OUT_BLE_HEADSET },
        { "AUDIO_DEVICE_OUT_BLE_SPEAKER", AUDIO_DEVICE_OUT_BLE_SPEAKER },
        { "AUDIO_DEVICE_OUT_BLE_BROADCAST", AUDIO_DEVICE_OUT_BLE_BROADCAST },
        { "AUDIO_DEVICE_OUT_STUB", AUDIO_DEVICE_OUT_STUB },
        { "AUDIO_DEVICE_OUT_DEFAULT", AUDIO_DEVICE_OUT_DEFAULT },
        { nullptr, (audio_devices_t)0 },
    };

/* Input devices (audio_devices_t) */
template <>
const TypeConverter<InputDeviceTraits>::Table
    TypeConverter<InputDeviceTraits>::mTable[] = {
        { "AUDIO_DEVICE_IN_COMMUNICATION", AUDIO_DEVICE_IN_COMMUNICATION },
        { "AUDIO_DEVICE_IN_AMBIENT", AUDIO_DEVICE_IN_AMBIENT },
        { "AUDIO_DEVICE_IN_BUILTIN_MIC", AUDIO_DEVICE_IN_BUILTIN_MIC },
        { "AUDIO_DEVICE_IN_BLUETOOTH_SCO_HEADSET", AUDIO_DEVICE_IN_BLUETOOTH_SCO_HEADSET },
        { "AUDIO_DEVICE_IN_WIRED_HEADSET", AUDIO_DEVICE_IN_WIRED_HEADSET },
        { "AUDIO_DEVICE_IN_HDMI", AUDIO_DEVICE_IN_HDMI },
        { "AUDIO_DEVICE_IN_TELEPHONY_RX", AUDIO_DEVICE_IN_TELEPHONY_RX },
        { "AUDIO_DEVICE_IN_BACK_MIC", AUDIO_DEVICE_IN_BACK_MIC },
        { "AUDIO_DEVICE_IN_REMOTE_SUBMIX", AUDIO_DEVICE_IN_REMOTE_SUBMIX },
        { "AUDIO_DEVICE_IN_ANLG_DOCK_HEADSET", AUDIO_DEVICE_IN_ANLG_DOCK_HEADSET },
        { "AUDIO_DEVICE_IN_DGTL_DOCK_HEADSET", AUDIO_DEVICE_IN_DGTL_DOCK_HEADSET },
        { "AUDIO_DEVICE_IN_USB_ACCESSORY", AUDIO_DEVICE_IN_USB_ACCESSORY },
        { "AUDIO_DEVICE_IN_USB_DEVICE", AUDIO_DEVICE_IN_USB_DEVICE },
        { "AUDIO_DEVICE_IN_USB_HEADSET", AUDIO_DEVICE_IN_USB_HEADSET },
        { "AUDIO_DEVICE_IN_AUX_DIGITAL", AUDIO_DEVICE_IN_AUX_DIGITAL },
        { "AUDIO_DEVICE_IN_VOICE_CALL", AUDIO_DEVICE_IN_VOICE_CALL },
        { "AUDIO_DEVICE_IN_LOOPBACK", AUDIO_DEVICE_IN_LOOPBACK },
        { "AUDIO_DEVICE_IN_SPDIF", AUDIO_DEVICE_IN_SPDIF },
        { "AUDIO_DEVICE_IN_HDMI_ARC", AUDIO_DEVICE_IN_HDMI_ARC },
        { "AUDIO_DEVICE_IN_HDMI_EARC", AUDIO_DEVICE_IN_HDMI_EARC },
        { "AUDIO_DEVICE_IN_FM_TUNER", AUDIO_DEVICE_IN_FM_TUNER },
        { "AUDIO_DEVICE_IN_TV_TUNER", AUDIO_DEVICE_IN_TV_TUNER },
        { "AUDIO_DEVICE_IN_LINE", AUDIO_DEVICE_IN_LINE },
        { "AUDIO_DEVICE_IN_IP", AUDIO_DEVICE_IN_IP },
        { "AUDIO_DEVICE_IN_BUS", AUDIO_DEVICE_IN_BUS },
        { "AUDIO_DEVICE_IN_PROXY", AUDIO_DEVICE_IN_PROXY },
        { "AUDIO_DEVICE_IN_BLUETOOTH_A2DP", AUDIO_DEVICE_IN_BLUETOOTH_A2DP },
        { "AUDIO_DEVICE_IN_BLUETOOTH_BLE", AUDIO_DEVICE_IN_BLUETOOTH_BLE },
        { "AUDIO_DEVICE_IN_BLE_HEADSET", AUDIO_DEVICE_IN_BLE_HEADSET },
        { "AUDIO_DEVICE_IN_ECHO_REFERENCE", AUDIO_DEVICE_IN_ECHO_REFERENCE },
        { "AUDIO_DEVICE_IN_STUB", AUDIO_DEVICE_IN_STUB },
        { "AUDIO_DEVICE_IN_DEFAULT", AUDIO_DEVICE_IN_DEFAULT },
        { nullptr, (audio_devices_t)0 },
    };

/* Formats (audio_format_t) */
template <>
const TypeConverter<FormatTraits>::Table
    TypeConverter<FormatTraits>::mTable[] = {
        { "AUDIO_FORMAT_PCM_16_BIT", AUDIO_FORMAT_PCM_16_BIT },
        { "AUDIO_FORMAT_PCM_8_BIT", AUDIO_FORMAT_PCM_8_BIT },
        { "AUDIO_FORMAT_PCM_32_BIT", AUDIO_FORMAT_PCM_32_BIT },
        { "AUDIO_FORMAT_PCM_8_24_BIT", AUDIO_FORMAT_PCM_8_24_BIT },
        { "AUDIO_FORMAT_PCM_FLOAT", AUDIO_FORMAT_PCM_FLOAT },
        { "AUDIO_FORMAT_PCM_24_BIT_PACKED", AUDIO_FORMAT_PCM_24_BIT_PACKED },
        { "AUDIO_FORMAT_MP3", AUDIO_FORMAT_MP3 },
        { "AUDIO_FORMAT_AMR_NB", AUDIO_FORMAT_AMR_NB },
        { "AUDIO_FORMAT_AMR_WB", AUDIO_FORMAT_AMR_WB },
        { "AUDIO_FORMAT_AAC", AUDIO_FORMAT_AAC },
        { "AUDIO_FORMAT_AAC_MAIN", AUDIO_FORMAT_AAC_MAIN },
        { "AUDIO_FORMAT_AAC_LC", AUDIO_FORMAT_AAC_LC },
        { "AUDIO_FORMAT_AAC_SSR", AUDIO_FORMAT_AAC_SSR },
        { "AUDIO_FORMAT_AAC_LTP", AUDIO_FORMAT_AAC_LTP },
        { "AUDIO_FORMAT_AAC_HE_V1", AUDIO_FORMAT_AAC_HE_V1 },
        { "AUDIO_FORMAT_AAC_SCALABLE", AUDIO_FORMAT_AAC_SCALABLE },
        { "AUDIO_FORMAT_AAC_ERLC", AUDIO_FORMAT_AAC_ERLC },
        { "AUDIO_FORMAT_AAC_LD", AUDIO_FORMAT_AAC_LD },
        { "AUDIO_FORMAT_AAC_HE_V2", AUDIO_FORMAT_AAC_HE_V2 },
        { "AUDIO_FORMAT_AAC_ELD", AUDIO_FORMAT_AAC_ELD },
        { "AUDIO_FORMAT_AAC_XHE", AUDIO_FORMAT_AAC_XHE },
        { "AUDIO_FORMAT_HE_AAC_V1", AUDIO_FORMAT_HE_AAC_V1 },
        { "AUDIO_FORMAT_HE_AAC_V2", AUDIO_FORMAT_HE_AAC_V2 },
        { "AUDIO_FORMAT_VORBIS", AUDIO_FORMAT_VORBIS },
        { "AUDIO_FORMAT_OPUS", AUDIO_FORMAT_OPUS },
        { "AUDIO_FORMAT_AC3", AUDIO_FORMAT_AC3 },
        { "AUDIO_FORMAT_E_AC3", AUDIO_FORMAT_E_AC3 },
        { "AUDIO_FORMAT_E_AC3_JOC", AUDIO_FORMAT_E_AC3_JOC },
        { "AUDIO_FORMAT_DTS", AUDIO_FORMAT_DTS },
        { "AUDIO_FORMAT_DTS_HD", AUDIO_FORMAT_DTS_HD },
        { "AUDIO_FORMAT_IEC61937", AUDIO_FORMAT_IEC61937 },
        { "AUDIO_FORMAT_DOLBY_TRUEHD", AUDIO_FORMAT_DOLBY_TRUEHD },
        { "AUDIO_FORMAT_FLAC", AUDIO_FORMAT_FLAC },
        { "AUDIO_FORMAT_ALAC", AUDIO_FORMAT_ALAC },
        { "AUDIO_FORMAT_APE", AUDIO_FORMAT_APE },
        { "AUDIO_FORMAT_WMA", AUDIO_FORMAT_WMA },
        { "AUDIO_FORMAT_WMA_PRO", AUDIO_FORMAT_WMA_PRO },
        { "AUDIO_FORMAT_MAT", AUDIO_FORMAT_MAT },
        { "AUDIO_FORMAT_DSD", AUDIO_FORMAT_DSD },
        { "AUDIO_FORMAT_DRA", AUDIO_FORMAT_DRA },
        { "AUDIO_FORMAT_CELT", AUDIO_FORMAT_CELT },
        { "AUDIO_FORMAT_EVRC", AUDIO_FORMAT_EVRC },
        { "AUDIO_FORMAT_EVRCB", AUDIO_FORMAT_EVRCB },
        { "AUDIO_FORMAT_EVRCWB", AUDIO_FORMAT_EVRCWB },
        { "AUDIO_FORMAT_EVRCNW", AUDIO_FORMAT_EVRCNW },
        { "AUDIO_FORMAT_SBC", AUDIO_FORMAT_SBC },
        { "AUDIO_FORMAT_LDAC", AUDIO_FORMAT_LDAC },
        { "AUDIO_FORMAT_LHDC", AUDIO_FORMAT_LHDC },
        { "AUDIO_FORMAT_LHDC_LL", AUDIO_FORMAT_LHDC_LL },
        { "AUDIO_FORMAT_APTX", AUDIO_FORMAT_APTX },
        { "AUDIO_FORMAT_APTX_HD", AUDIO_FORMAT_APTX_HD },
        { "AUDIO_FORMAT_APTX_ADAPTIVE", AUDIO_FORMAT_APTX_ADAPTIVE },
        { "AUDIO_FORMAT_APTX_TWSP", AUDIO_FORMAT_APTX_TWSP },
        { "AUDIO_FORMAT_LC3", AUDIO_FORMAT_LC3 },
        { nullptr, (audio_format_t)0 },
    };

/*
 * android::deviceFromString(const std::string&, audio_devices_t&) as shipped
 * in Android 11 libmedia_helper (audio_devices_t was uint32_t there, hence the
 * unsigned int& mangling). Removed in Android 14; libeffectsconfig.so still
 * calls it. Returns NO_ERROR(0) on match, BAD_VALUE(-1) otherwise.
 */
int32_t deviceFromString(const std::string &literal, unsigned int &device) {
    for (size_t i = 0; TypeConverter<OutputDeviceTraits>::mTable[i].literal; ++i)
        if (literal == TypeConverter<OutputDeviceTraits>::mTable[i].literal) {
            device = static_cast<unsigned int>(TypeConverter<OutputDeviceTraits>::mTable[i].value);
            return 0;
        }
    for (size_t i = 0; TypeConverter<InputDeviceTraits>::mTable[i].literal; ++i)
        if (literal == TypeConverter<InputDeviceTraits>::mTable[i].literal) {
            device = static_cast<unsigned int>(TypeConverter<InputDeviceTraits>::mTable[i].value);
            return 0;
        }
    return -1;
}

} // namespace android
