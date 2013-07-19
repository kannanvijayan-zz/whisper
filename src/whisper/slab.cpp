
#include "slab.hpp"
#include "memalloc.hpp"

#include <unistd.h>
#include <new>

namespace Whisper {

static uint32_t CachedSystemPageSize = 0;
static uint32_t CachedStandardSlabCards = 0;
static uint32_t CachedStandardSlabHeaderCards = 0;
static uint32_t CachedStandardSlabDataCards = 0;
static uint32_t CachedStandardSlabMaxObjectSize = 0;

static void
InitializeStandardSlabInfo()
{

    CachedSystemPageSize = sysconf(_SC_PAGESIZE);

    WH_ASSERT(CachedSystemPageSize >= Slab::CardSize);
    WH_ASSERT(IsPowerOfTwo(CachedSystemPageSize));

    uint32_t pageSize = CachedSystemPageSize;
    uint32_t pageCards = pageSize / Slab::CardSize;

    uint32_t maxObjectSize = pageSize / 2;

    // If page is larger than 64 cards, a standard slab is a single page,
    // and the maximum sized object in a standard slab is half the size of
    // of the page.  Otherwise, a slab is 64 cards.
    uint32_t slabCards = 64;
    if (pageCards > slabCards)
        slabCards = pageCards;

    // Figure out the number of data cards.
    uint32_t dataCards = slabCards - 1;
    uint32_t headerCards;
    do {
        headerCards = Slab::NumHeaderCardsForDataCards(dataCards);
    } while (headerCards + dataCards > slabCards);

    headerCards = slabCards - dataCards;

    // If initial max object size (pageSize / 2) is smaller than
    // 1/8th of a standard slab size, choose 1/8th of a standard slab
    // size as a maximum object size.
    if (maxObjectSize < ((slabCards * Slab::CardSize) / 8))
        maxObjectSize = (slabCards * Slab::CardSize) / 8;

    CachedStandardSlabCards = slabCards;
    CachedStandardSlabHeaderCards = headerCards;
    CachedStandardSlabDataCards = dataCards;
    CachedStandardSlabMaxObjectSize = maxObjectSize;
}

/*static*/ uint32_t
Slab::PageSize()
{
    if (CachedSystemPageSize == 0)
        InitializeStandardSlabInfo();

    WH_ASSERT(CachedSystemPageSize > 0);
    return CachedSystemPageSize;
}

/*static*/ uint32_t
Slab::StandardSlabCards()
{
    if (CachedStandardSlabCards == 0)
        InitializeStandardSlabInfo();

    WH_ASSERT(CachedStandardSlabCards > 0);
    return CachedStandardSlabCards;
}

/*static*/ uint32_t
Slab::StandardSlabHeaderCards()
{
    if (CachedStandardSlabHeaderCards == 0)
        InitializeStandardSlabInfo();

    WH_ASSERT(CachedStandardSlabHeaderCards > 0);
    return CachedStandardSlabHeaderCards;
}

/*static*/ uint32_t
Slab::StandardSlabDataCards()
{
    if (CachedStandardSlabDataCards == 0)
        InitializeStandardSlabInfo();

    WH_ASSERT(CachedStandardSlabDataCards > 0);
    return CachedStandardSlabDataCards;
}

/*static*/ uint32_t
Slab::StandardSlabMaxObjectSize()
{
    if (CachedStandardSlabMaxObjectSize == 0)
        InitializeStandardSlabInfo();

    WH_ASSERT(CachedStandardSlabMaxObjectSize > 0);
    return CachedStandardSlabMaxObjectSize;
}

/*static*/ uint32_t
Slab::NumDataCardsForObjectSize(uint32_t objectSize)
{
    uint32_t dataSize = AlignIntUp<uint32_t>(objectSize, AllocAlign);

    // Add sizeof(void *) to account for the first word of
    // the data space being a pointer to the slab.
    dataSize += AlignIntUp<uint32_t>(sizeof(void *), AllocAlign);

    // Align object size up by CardSize
    return AlignIntUp(dataSize, CardSize) / CardSize;
}

/*static*/ uint32_t
Slab::NumHeaderCardsForDataCards(uint32_t dataCards)
{
    // Align size of slab struct up to natural alignment.
    uint32_t headerMinimum = AlignIntUp<uint32_t>(sizeof(Slab), AllocAlign);

    // Add space for alien refs.
    headerMinimum += AlienRefSpaceSize;

    // Add 1 byte for every data card, aligned up to natural alignment.
    headerMinimum += AlignIntUp<uint32_t>(dataCards, AllocAlign);

    // Align final amount up to CardSize
    return AlignIntUp<uint32_t>(headerMinimum, CardSize) / CardSize;
}

/*static*/ Slab *
Slab::AllocateStandard()
{
    size_t size = AlignIntUp<size_t>(StandardSlabCards() * CardSize,
                                     PageSize());
    void *result = AllocateMappedMemory(size);
    if (!result)
        return nullptr;

    WH_ASSERT(IsPtrAligned(result, CardSize));

    return new (result) Slab(result, size,
                             StandardSlabHeaderCards(),
                             StandardSlabDataCards());
}

/*static*/ Slab *
Slab::AllocateSingleton(uint32_t objectSize, bool needsGC)
{
    uint32_t dataCards = NumDataCardsForObjectSize(objectSize);
    uint32_t headerCards = NumHeaderCardsForDataCards(dataCards);
    size_t size = AlignIntUp<size_t>((dataCards + headerCards) * CardSize,
                                     PageSize());

    void *result = AllocateMappedMemory(size);
    if (!result)
        return nullptr;

    WH_ASSERT(IsPtrAligned(result, CardSize));

    Slab *slab = new (result) Slab(result, size, dataCards, headerCards);
    slab->needsGC_ = needsGC;
    return slab;
}

/*static*/ void
Slab::Destroy(Slab *slab)
{
    DebugVal<bool> r = ReleaseMappedMemory(slab->region_, slab->regionSize_);
    WH_ASSERT(r);
}

Slab::Slab(void *region, uint32_t regionSize,
           uint32_t headerCards, uint32_t dataCards)
  : region_(region), regionSize_(regionSize),
    headerCards_(headerCards), dataCards_(dataCards)
{
    // Calculate allocTop.
    uint8_t *slabBase = reinterpret_cast<uint8_t *>(this);

    uint8_t *dataSpace = slabBase + (CardSize * headerCards_);

    allocTop_ = dataSpace;
    allocBottom_ = dataSpace + (CardSize * dataCards_);
    

    headAlloc_ = allocTop_ + AlignIntUp<uint32_t>(sizeof(void *), AllocAlign);
    tailAlloc_ = allocBottom_;
}


} // namespace Whisper
