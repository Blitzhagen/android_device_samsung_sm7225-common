#!/usr/bin/env python3
"""
Binary patch for vendor/lib/libstagefright_bufferqueue_helper_vendor.so

The prebuilt library was compiled against the Android 11 framework ABI.
On Android 14 it must translate the wire-format QueueBufferInput written
by the A14 (BLAST) client into native IGraphicBufferProducer calls for the
in-process BufferQueueProducer.  Several framework object layouts changed
between the releases, which broke the conversion path and made video
recording fail (queueBuffer returning -EINVAL/-ENOMEM and later a crash
in BufferItem::~BufferItem -> shared_ptr<FenceTime> release).

Patches applied (offsets are virtual addresses; file offset = vaddr-0x1000):

  1. hidl_vec<Rect> mSize read: vec+8 -> vec+4, 3 sites.
     A14 hidl_vec is {mBuffer, mSize, mOwns} while the blob expects
     {mBuffer, pad, mSize, mOwns}; reading +8 returned mOwns(=1) instead
     of mSize(=0) and made flatten() fail with NO_MEMORY.

  2. QueueBufferInput damage-vec size read: input+72 -> input+68, 2 sites
     (conversion::flatten and convertTo size checks).

  3. QueueBufferInput::unflatten: continue when Region::unflatten rejects
     an empty surfaceDamage (cbz -> unconditional branch).  A14/BLAST
     sends empty damage regions which the old code rejected with
     BAD_VALUE.

  4. Fence allocation: new(8) -> new(16); manual {count,fd} init replaced
     by a call to the imported A14 Fence::Fence(int) constructor, which
     writes the vptr, mCount(+4) and mFenceFd(+8) the A14 libui expects.

  5. Inlined sp<Fence> inc/dec in QueueBufferInput::unflatten operated on
     fence+0 (A11 mCount offset = A14 vptr) corrupting the vtable pointer.
     Retargeted to fence+4 (A14 mCount).  ldrex has no offset variant, so
     the atomic loops become plain ldr/str; the calls are serialized per
     producer, which keeps this safe.

  6. The vendored destroy path reads/closes the fence fd at +4 (A11) and
     stores -1 at +4; retargeted to +8 (A14 mFenceFd).

  7. TWGraphicBufferProducer::dequeueBuffer (+0x2d4): the vendored fence
     release sequence calls close() on an fd that is owned by an A14
     unique_fd elsewhere (the fd was taken out of the incoming
     hidl_handle without dup(), i.e. double ownership).  On A14 fdsan
     aborts the process: 'attempted to close file descriptor 1,
     expected to be owned by unique_fd ..., actually unowned'
     -> media.codec dies -> -EPIPE to the camera stream.
     Patch makes the `fd == -1` check always true (adds r0,r6,#1 ->
     movs r0,#0), which skips only the close; refcount decrement,
     the -1 store and object deletion still run.  The fd stays owned
     by its real unique_fd owner and is released there.

Usage: patch_bqh_vendor.py <path-to-library>
"""

import sys

V2F = 0x1000  # file offset = virtual address - 0x1000

# (vaddr, old_bytes_hex, new_bytes_hex)
PATCHES = [
    # --- 1. hidl_vec<Rect> mSize +8 -> +4 ---
    (0x2FCD0, '8368', '4368'),  # ldr r3,[r0,#8] -> ldr r3,[r0,#4]
    (0x2FCF2, '8368', '4368'),
    (0x2FD32, '8668', '4668'),  # ldr r6,[r0,#8] -> ldr r6,[r0,#4]
    # --- 2. QueueBufferInput mSize +72 -> +68 ---
    (0x300F0, 'a96c', '696c'),        # ldr r1,[r5,#72] -> #68
    (0x2FE9E, 'daf84800', 'daf84400'),# ldr.w r0,[sl,#72] -> #68
    # --- 3. Region::unflatten: continue on error ---
    (0x334CE, '10b1', '02e0'),        # cbz r0 -> b.n (skip error return)
    # --- 4. Fence alloc + ctor ---
    (0x333FC, '0820', '1020'),        # movs r0,#8 -> movs r0,#16
    # mov r0,r4 ; mov.w r1,#-1 ; blx 0x37a30 (Fence::Fence(int))
    (0x33404, '4ff0ff300021c4e90010', '20464ff0ff3104f012eb'),
    # --- 5. sp<Fence> inc/dec -> mCount at +4 ---
    # ldr r0,[r4,#4]; adds r0,#1; str r0,[r4,#4]; nop x4
    (0x33420, '54e8000f013044e800010029f8d1',
              '60680130606000bf00bf00bf00bf'),
    # ldr r0,[r6,#4]; subs r1,r0,#1; str r1,[r6,#4]; nop x4
    (0x33434, '56e8000f411e46e80012002af8d1',
              '7068411e716000bf00bf00bf00bf'),
    # --- 6. fd accesses +4 -> +8 ---
    (0x33456, '52f8048f', '52f8088f'),  # ldr.w r8,[r2,#4]! -> #8
    (0x3348C, '7060', 'b060'),          # str r0,[r6,#4] -> [r6,#8]
    # --- 7. dequeueBuffer: skip close on fd owned by another object ---
    (0x2264A, '701c', '0020'),          # adds r0,r6,#1 -> movs r0,#0
]


def main(path):
    with open(path, 'rb') as f:
        data = bytearray(f.read())
    for vaddr, old, new in PATCHES:
        off = vaddr - V2F
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
