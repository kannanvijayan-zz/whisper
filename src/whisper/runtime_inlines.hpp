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
    return createFlagged<ObjT, Args...>(0, std::forward<Args>(args)...);
}

template <typename ObjT, typename... Args>
inline ObjT *
AllocationContext::createFlagged(uint8_t flags, Args... args)
{
    WH_ASSERT(flags <= AllocHeader::UserDataMax);

    uint32_t size = SlabThingTraits<ObjT>::template SIZE_OF<Args...>(args...);
    WH_ASSERT(size >= sizeof(ObjT));

    // Allocate the space for the object.
    constexpr AllocFormat FMT = AllocTraits<ObjT>::FORMAT;
    constexpr bool TRACED = AllocFormatTraits<FMT>::TRACED;
    uint8_t *mem = allocate<TRACED>(size, FMT, flags);
    if (!mem)
        return nullptr;

    // Construct object in memory.
    new (mem) ObjT(std::forward<Args>(args)...);

    return reinterpret_cast<ObjT *>(mem);
}

template <bool Traced>
inline uint8_t *
AllocationContext::allocate(uint32_t size, AllocFormat fmt, uint8_t flags)
{
    WH_ASSERT(flags <= AllocHeader::UserDataMax);

    uint32_t allocSize = AlignIntUp<uint32_t>(size, Slab::AllocAlign)
                         + sizeof(AllocFormat);

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
    AllocHeader *hdr = new (mem) AllocHeader(fmt, slab_->gen(), cardNo, size);
    return reinterpret_cast<uint8_t *>(hdr->payload());
}


} // namespace Whisper

#endif // WHISPER__RUNTIME_INLINES_HPP
