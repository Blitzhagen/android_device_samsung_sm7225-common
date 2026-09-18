/*
 * Stock (Android 11) vendor blobs link the pre-Android-14 signatures of
 * android::GraphicBufferMapper::lock/unlock which no longer exist in the
 * current libui:
 *   _ZN7android19GraphicBufferMapper4lockEPK13native_handlejRKNS_4RectEPPvPiS9_
 *   _ZN7android19GraphicBufferMapper6unlockEPK13native_handle
 * This shim exports them under their exact mangled names and forwards to the
 * current API. It is preloaded into vendor services that load affected blobs.
 */

#include <ui/GraphicBufferMapper.h>
#include <ui/Rect.h>
#include <utils/Errors.h>

using android::GraphicBufferMapper;
using android::Rect;
using android::status_t;

extern "C" status_t
_ZN7android19GraphicBufferMapper4lockEPK13native_handlejRKNS_4RectEPPvPiS9_(
        const native_handle_t* handle, uint32_t usage, const Rect& bounds,
        void** vaddr, int* outBytesPerPixel, int* outBytesPerStride) {
    auto res = GraphicBufferMapper::get().lock(
            const_cast<native_handle_t*>(handle), usage, bounds);
    if (!res.ok()) {
        return res.error().asStatus();
    }
    *vaddr = res->address;
    if (outBytesPerPixel) {
        *outBytesPerPixel = res->bytesPerPixel;
    }
    if (outBytesPerStride) {
        *outBytesPerStride = res->bytesPerStride;
    }
    return android::OK;
}

extern "C" status_t
_ZN7android19GraphicBufferMapper6unlockEPK13native_handle(
        const native_handle_t* handle) {
    return GraphicBufferMapper::get().unlock(const_cast<native_handle_t*>(handle));
}
