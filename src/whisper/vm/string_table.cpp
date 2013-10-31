
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

StringTable::Hash::Hash(StringTable *table)
  : table(table)
{}


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
StringTable::Hash::operator ()(const StringOrQuery &str)
{
    if (str.isQuery()) {
        const Query *query = str.toQuery();
        if (query->isEightBit) {
            return HashString(table->spoiler_, query->length,
                              reinterpret_cast<const uint8_t *>(query->data));
        }

        return HashString(table->spoiler_, query->length,
                          reinterpret_cast<const uint16_t *>(query->data));
    }

    WH_ASSERT(str.isHeapString());
    const HeapString *heapStr = str.toHeapString();

    return HashString(table->spoiler_, heapStr->length(), StrWrap(heapStr));
}


StringTable::StringTable(uint32_t spoiler)
  : spoiler_(spoiler),
    entries_(0),
    tuple_(nullptr)
{}


} // namespace VM
} // namespace Whisper
