/*
 * Real-implementation half of the C2 compat shim. This translation unit sees
 * the actual Android 14 class declarations; the mangled-name replicas live in
 * C2CompatShim.cpp.
 */

#include <C2BufferPriv.h>
#include <C2PlatformStorePluginLoader.h>
#include <memory>

std::shared_ptr<C2BlockPool> c2compat_createPool(
        const std::shared_ptr<C2Allocator> &allocator, C2BlockPool::local_id_t localId) {
    return std::make_shared<C2PooledBlockPool>(allocator, localId, C2PooledBlockPool::VER_HIDL);
}

c2_status_t c2compat_createAllocator(void *self, C2Allocator::id_t id,
        std::shared_ptr<C2Allocator> *allocator) {
    return static_cast<C2PlatformStorePluginLoader *>(self)->createAllocator(id, allocator);
}

c2_status_t c2compat_createBlockPool(void *self, C2Allocator::id_t id,
        C2BlockPool::local_id_t poolId, std::shared_ptr<C2BlockPool> *pool) {
    return static_cast<C2PlatformStorePluginLoader *>(self)->createBlockPool(id, poolId, pool);
}
