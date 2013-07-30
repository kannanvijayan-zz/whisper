#ifndef WHISPER__RUNTIME_INLINES_HPP
#define WHISPER__RUNTIME_INLINES_HPP

#include "runtime.hpp"
#include "vm/heap_thing.hpp"

namespace Whisper {

template <typename ObjT, typename... Args>
inline ObjT *
RunContext::create(bool allowGC, Args... args)
{
    return create<ObjT, Args...>(allowGC, sizeof(ObjT), args...);
}

template <typename ObjT, typename... Args>
inline ObjT *
RunContext::create(bool allowGC, uint32_t size, Args... args)
{
    // Allocate the space for the object.
    uint8_t *mem = allocate<ObjT::Type>(size);
    if (!mem) {
        if (!allowGC)
            return nullptr;

        WH_ASSERT(!"GC infrastructure.");
        return nullptr;
    }

    // Figure out the card number.
    uint32_t cardNo = hatchery_->calculateCardNumber(mem);

    // Initialize the object using HeapThingWrapper, and
    // return it.
    typedef VM::HeapThingWrapper<ObjT> WrappedType;
    WrappedType *wrapped = new (mem) WrappedType(cardNo, size, args...);
    return wrapped->payloadPointer();
}

template <typename ObjT>
inline uint8_t *
RunContext::allocate(uint32_t size)
{
    WH_ASSERT(size >= sizeof(ObjT));

    // Add HeaderSize to size.
    uint32_t allocSize = size + VM::HeapThingHeader::HeaderSize;
    allocSize = AlignIntUp<uint32_t>(allocSize, Slab::AllocAlign);

    // Track whether to allocate from top or bottom.
    bool headAlloc = VM::HeapTypeTraits<ObjT::Type>::Traced;

    // Allocate the space.
    return headAlloc ? hatchery_->allocateHead(allocSize)
                     : hatchery_->allocateTail(allocSize);
}


} // namespace Whisper

#endif // WHISPER__RUNTIME_INLINES_HPP
