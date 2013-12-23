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
    return createSized<ObjT, Args...>(sizeof(ObjT),
                                      std::forward<Args>(args)...);
}

template <typename ObjT, typename... Args>
inline ObjT *
AllocationContext::createSized(uint32_t size, Args... args)
{
    WH_ASSERT(size >= sizeof(ObjT));

    // Allocate the space for the object.
    constexpr bool TRACED = AllocationTracedTrait<ObjT>::TRACED;
    constexpr SlabAllocType TYPETAG = AllocationTypeTagTrait<ObjT>::TYPETAG;
    uint8_t *mem = allocate<TRACED>(size, TYPETAG);
    if (!mem)
        return nullptr;

    // Construct object in memory.
    new (mem) ObjT(std::forward<Args>(args)...);

    return reinterpret_cast<ObjT *>(mem);
}

template <bool Traced>
inline uint8_t *
AllocationContext::allocate(uint32_t size, SlabAllocType typeTag)
{
    bool large = size >= SlabAllocHeader::ALLOCSIZE_MAX;

    // Add HeaderSize to size.
    uint32_t allocSize = size + sizeof(word_t);
    if (large)
        allocSize += sizeof(word_t);

    allocSize = AlignIntUp<uint32_t>(allocSize, Slab::AllocAlign);

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

    // Initialize the header and maybe size ext word.
    if (large) {
        uint32_t hdrSize = SlabAllocHeader::ALLOCSIZE_MAX;
        new (mem) SlabAllocHeader(cardNo, hdrSize, typeTag);
        new (mem + sizeof(word_t)) SlabSizeExtHeader(size);
        return mem + sizeof(word_t) + sizeof(word_t);
    }

    new (mem) SlabAllocHeader(cardNo, size, typeTag);
    return mem + sizeof(word_t);
}


} // namespace Whisper

#endif // WHISPER__RUNTIME_INLINES_HPP
