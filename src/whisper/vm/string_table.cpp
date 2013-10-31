
#include "rooting_inlines.hpp"
#include "vm/string_table.hpp"

namespace Whisper {
namespace VM {


//
// StringTable
//

StringTable::Query::Query(uint32_t length, const uint8_t *data)
  : data(data), length(length), isEightBit(true)
{}

StringTable::Query::Query(uint32_t length, const uint16_t *data)
  : data(data), length(length), isEightBit(false)
{}

const uint8_t *
StringTable::Query::eightBitData() const
{
    WH_ASSERT(isEightBit);
    return reinterpret_cast<const uint8_t *>(data);
}

const uint16_t *
StringTable::Query::sixteenBitData() const
{
    WH_ASSERT(!isEightBit);
    return reinterpret_cast<const uint16_t *>(data);
}

StringTable::StringOrQuery::StringOrQuery(const VM::HeapString *str)
  : ptr(reinterpret_cast<uintptr_t>(str))
{}

StringTable::StringOrQuery::StringOrQuery(const Query *str)
  : ptr(reinterpret_cast<uintptr_t>(str) | 1)
{}

bool
StringTable::StringOrQuery::isHeapString() const
{
    return !(ptr & 1);
}

bool
StringTable::StringOrQuery::isQuery() const
{
    return ptr & 1;
}

const VM::HeapString *
StringTable::StringOrQuery::toHeapString() const
{
    WH_ASSERT(isHeapString());
    return reinterpret_cast<const VM::HeapString *>(ptr);
}

const StringTable::Query *
StringTable::StringOrQuery::toQuery() const
{
    WH_ASSERT(isQuery());
    return reinterpret_cast<const Query *>(ptr);
}

StringTable::StringTable(uint32_t spoiler)
  : spoiler_(spoiler),
    entries_(0),
    tuple_(nullptr)
{}


bool
StringTable::initialize(RunContext *cx)
{
    WH_ASSERT(tuple_ == nullptr);

    // Allocate a new tuple with reasonable capacity in tenured space.
    tuple_ = cx->inTenured().createTuple(INITIAL_TUPLE_SIZE);
    if (!tuple_)
        return false;

    return true;
}


static constexpr size_t FNV_PRIME =
#   if defined(ARCH_BITS_32)
        (ToUInt32(1) << 24) | (ToUInt32(1) << 8) | 0x93;
#   else
        (ToUInt64(1) << 40) | (ToUInt64(1) << 8) | 0xb3;
#   endif

static constexpr size_t FNV_OFFSET_BASIS =
#   if defined(ARCH_BITS_32)
        2166136261UL;
#   else
        14695981039346656037ULL;
#   endif

template <typename StrT>
static size_t
HashString(uint32_t spoiler, uint32_t length, const StrT &data)
{
    // Start with spoiler.
    size_t perturb = spoiler;
    size_t hash = FNV_OFFSET_BASIS;

    for (uint32_t i = 0; i < length; i++) {
        uint16_t ch = data[i];
        uint8_t ch_low = ch & 0xFFu;
        uint8_t ch_high = (ch >> 8) & 0xFFu;

        // Mix low byte in, perturbed.
        hash ^= ch_low ^ (perturb & 0xFFu);
        hash *= FNV_PRIME;

        // Shift and update perturbation.
        perturb ^= hash;
        perturb >>= 8;

        // Mix high byte in, perturbed.
        hash ^= ch_high ^ (perturb & 0xFFu);
        hash *= FNV_PRIME;

        // Shift and update perturbation.
        perturb ^= hash;
        perturb >>= 8;
    }
    return hash;
}

struct StrWrap
{
    const HeapString *str;
    StrWrap(const HeapString *str) : str(str) {}

    uint16_t operator[](uint32_t idx) const {
        return str->getChar(idx);
    }
};

size_t
StringTable::hash(const StringOrQuery &str)
{
    if (str.isQuery()) {
        const Query *query = str.toQuery();
        if (query->isEightBit)
            return HashString(spoiler_, query->length, query->eightBitData());

        return HashString(spoiler_, query->length, query->sixteenBitData());
    }

    WH_ASSERT(str.isHeapString());
    const HeapString *heapStr = str.toHeapString();

    if (heapStr->isLinearString()) {
        const VM::LinearString *linStr = heapStr->toLinearString();

        if (linStr->isEightBit()) {
            return HashString(spoiler_, linStr->length(),
                              linStr->eightBitData());
        }

        return HashString(spoiler_, linStr->length(),
                          linStr->sixteenBitData());
    }

    return HashString(spoiler_, heapStr->length(), StrWrap(heapStr));
}

template <typename StrT1, typename StrT2>
static size_t
CompareStrings(uint32_t len1, const StrT1 &str1,
               uint32_t len2, const StrT2 &str2)
{
    for (uint32_t i = 0; i < len1; i++) {
        // Check if str2 is prefix of str1.
        if (i >= len2)
            return 1;

        // Check characters.
        uint16_t ch1 = str1[i];
        uint16_t ch2 = str2[i];
        if (ch1 < ch2)
            return -1;
        if (ch1 > ch2)
            return 1;
    }

    // Check if str1 is a prefix of str2
    if (len2 > len1)
        return -1;

    return 0;
}

int
StringTable::compare(Handle<VM::LinearString *> a, const StringOrQuery &b)
{
    bool eightBitA = a->isEightBit();
    const void *dataA;
    if (eightBitA)
        dataA = a->eightBitData();
    else
        dataA = a->sixteenBitData();
   
    bool eightBitB;
    uint32_t lengthB;
    const void *dataB;
    if (b.isQuery()) {
        const Query *query = b.toQuery();
        lengthB = query->length;
        if (query->isEightBit) {
            eightBitB = true;
            dataB = query->eightBitData();
        } else {
            eightBitB = false;
            dataB = query->sixteenBitData();
        }
    } else {
        WH_ASSERT(b.isHeapString());
        const HeapString *heapStr = b.toHeapString();

        if (heapStr->isLinearString()) {
            const VM::LinearString *linStr = heapStr->toLinearString();

            lengthB = linStr->length();
            if (linStr->isEightBit()) {
                eightBitB = true;
                dataB = linStr->eightBitData();
            } else {
                eightBitB = false;
                dataB = linStr->sixteenBitData();
            }
        } else {
            if (eightBitA) {
                return CompareStrings(
                    a->length(), reinterpret_cast<const uint8_t *>(dataA),
                    heapStr->length(), StrWrap(heapStr));
            }

            return CompareStrings(
                a->length(), reinterpret_cast<const uint16_t *>(dataA),
                heapStr->length(), StrWrap(heapStr));
        }
    }

    if (eightBitA) {
        if (eightBitB) {
            return CompareStrings(
                    a->length(), reinterpret_cast<const uint8_t *>(dataA),
                    lengthB, reinterpret_cast<const uint8_t *>(dataB));
        }

        // Eight-bit A, Sixteen-bit B
        return CompareStrings(
                a->length(), reinterpret_cast<const uint8_t *>(dataA),
                lengthB, reinterpret_cast<const uint16_t *>(dataB));
    }

    // Sixteen-bit A, Eight-bit B
    if (eightBitB) {
        return CompareStrings(
                a->length(), reinterpret_cast<const uint16_t *>(dataA),
                lengthB, reinterpret_cast<const uint8_t *>(dataB));
    }

    // Sixteen-bit A, Sixteen-bit B
    return CompareStrings(
            a->length(), reinterpret_cast<const uint16_t *>(dataA),
            lengthB, reinterpret_cast<const uint16_t *>(dataB));
}


} // namespace VM
} // namespace Whisper
