#ifndef WHISPER__RUNTIME_INLINES_HPP
#define WHISPER__RUNTIME_INLINES_HPP

#include "runtime.hpp"

#include <new>

namespace Whisper {


//
// AllocationContext
//

template <typename ObjT, typename... Args>
inline ObjT *
AllocationContext::create(Args... args)
{
    static_assert(GC::HeapTraits<ObjT>::Specialized,
                  "GC::HeapTraits not specialized for ObjT.");
    return createFlagged<ObjT, Args...>(0, std::forward<Args>(args)...);
}

template <typename ObjT, typename... Args>
inline ObjT *
AllocationContext::createFlagged(uint8_t flags, Args... args)
{
    static_assert(GC::HeapTraits<ObjT>::Specialized,
                  "GC::HeapTraits not specialized for ObjT.");
    WH_ASSERT(flags <= GC::AllocHeader::UserDataMax);

    uint32_t size = GC::HeapTraits<ObjT>::template SizeOf<Args...>(args...);
    WH_ASSERT(size >= sizeof(ObjT));

    // Allocate the space for the object.
    constexpr GC::AllocFormat FMT = GC::HeapTraits<ObjT>::Format;
    typedef typename GC::AllocFormatTraits<FMT>::Type TRACE_TYPE;
    constexpr bool TRACED = ! GC::TraceTraits<TRACE_TYPE>::IsLeaf;
    uint8_t *mem = allocate<TRACED>(size, FMT, flags);
    if (!mem)
        return nullptr;

    // Construct object in memory.
    new (mem) ObjT(std::forward<Args>(args)...);

    return reinterpret_cast<ObjT *>(mem);
}

template <bool Traced>
inline uint8_t *
AllocationContext::allocate(uint32_t size, GC::AllocFormat fmt, uint8_t flags)
{
    WH_ASSERT(flags <= GC::AllocHeader::UserDataMax);

    uint32_t allocSize = AlignIntUp<uint32_t>(size, Slab::AllocAlign)
                         + sizeof(GC::AllocFormat);

    // Allocate the space.
    uint8_t *mem = Traced ? slab_->allocateHead(allocSize)
                          : slab_->allocateTail(allocSize);

    if (!mem) {
        if (cx_->suppressGC())
            return nullptr;

        WH_ASSERT(!"GC infrastructure not yet implemented.");
        return nullptr;
    }

    // Figure out the card number.
    uint32_t cardNo = slab_->calculateCardNumber(mem);

    // Initialize the header.
    GC::AllocHeader *hdr = new (mem) GC::AllocHeader(fmt, slab_->gen(),
                                                     cardNo, size);
    return reinterpret_cast<uint8_t *>(hdr->payload());
}


} // namespace Whisper

#endif // WHISPER__RUNTIME_INLINES_HPP
