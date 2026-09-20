#!/usr/bin/env python3
"""
Binary patch for vendor/lib/hw/audio.primary.lito.so

The Samsung audio HAL selects the Bluetooth SCO backend sample rate and the
"-wb" device/usecase variants from an internal wideband flag (adev+0x1c0).
Stock firmware sets that flag from the parameter key "g_sco_samplerate=<Hz>"
(emitted by Samsung's own framework).  AOSP instead sends "bt_wbs=on/off"
via AudioManager.setBluetoothHeadsetProperties() whenever the HFP codec is
negotiated (mSBC/WBS -> on, CVSD -> off).

Without this patch the flag stays 0, so mSBC links were run on the 8 kHz
narrowband backend (severely distorted robotic audio).  The patch makes
sec_set_parameters() evaluate "bt_wbs" instead of "g_sco_samplerate", so the
flag tracks the codec negotiated by the Bluetooth stack dynamically.

sec_set_parameters (vaddr 0x90160) originally does:

    str_parms_get_int(parms, "g_sco_samplerate", &v)   // v at [sp,#0x24]
    if (ret >= 0) {
        str_parms_del(parms, "g_sco_samplerate");
        flag = (v == 16000);
        if (flag != adev->wbs) { log; adev->wbs = flag; }
    }

Patched to (ARM; .text file offset = vaddr - 0x1000, .rodata = vaddr):

    str_parms_get_str(parms, "bt_wbs", buf, 256)       // buf at [sp,#0x24]
    if (ret >= 0) {
        // str_parms_del skipped; leaving the key in parms is harmless
        flag = (buf[1] == 'n');                        // "on" vs "off"
        if (flag != adev->wbs) { log; adev->wbs = flag; }
    }

The block is reshuffled by one instruction so the del() call site makes room
for "mov r3,#256" (get_str needs the buffer length in r3; the original
get_int call had no 4th argument, so r3 was uninitialized).  The literal-pool
offsets at 0x91374/0x91378 keep their original pc-relative encoding; the key
pointer itself is now the "bt_wbs" string.

Also the key string "g_sco_samplerate" at 0x24848 is changed to "bt_wbs",
which additionally makes sec_get_parameters() report the state under the
AOSP-standard key.

Usage: patch_audio_primary_lito.py <path-to-library>
"""

import sys

# .text: file offset = vaddr - 0x1000 ; .rodata: file offset = vaddr
TEXT_BASE = 0x310F0

def v2f(vaddr):
    return vaddr - 0x1000 if vaddr >= TEXT_BASE else vaddr

# (vaddr, old_bytes_hex, new_bytes_hex)
PATCHES = [
    # --- key string: "g_sco_samplerate" -> "bt_wbs" (.rodata, file off = vaddr) ---
    (0x24848, '675f73636f5f73616d706c6572617465',
              '62745f77627300000000000000000000'),
    # --- set up len arg then call str_parms_get_str ---
    (0x903C8, '4c3f00eb', '013ca0e3'),  # bl str_parms_get_int -> mov r3,#256
    (0x903CC, '000050e3', 'a34000eb'),  # cmp r0,#0 -> bl str_parms_get_str
    (0x903D0, '3800004a', '000050e3'),  # bmi skip -> cmp r0,#0
    (0x903D4, '9c1f9fe5', '3700004a'),  # ldr r1,[key] -> bmi 0x904b8
    (0x903D8, '0400a0e1', '0000a0e1'),  # mov r0,r4 -> nop
    (0x903DC, '01108fe0', '0000a0e1'),  # add r1,pc,r1 -> nop
    (0x903E0, '2e4500eb', '0000a0e1'),  # bl str_parms_del -> nop
    # --- evaluate buf[1]: 'n' ("on") -> wideband ---
    (0x903E4, '24009de5', '2500dde5'),  # ldr r0,[sp,#0x24] -> ldrb r0,[sp,#0x25]
    (0x903EC, 'fa0d40e2', '6e0040e2'),  # sub r0,#16000 -> sub r0,#'n'
]


def main(path):
    with open(path, 'rb') as f:
        data = bytearray(f.read())
    for vaddr, old, new in PATCHES:
        off = v2f(vaddr)
        old_b, new_b = bytes.fromhex(old), bytes.fromhex(new)
        cur = bytes(data[off:off + len(old_b)])
        if cur == new_b:
            continue  # already patched
        if cur != old_b:
            raise SystemExit(
                f'{path}: patch at {vaddr:#x}: expected {old} got {cur.hex()}')
        data[off:off + len(new_b)] = new_b
    with open(path, 'wb') as f:
        f.write(data)
    print(f'{path}: {len(PATCHES)} patches applied')


if __name__ == '__main__':
    main(sys.argv[1])
