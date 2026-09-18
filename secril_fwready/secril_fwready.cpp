// SPDX-License-Identifier: Apache-2.0
//
// Schreibt die Vendor-Konfiguration "FW_READY" ueber ISehRadio.
// Samsungs libsec-ril haelt eingehende SMS (und andere Unsol-Responses
// mit spezifischen IDs) in einer Pending-Queue, bis die Konfiguration
// FW_READY vom Framework gesetzt wurde. Stock macht das die Samsung-
// Telephony-Schicht; auf LineageOS fehlt sie, also schickt dieser
// Dienst den Schreibvorgang nach.

#include <hidl/HidlTransportSupport.h>
#include <utils/Log.h>
#include <vendor/samsung/hardware/radio/2.2/ISehRadio.h>
#include <vendor/samsung/hardware/radio/2.2/types.h>
#include <unistd.h>
#include <string>

using android::sp;
using android::hardware::hidl_vec;
using vendor::samsung::hardware::radio::V2_2::ISehRadio;
using vendor::samsung::hardware::radio::V2_2::SehVendorConfiguration;

static const char* kSlots[] = {"slot1", "slot2"};

static void sendFwReady(const sp<ISehRadio>& radio, const char* slot, int counter) {
    hidl_vec<SehVendorConfiguration> cfgs(1);
    cfgs[0].name = "FW_READY";
    cfgs[0].value = std::to_string(counter);
    auto ret = radio->setVendorSpecificConfiguration(1, cfgs);
    if (ret.isOk()) {
        ALOGI("FW_READY (%d) an ISehRadio/%s gesendet", counter, slot);
    } else {
        ALOGE("setVendorSpecificConfiguration an %s fehlgeschlagen: %s",
              slot, ret.description().c_str());
    }
}

int main() {
    int counter = 0;
    for (;;) {
        bool any = false;
        for (const char* slot : kSlots) {
            sp<ISehRadio> radio = ISehRadio::getService(slot);
            if (radio != nullptr) {
                sendFwReady(radio, slot, ++counter);
                any = true;
            }
        }
        if (!any) {
            ALOGI("ISehRadio noch nicht verfuegbar, retry in 2s");
            sleep(2);
        } else {
            // Erneut senden schadet nicht: EnableUnsolResponse kehrt sofort
            // zurueck, wenn der Schalter bereits gesetzt ist. Ein rotierender
            // Wert umgeht die IsValueChanged-Dedup, falls rild neu startet
            // und der Schalter zurueckgesetzt wurde.
            sleep(60);
        }
    }
    return 0;
}
