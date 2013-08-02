
#include "heap_thing.hpp"
#include "heap_thing_inlines.hpp"

namespace Whisper {
namespace VM {

//
// HeapThingHeader
//

HeapThingHeader::HeapThingHeader(HeapType type, uint32_t cardNo, uint32_t size)
{
    WH_ASSERT(IsValidHeapType(type));
    WH_ASSERT(cardNo <= CardNoMask);
    WH_ASSERT(size <= SizeMask);

    header_ |= Tag << TagShift;
    header_ |= static_cast<uint64_t>(size) << SizeShift;
    header_ |= static_cast<uint64_t>(type) << TypeShift;
    header_ |= static_cast<uint64_t>(cardNo) << CardNoShift;
}


uint32_t
HeapThingHeader::cardNo() const
{
    return (header_ >> CardNoShift) & CardNoMask;
}

HeapType
HeapThingHeader::type() const
{
    return static_cast<HeapType>((header_ >> TypeShift) & TypeMask);
}

uint32_t
HeapThingHeader::size() const
{
    return (header_ >> SizeShift) & SizeMask;
}

uint32_t
HeapThingHeader::flags() const
{
    return (header_ >> FlagsShift) & FlagsMask;
}

void
HeapThingHeader::initFlags(uint32_t fl)
{
    WH_ASSERT(fl <= FlagsMask);
    WH_ASSERT(flags() == 0);
    header_ |= static_cast<uint64_t>(fl) << FlagsShift;
}

void
HeapThingHeader::addFlags(uint32_t fl)
{
    WH_ASSERT(fl <= FlagsMask);
    header_ |= static_cast<uint64_t>(fl) << FlagsShift;
}

//
// HeapThing
//

HeapThing::HeapThing() {}
HeapThing::~HeapThing() {}

HeapThingHeader *
HeapThing::header()
{
    uint64_t *thisp = recastThis<uint64_t>();
    return reinterpret_cast<HeapThingHeader *>(thisp - 1);
}

const HeapThingHeader *
HeapThing::header() const
{
    const uint64_t *thisp = recastThis<const uint64_t>();
    return reinterpret_cast<const HeapThingHeader *>(thisp - 1);
}

void 
HeapThing::initFlags(uint32_t flags)
{
    return header()->initFlags(flags);
}

void
HeapThing::addFlags(uint32_t flags)
{
    header()->addFlags(flags);
}

void 
HeapThing::noteWrite(void *ptr)
{
    // TODO: Add write barrier.
}

uint32_t 
HeapThing::cardNo() const
{
    return header()->cardNo();
}

HeapType 
HeapThing::type() const
{
    return header()->type();
}

uint32_t 
HeapThing::objectSize() const
{
    return header()->size();
}

uint32_t 
HeapThing::objectValueCount() const
{
    WH_ASSERT(IsIntAligned<uint32_t>(objectSize(), sizeof(Value)));
    return objectSize() / sizeof(Value);
}

uint32_t 
HeapThing::flags() const
{
    return header()->flags();
}

uint32_t 
HeapThing::reservedSpace() const
{
    return AlignIntUp<uint32_t>(objectSize(), Slab::AllocAlign);
}

Value *
HeapThing::valuePointer(uint32_t idx)
{
    return dataPointer<Value>(idx * 8);
}

const Value *
HeapThing::valuePointer(uint32_t idx) const
{
    return dataPointer<Value>(idx * 8);
}

Value &
HeapThing::valueRef(uint32_t idx)
{
    return dataRef<Value>(idx * 8);
}

const Value &
HeapThing::valueRef(uint32_t idx) const
{
    return dataRef<Value>(idx * 8);
}


} // namespace VM
} // namespace Whisper
