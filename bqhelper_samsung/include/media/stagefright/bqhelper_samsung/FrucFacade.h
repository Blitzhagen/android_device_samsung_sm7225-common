/*
 * Samsung FrucFacade type declarations for the vendor
 * libstagefright_bufferqueue_helper ABI.
 *
 * Reconstructed from libstagefright_bufferqueue_helper_vendor.so /
 * libstagefright_omx_vendor.so (Android 11 vendor blobs). The class
 * layouts below reproduce the exact vtable layout of the blobs:
 *
 *   GraphicBufferSource vtable (verified against the blob):
 *     [D1, D0][RefBase x4][onInputPrepare, onInputProcessed,
 *      onOutputStorageRequired, onOutputReady, onError]   <- primary (FrucListener)
 *     [same 5 methods as Thn8 thunks][D1, D0]             <- secondary @+8
 *
 * FrucFrame is opaque to this implementation (all Fruc methods only
 * receive pointers to it).
 */

#ifndef FRUC_FACADE_H_
#define FRUC_FACADE_H_

#include <utils/RefBase.h>

namespace FrucFacade {

struct FrucFrame;

/*
 * Primary listener interface of GraphicBufferSource (sits at offset 0,
 * inherits RefBase). FrucListener carries no data members of its own:
 * it is exactly RefBase-sized (8 bytes on 32-bit), which places the
 * secondary FrucCallbackInterface subobject at object offset +8 and
 * yields the blob's secondary vtable offset_to_top of -8.
 */
class FrucListener : public android::RefBase {
public:
    virtual void onInputPrepare(FrucFrame *frame);
    virtual void onInputProcessed(const FrucFrame *frame);
    virtual void onOutputStorageRequired(FrucFrame *frame);
    virtual void onOutputReady(const FrucFrame *frame);
    virtual void onError();
    virtual ~FrucListener();
};

/*
 * Secondary interface of GraphicBufferSource (subobject at offset +8).
 * Plain polymorphic interface without RefBase. The virtual destructor
 * is declared after the methods, which places the D1/D0 thunk entries
 * at the end of the secondary vtable, exactly as in the blob.
 */
class FrucCallbackInterface {
public:
    virtual void onInputPrepare(FrucFrame *frame);
    virtual void onInputProcessed(const FrucFrame *frame);
    virtual void onOutputStorageRequired(FrucFrame *frame);
    virtual void onOutputReady(const FrucFrame *frame);
    virtual void onError();
    virtual ~FrucCallbackInterface();
};

}  // namespace FrucFacade

#endif  // FRUC_FACADE_H_
