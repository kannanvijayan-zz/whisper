
#define __STDC_FORMAT_MACROS
#include "spew.hpp"
#include "vm/heap_thing.hpp"
#include "vm/heap_thing_inlines.hpp"

namespace Whisper {
namespace VM {

const char *
HeapTypeString(HeapType ht)
{
    switch (ht) {
      case HeapType::INVALID:
        return "INVALID";
#define CASE_(t, ...) case HeapType::t: return #t;
    WHISPER_DEFN_HEAP_TYPES(CASE_)
#undef CASE_
      default:
        return "UNKNOWN";
    }
}

void
SpewHeapThingArea(const uint8_t *startu8, const uint8_t *endu8)
{
    const uint64_t *start = reinterpret_cast<const uint64_t *>(startu8);
    const uint64_t *end = reinterpret_cast<const uint64_t *>(endu8);
    
    const uint64_t *cur = start;
    while (cur < end) {
        const HeapThingHeader *hdr =
            reinterpret_cast<const HeapThingHeader *>(cur);

        SpewSlabNote("{%016p}  <%s> [card=%u] [size=%u] [flags=%02x]",
                     hdr, HeapTypeString(hdr->type()),
                     (unsigned) hdr->cardNo(),
                     (unsigned) hdr->size(),
                     (unsigned) hdr->flags());

        uint32_t size = hdr->size();
        uint32_t words = DivUp<uint32_t>(size, sizeof(uint64_t));
        const uint64_t *dataEnd = cur + 1 + words;
        for (const uint64_t *data = cur + 1; data < dataEnd; data++)
            SpewSlabNote("{%016p}  %016" PRIx64, data, *data);

        cur += 1 + words;
        WH_ASSERT(cur <= end);
    }
}

void
SpewHeapThingSlab(Slab *slab)
{
    if (ChannelSpewLevel(SpewChannel::Slab) > SpewLevel::Note)
        return;

    uint8_t *headStart = slab->headStartAlloc();
    uint8_t *headEnd = slab->headEndAlloc();
    SpewHeapThingArea(headStart, headEnd);

    SpewSlabNote("...");

    // tailStart > tailEnd (since tail allocation grows down).
    // So reverse them when printing memory.
    uint8_t *tailStart = slab->tailStartAlloc();
    uint8_t *tailEnd = slab->tailEndAlloc();
    SpewHeapThingArea(tailEnd, tailStart);
}

//
// HeapThingHeader
//

HeapThingHeader::HeapThingHeader(HeapType type, uint32_t cardNo, uint32_t size)
{
    WH_ASSERT(IsValidHeapType(type));
    WH_ASSERT(cardNo <= CardNoMask);
    WH_ASSERT(size <= SizeMask);

    header_ |= ToUInt64(size) << SizeShift;
    header_ |= ToUInt64(type) << TypeShift;
    header_ |= ToUInt64(cardNo) << CardNoShift;
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
    header_ |= ToUInt64(fl) << FlagsShift;
}

void
HeapThingHeader::addFlags(uint32_t fl)
{
    WH_ASSERT(fl <= FlagsMask);
    header_ |= ToUInt64(fl) << FlagsShift;
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
HeapThing::flags() const
{
    return header()->flags();
}

uint32_t 
HeapThing::reservedSpace() const
{
    return AlignIntUp<uint32_t>(objectSize(), Slab::AllocAlign);
}


} // namespace VM
} // namespace Whisper
