/*
 * ABI-compat shim for the stock (Android 11/12) Samsung Codec2 component store
 * (libSecC2ComponentStore.so) on Android 15.
 *
 * The blob was built against the older C2PlatformStorePluginLoader /
 * C2PooledBlockPool signatures. Android 15 appended a std::function deleter
 * parameter to createAllocator/createBlockPool and a BufferPoolVer parameter
 * to the C2PooledBlockPool ctor, so the blob fails to link with:
 *   cannot locate symbol "C2PlatformStorePluginLoader15createAllocatorEj..."
 *
 * We do NOT include the real A15 headers for these two classes; instead we
 * re-declare them minimally so the mangled names we emit match exactly what
 * the blob references, and forward to the real implementations in
 * libcodec2_vndk. Only the missing legacy symbols are defined here.
 */

#include <cstdint>
#include <functional>
#include <memory>
#include <new>

#include <C2.h>

/*
 * Minimal declarations of the codec2 types the signatures reference. Only the
 * nested id types matter for the mangled names; completeness is irrelevant here
 * (we never instantiate these ourselves — the real objects come from
 * libcodec2_vndk), so we avoid pulling in the full C2Buffer.h include chain.
 */
class C2Allocator {
public:
    typedef uint32_t id_t;
};

class C2BlockPool {
public:
    typedef uint64_t local_id_t;
};

/* ------------------------------------------------------------------ *
 * C2PlatformStorePluginLoader
 *
 * Blob wants (old, no deleter):
 *   createAllocator(id_t, shared_ptr<C2Allocator>*)
 *   createBlockPool(id_t, local_id_t, shared_ptr<C2BlockPool>*)
 * A15 provides (new, with deleter):
 *   createAllocator(id_t, shared_ptr<C2Allocator>*, function<void(C2Allocator*)>)
 *   createBlockPool(id_t, local_id_t, shared_ptr<C2BlockPool>*, function<void(C2BlockPool*)>)
 * ------------------------------------------------------------------ */
class C2PlatformStorePluginLoader {
public:
    static const std::unique_ptr<C2PlatformStorePluginLoader>& GetInstance();

    // Real A15 signatures — resolved from libcodec2_vndk.
    c2_status_t createAllocator(::C2Allocator::id_t allocatorId,
                                std::shared_ptr<C2Allocator>* allocator,
                                std::function<void(C2Allocator*)> deleter);
    c2_status_t createBlockPool(::C2Allocator::id_t allocatorId,
                                ::C2BlockPool::local_id_t blockPoolId,
                                std::shared_ptr<C2BlockPool>* pool,
                                std::function<void(C2BlockPool*)> deleter);

    // Legacy signatures the blob references — we define these.
    c2_status_t createAllocator(::C2Allocator::id_t allocatorId,
                                std::shared_ptr<C2Allocator>* allocator);
    c2_status_t createBlockPool(::C2Allocator::id_t allocatorId,
                                ::C2BlockPool::local_id_t blockPoolId,
                                std::shared_ptr<C2BlockPool>* pool);
};

c2_status_t C2PlatformStorePluginLoader::createAllocator(
        ::C2Allocator::id_t allocatorId, std::shared_ptr<C2Allocator>* allocator) {
    return createAllocator(allocatorId, allocator,
                           std::function<void(C2Allocator*)>(std::default_delete<C2Allocator>()));
}

c2_status_t C2PlatformStorePluginLoader::createBlockPool(
        ::C2Allocator::id_t allocatorId, ::C2BlockPool::local_id_t blockPoolId,
        std::shared_ptr<C2BlockPool>* pool) {
    return createBlockPool(allocatorId, blockPoolId, pool,
                           std::function<void(C2BlockPool*)>(std::default_delete<C2BlockPool>()));
}

/* ------------------------------------------------------------------ *
 * C2PooledBlockPool
 *
 * Blob wants (old, 2-arg):
 *   C2PooledBlockPool(const shared_ptr<C2Allocator>&, local_id_t)
 * A15 provides (new, 3-arg):
 *   C2PooledBlockPool(const shared_ptr<C2Allocator>&, local_id_t, BufferPoolVer)
 * Old behaviour corresponds to BufferPoolVer::VER_HIDL.
 *
 * local_id_t is uint64_t (mangles to 'm'), and the class name alone determines
 * the ctor's mangled name — the base class is irrelevant for the symbol — so we
 * declare it standalone and use placement new to run the real A15 ctor on this.
 * ------------------------------------------------------------------ */
class C2PooledBlockPool {
public:
    enum BufferPoolVer : int {
        VER_HIDL = 0,
        VER_AIDL2,
    };

    // Real A15 ctor — resolved from libcodec2_vndk.
    C2PooledBlockPool(const std::shared_ptr<C2Allocator>& allocator,
                      const uint64_t localId, BufferPoolVer ver);

    // Legacy 2-arg ctor the blob references — we define this.
    C2PooledBlockPool(const std::shared_ptr<C2Allocator>& allocator,
                      const uint64_t localId);
};

C2PooledBlockPool::C2PooledBlockPool(
        const std::shared_ptr<C2Allocator>& allocator, const uint64_t localId) {
    new (this) C2PooledBlockPool(allocator, localId, VER_HIDL);
}
