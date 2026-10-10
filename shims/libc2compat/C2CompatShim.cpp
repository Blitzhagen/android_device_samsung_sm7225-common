/*
 * ABI-compat shim for the stock (Android 12-era) Samsung C2 stack on Android 14.
 *
 * libSecC2ComponentStore.so was built against an older libcodec2_vndk where
 *   - C2PlatformStorePluginLoader::createAllocator/createBlockPool did not yet
 *     take the trailing deleter argument, and
 *   - C2PooledBlockPool had no BufferPoolVer ctor parameter.
 * The blobs fail to link: "cannot locate symbol ...createAllocator/createBlockPool/
 * C2PooledBlockPoolC1".
 *
 * This shim re-declares the classes with the old member signatures so the
 * mangled names match, and forwards to the real Android 14 implementations.
 * The replica C2PooledBlockPool keeps the old 40-byte footprint (callers
 * allocate a fixed-size buffer) and delegates every virtual call to a real
 * heap-allocated C2PooledBlockPool.
 */

#include <C2.h>
#include <C2Buffer.h>
#include <memory>

// Implemented in RealCodec2.cpp, which sees the real class definitions.
extern std::shared_ptr<C2BlockPool> c2compat_createPool(
        const std::shared_ptr<C2Allocator> &allocator, C2BlockPool::local_id_t localId);
extern c2_status_t c2compat_createAllocator(void *self, C2Allocator::id_t id,
        std::shared_ptr<C2Allocator> *allocator);
extern c2_status_t c2compat_createBlockPool(void *self, C2Allocator::id_t id,
        C2BlockPool::local_id_t poolId, std::shared_ptr<C2BlockPool> *pool);

// Replica of the old C2PooledBlockPool ABI surface. Only the mangled names
// must match; the object is a thin adapter over a real pool instance.
class C2PooledBlockPool : public C2BlockPool {
public:
    C2PooledBlockPool(const std::shared_ptr<C2Allocator> &allocator, local_id_t localId);
    ~C2PooledBlockPool() override = default;

    local_id_t getLocalId() const override;
    C2Allocator::id_t getAllocatorId() const override;
    c2_status_t fetchLinearBlock(uint32_t capacity, C2MemoryUsage usage,
            std::shared_ptr<C2LinearBlock> *block) override;
    c2_status_t fetchGraphicBlock(uint32_t width, uint32_t height, uint32_t format,
            C2MemoryUsage usage, std::shared_ptr<C2GraphicBlock> *block) override;

private:
    std::shared_ptr<C2BlockPool> mPool;
};

C2PooledBlockPool::C2PooledBlockPool(
        const std::shared_ptr<C2Allocator> &allocator, local_id_t localId)
        : mPool(c2compat_createPool(allocator, localId)) {}

C2BlockPool::local_id_t C2PooledBlockPool::getLocalId() const {
    return mPool->getLocalId();
}

C2Allocator::id_t C2PooledBlockPool::getAllocatorId() const {
    return mPool->getAllocatorId();
}

c2_status_t C2PooledBlockPool::fetchLinearBlock(uint32_t capacity, C2MemoryUsage usage,
        std::shared_ptr<C2LinearBlock> *block) {
    return mPool->fetchLinearBlock(capacity, usage, block);
}

c2_status_t C2PooledBlockPool::fetchGraphicBlock(uint32_t width, uint32_t height,
        uint32_t format, C2MemoryUsage usage, std::shared_ptr<C2GraphicBlock> *block) {
    return mPool->fetchGraphicBlock(width, height, format, usage, block);
}

// Replica of the old two/three-argument loader methods.
class C2PlatformStorePluginLoader {
public:
    c2_status_t createAllocator(C2Allocator::id_t allocatorId,
            std::shared_ptr<C2Allocator> *allocator);
    c2_status_t createBlockPool(C2Allocator::id_t allocatorId,
            C2BlockPool::local_id_t poolId, std::shared_ptr<C2BlockPool> *pool);
};

c2_status_t C2PlatformStorePluginLoader::createAllocator(C2Allocator::id_t allocatorId,
        std::shared_ptr<C2Allocator> *allocator) {
    return c2compat_createAllocator(this, allocatorId, allocator);
}

c2_status_t C2PlatformStorePluginLoader::createBlockPool(C2Allocator::id_t allocatorId,
        C2BlockPool::local_id_t poolId, std::shared_ptr<C2BlockPool> *pool) {
    return c2compat_createBlockPool(this, allocatorId, poolId, pool);
}
