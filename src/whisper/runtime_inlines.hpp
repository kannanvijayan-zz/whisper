#ifndef WHISPER__RUNTIME_INLINES_HPP
#define WHISPER__RUNTIME_INLINES_HPP

#include "runtime.hpp"
#include "spew.hpp"

#include <new>

namespace Whisper {


//
// AllocationContext
//

template <typename ObjT, typename... Args>
inline Result<ObjT*>
AllocationContext::create(Args const&... args)
{
    static_assert(HeapTraits<ObjT>::Specialized,
                  "HeapTraits not specialized for ObjT.");
    static_assert(!HeapTraits<ObjT>::VarSized,
                  "Use createSized* methods to allocate varsized objects.");

    uint32_t size = sizeof(ObjT);

    // Allocate the space for the object.
    constexpr HeapFormat FMT = HeapTraits<ObjT>::Format;
    typedef typename HeapFormatTraits<FMT>::Type TRACE_TYPE;
    constexpr bool TRACED = ! TraceTraits<TRACE_TYPE>::IsLeaf;
    uint8_t* mem = allocate<TRACED>(size, FMT);
    if (!mem)
        return ErrorVal();

    // Construct object in memory.
    new (mem) ObjT(std::forward<Args const&>(args)...);

    return OkVal(reinterpret_cast<ObjT*>(mem));
}

template <typename ObjT, typename... Args>
inline Result<ObjT*>
AllocationContext::createSized(uint32_t size, Args const&... args)
{
    static_assert(HeapTraits<ObjT>::Specialized,
                  "HeapTraits not specialized for ObjT.");
    static_assert(HeapTraits<ObjT>::VarSized,
                  "Explicitly sized create called for fixed-size object.");

    WH_ASSERT(size >= sizeof(ObjT));

    // Allocate the space for the object.
    constexpr HeapFormat FMT = HeapTraits<ObjT>::Format;
    typedef typename HeapFormatTraits<FMT>::Type TRACE_TYPE;
    constexpr bool TRACED = ! TraceTraits<TRACE_TYPE>::IsLeaf;
    uint8_t* mem = allocate<TRACED>(size, FMT);
    if (!mem)
        return ErrorVal();

    // Construct object in memory.
    new (mem) ObjT(std::forward<Args const&>(args)...);

    return OkVal(reinterpret_cast<ObjT*>(mem));
}

template <bool Traced>
inline uint8_t*
AllocationContext::allocate(uint32_t size, HeapFormat fmt)
{
    uint32_t allocSize = AlignIntUp<uint32_t>(size, Slab::AllocAlign)
                         + sizeof(HeapHeader);

    // Allocate the space.
    uint8_t* mem = Traced ? slab_->allocateHead(allocSize)
                          : slab_->allocateTail(allocSize);

    if (!mem) {
        if (cx_->suppressGC()) {
            cx_->setError(RuntimeError::MemAllocFailed);
            return nullptr;
        }

        cx_->setError(RuntimeError::MemAllocFailed);
        return nullptr;
    }

    SpewMemoryNote("Allocated %d bytes from %p, leaving %d bytes",
                   size, slab_, slab_->unallocatedBytes());

    // Figure out the card number.
    uint32_t cardNo = slab_->calculateCardNumber(mem);

    // Initialize the header.
    HeapHeader* hdr = new (mem) HeapHeader(fmt, slab_->gen(), cardNo, size);
    return reinterpret_cast<uint8_t*>(hdr->payload());
}


} // namespace Whisper

#endif // WHISPER__RUNTIME_INLINES_HPP
