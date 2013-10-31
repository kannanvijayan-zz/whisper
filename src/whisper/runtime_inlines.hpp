#ifndef WHISPER__RUNTIME_INLINES_HPP
#define WHISPER__RUNTIME_INLINES_HPP

#include "runtime.hpp"
#include "vm/heap_thing.hpp"

namespace Whisper {


//
// AllocationContext
//

template <typename ObjT>
inline uint8_t *
AllocationContext::allocate(uint32_t size)
{
    WH_ASSERT(size >= sizeof(ObjT));

    // Add HeaderSize to size.
    uint32_t allocSize = size + VM::HeapThingHeader::HeaderSize;
    allocSize = AlignIntUp<uint32_t>(allocSize, Slab::AllocAlign);

    // Track whether to allocate from top or bottom.
    bool headAlloc = VM::HeapTypeTraits<ObjT::Type>::Traced;

    // Allocate the space.
    return headAlloc ? slab_->allocateHead(allocSize)
                     : slab_->allocateTail(allocSize);
}

template <typename ObjT, typename... Args>
inline ObjT *
AllocationContext::create(Args... args)
{
    return createSized<ObjT, Args...>(sizeof(ObjT), args...);
}

template <typename ObjT, typename... Args>
inline ObjT *
AllocationContext::createSized(uint32_t size, Args... args)
{
    // Allocate the space for the object.
    uint8_t *mem = allocate<ObjT>(size);
    if (!mem) {
        if (cx_->suppressGC())
            return nullptr;

        WH_ASSERT(!"GC infrastructure.");
        return nullptr;
    }

    // Figure out the card number.
    uint32_t cardNo = slab_->calculateCardNumber(mem);

    // Initialize the object using HeapThingWrapper, and
    // return it.
    typedef VM::HeapThingWrapper<ObjT> WrappedType;
    WrappedType *wrapped = new (mem) WrappedType(cardNo, size, args...);
    return wrapped->payloadPointer();
}


} // namespace Whisper

#endif // WHISPER__RUNTIME_INLINES_HPP
