/*
 * FrucFacade base-class method bodies.
 *
 * FrucListener's and FrucCallbackInterface's own vtables are emitted
 * here (their key functions are the destructors). The methods are
 * never dispatched on a standalone base object - GraphicBufferSource
 * overrides all of them - but the vtable/typeinfo emission requires
 * out-of-line definitions.
 */

#define LOG_TAG "FrucFacade"
#include <utils/Log.h>
#include <media/stagefright/bqhelper_samsung/FrucFacade.h>

namespace FrucFacade {

FrucListener::~FrucListener() {}
void FrucListener::onInputPrepare(FrucFrame *) {}
void FrucListener::onInputProcessed(const FrucFrame *) {}
void FrucListener::onOutputStorageRequired(FrucFrame *) {}
void FrucListener::onOutputReady(const FrucFrame *) {}
void FrucListener::onError() {}

FrucCallbackInterface::~FrucCallbackInterface() {}
void FrucCallbackInterface::onInputPrepare(FrucFrame *) {}
void FrucCallbackInterface::onInputProcessed(const FrucFrame *) {}
void FrucCallbackInterface::onOutputStorageRequired(FrucFrame *) {}
void FrucCallbackInterface::onOutputReady(const FrucFrame *) {}
void FrucCallbackInterface::onError() {}

}  // namespace FrucFacade
