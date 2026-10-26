#!/usr/bin/env python3
"""
Binary patch for vendor/lib64/libsec-ril.so

Samsung's libsec-ril only emits the uiccApplicationsEnablementChanged
unsolicited indication when ro.build.version.oneui is set and reports a
version >= 60100 (OneUI 6.1). The prop is unset on LineageOS, so after
RIL_REQUEST_ENABLE_UICC_APPLICATIONS false the RIL never reports the new
state. The framework's mUiccApplicationsEnabled stays true, the re-enable
request is then skipped as a no-op and the SIM stays dead until reboot.

Both gates follow the same shape at the OneUI version check:

    mov  w8, #0xeac4      ; 60100
    cmp  wN, w8
    b.lt skip_emit

NOP the b.lt so the indication is always sent. Pattern and fix mirror
the upstream change for sm8550-common / sm7325-common / sm7125-common
(LineageOS Gerrit I866954e756cebaae7a6816c9738eb486659a87ed); our blob
carries the identical code sequence.

Usage: patch_sec_ril.py <path-to-library>
"""

import sys

# (old_bytes_hex, new_bytes_hex): each pattern must match exactly once
PATCHES = [
    # SimManager::UpdateState gate
    ('88589d521f00086bab010054', '88589d521f00086b1f2003d5'),
    # OnSetSimOnOffDone gate
    ('88589d52ff02086bab010054', '88589d52ff02086b1f2003d5'),
]


def main(path):
    with open(path, 'rb') as f:
        data = bytearray(f.read())
    for old, new in PATCHES:
        old_b, new_b = bytes.fromhex(old), bytes.fromhex(new)
        assert len(old_b) == len(new_b)
        off = data.find(old_b)
        assert off >= 0 and data.find(old_b, off + 1) < 0, \
            f'pattern {old} not unique (off={off:#x})'
        data[off:off + len(new_b)] = new_b
    with open(path, 'wb') as f:
        f.write(data)


if __name__ == '__main__':
    main(sys.argv[1])
