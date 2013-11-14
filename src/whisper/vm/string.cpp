
#include "value_inlines.hpp"
#include "rooting_inlines.hpp"
#include "vm/string.hpp"
#include "vm/heap_thing_inlines.hpp"

#include <algorithm>

namespace Whisper {
namespace VM {


//
// HeapString
//

HeapString::HeapString()
{}

const HeapThing *
HeapString::toHeapThing() const
{
    return reinterpret_cast<const HeapThing *>(this);
}

HeapThing *
HeapString::toHeapThing()
{
    return reinterpret_cast<HeapThing *>(this);
}

#if defined(ENABLE_DEBUG)
bool
HeapString::isValidString() const
{
    return isLinearString();
}
#endif

bool
HeapString::isLinearString() const
{
    return toHeapThing()->type() == HeapType::LinearString;
}

const LinearString *
HeapString::toLinearString() const
{
    WH_ASSERT(isLinearString());
    return reinterpret_cast<const LinearString *>(this);
}

LinearString *
HeapString::toLinearString()
{
    WH_ASSERT(isLinearString());
    return reinterpret_cast<LinearString *>(this);
}

bool
HeapString::linearize(RunContext *cx, MutHandle<LinearString *> out)
{
    WH_ASSERT(isLinearString());
    out = toLinearString();
    return true;
}

uint32_t
HeapString::length() const
{
    WH_ASSERT(isLinearString());
    return toLinearString()->length();
}

uint16_t
HeapString::getChar(uint32_t idx) const
{
    WH_ASSERT(isLinearString());
    return toLinearString()->getChar(idx);
}

bool
HeapString::fitsImmediate() const
{
    uint32_t len = length();

    if (len <= Value::ImmString16MaxLength)
        return true;

    if (len > Value::ImmString8MaxLength)
        return false;

    // Maybe fits in an 8-bit immediate string.  Check to see if all
    // chars are 8-bit.
    for (uint32_t i = 0; i < len; i++) {
        uint16_t ch = getChar(i);
        if (ch > 0xFFu)
            return false;
    }

    return true;
}

//
// LinearString
//

void
LinearString::initializeFlags(bool interned)
{
    uint32_t flags = 0;
    if (interned)
        flags |= InternedFlagMask;
    initFlags(flags);
}

uint16_t *
LinearString::writableData()
{
    return recastThis<uint16_t>();
}

LinearString::LinearString(const HeapString *str, bool interned)
{
    WH_ASSERT(length() == str->length());

    initializeFlags(interned);

    // Only LinearString possible for now.
    WH_ASSERT(str->isLinearString());
    const LinearString *linStr = str->toLinearString();

    const uint16_t *data = linStr->data();
    std::copy(data, data + length(), writableData());

}

LinearString::LinearString(const uint8_t *data, bool interned)
{
    initializeFlags(interned);
    std::copy(data, data + length(), writableData());
}

LinearString::LinearString(const uint16_t *data, bool interned)
{
    initializeFlags(interned);
    std::copy(data, data + length(), writableData());
}

const uint16_t *
LinearString::data() const
{
    return recastThis<uint16_t>();
}

bool
LinearString::isInterned() const
{
    return flags() & InternedFlagMask;
}

uint32_t
LinearString::length() const
{
    WH_ASSERT(objectSize() % 2 == 0);
    return objectSize() / 2;
}

uint16_t
LinearString::getChar(uint32_t idx) const
{
    WH_ASSERT(idx < length());
    return data()[idx];
}

//
// Helper struct that makes an arbitrary HeapString behave like
// an array of chars.
//

struct StrWrap
{
    const HeapString *str;
    StrWrap(const HeapString *str) : str(str) {}

    uint16_t operator[](uint32_t idx) const {
        return str->getChar(idx);
    }
};

//
// String hashing.
//

static constexpr uint32_t FNV_PRIME = 0x01000193ul;
static constexpr uint32_t FNV_OFFSET_BASIS = 2166136261UL;

template <typename StrT>
static inline uint32_t
FNVHashString(uint32_t spoiler, const StrT &data, uint32_t length)
{
    // Start with spoiler.
    uint32_t perturb = spoiler;
    uint32_t hash = FNV_OFFSET_BASIS;

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

uint32_t
FNVHashString(uint32_t spoiler, const Value &strVal)
{
    WH_ASSERT(strVal.isString());

    if (strVal.isImmString()) {
        uint16_t buf[Value::ImmStringMaxLength];
        uint32_t length = strVal.readImmString(buf);
        return FNVHashString(spoiler, buf, length);
    }

    WH_ASSERT(strVal.isHeapString());
    return FNVHashString(spoiler, strVal.heapStringPtr());
}

uint32_t
FNVHashString(uint32_t spoiler, const HeapString *heapStr)
{
    return FNVHashString(spoiler, StrWrap(heapStr), heapStr->length());
}

uint32_t
FNVHashString(uint32_t spoiler, const uint8_t *str, uint32_t length)
{
    return FNVHashString(spoiler, str, length);
}

uint32_t
FNVHashString(uint32_t spoiler, const uint16_t *str, uint32_t length)
{
    return FNVHashString(spoiler, str, length);
}

//
// String comparison.
//

template <typename StrT1, typename StrT2>
static int
CompareStringsImpl(const StrT1 &str1, uint32_t len1,
                   const StrT2 &str2, uint32_t len2)
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
CompareStrings(const Value &strA, const uint8_t *strB, uint32_t lengthB)
{
    WH_ASSERT(strA.isString());

    if (strA.isImmString()) {
        uint16_t bufA[Value::ImmStringMaxLength];
        uint32_t lengthA = strA.readImmString(bufA);
        return CompareStringsImpl(bufA, lengthA, strB, lengthB);
    }

    WH_ASSERT(strA.isHeapString());
    return CompareStrings(strA.heapStringPtr(), strB, lengthB);
}

int
CompareStrings(const uint8_t *strA, uint32_t lengthA, const Value &strB)
{
    return -CompareStrings(strB, strA, lengthA);
}

int
CompareStrings(const Value &strA, const uint16_t *strB, uint32_t lengthB)
{
    WH_ASSERT(strA.isString());

    if (strA.isImmString()) {
        uint16_t bufA[Value::ImmStringMaxLength];
        uint32_t lengthA = strA.readImmString(bufA);
        return CompareStringsImpl(bufA, lengthA, strB, lengthB);
    }

    WH_ASSERT(strA.isHeapString());
    return CompareStrings(strA.heapStringPtr(), strB, lengthB);
}

int
CompareStrings(const uint16_t *strA, uint32_t lengthA, const Value &strB)
{
    return -CompareStrings(strB, strA, lengthA);
}

int
CompareStrings(const HeapString *strA,
               const uint8_t *strB, uint32_t lengthB)
{
    return CompareStringsImpl(StrWrap(strA), strA->length(), strB, lengthB);
}

int
CompareStrings(const uint8_t *strA, uint32_t lengthA,
               const HeapString *strB)
{
    return -CompareStrings(strB, strA, lengthA);
}

int
CompareStrings(const HeapString *strA,
               const uint16_t *strB, uint32_t lengthB)
{
    return CompareStringsImpl(StrWrap(strA), strA->length(), strB, lengthB);
}

int
CompareStrings(const uint16_t *strA, uint32_t lengthA,
               const HeapString *strB)
{
    return -CompareStrings(strB, strA, lengthA);
}

int
CompareStrings(const Value &strA, const HeapString *strB)
{
    WH_ASSERT(strA.isString());

    if (strA.isImmString()) {
        uint16_t bufA[Value::ImmStringMaxLength];
        uint32_t lengthA = strA.readImmString(bufA);
        return CompareStringsImpl(bufA, lengthA,
                                  StrWrap(strB), strB->length());
    }

    WH_ASSERT(strA.isHeapString());
    return CompareStrings(strA.heapStringPtr(), strB);
}

int
CompareStrings(const HeapString *strA, const Value &strB)
{
    return -CompareStrings(strB, strA);
}

int
CompareStrings(const Value &strA, const Value &strB)
{
    WH_ASSERT(strA.isString());

    if (strA.isImmString()) {
        uint16_t bufA[Value::ImmStringMaxLength];
        uint32_t lengthA = strA.readImmString(bufA);

        if (strB.isImmString()) {
            uint16_t bufB[Value::ImmStringMaxLength];
            uint32_t lengthB = strB.readImmString(bufB);
            return CompareStrings(bufA, lengthA, bufB, lengthB);
        }

        WH_ASSERT(strB.isHeapString());
        HeapString *heapB = strB.heapStringPtr();

        return CompareStringsImpl(bufA, lengthA,
                                  StrWrap(heapB), heapB->length());
    }

    WH_ASSERT(strA.isHeapString());
    HeapString *heapA = strA.heapStringPtr();

    if (strB.isImmString()) {
        uint16_t bufB[Value::ImmStringMaxLength];
        uint32_t lengthB = strB.readImmString(bufB);
        return CompareStringsImpl(StrWrap(heapA), heapA->length(),
                                  bufB, lengthB);
    }

    WH_ASSERT(strB.isHeapString());
    HeapString *heapB = strB.heapStringPtr();

    return CompareStrings(heapA, heapB);
}

int
CompareStrings(const HeapString *strA, const HeapString *strB)
{
    return CompareStringsImpl(StrWrap(strA), strA->length(),
                              StrWrap(strB), strB->length());
}

int
CompareStrings(const uint8_t *strA, uint32_t lengthA,
               const uint8_t *strB, uint32_t lengthB)
{
    return CompareStringsImpl(strA, lengthA, strB, lengthB);
}

int
CompareStrings(const uint16_t *strA, uint32_t lengthA,
               const uint16_t *strB, uint32_t lengthB)
{
    return CompareStringsImpl(strA, lengthA, strB, lengthB);
}

int
CompareStrings(const uint8_t *strA, uint32_t lengthA,
               const uint16_t *strB, uint32_t lengthB)
{
    return CompareStringsImpl(strA, lengthA, strB, lengthB);
}

int
CompareStrings(const uint16_t *strA, uint32_t lengthA,
               const uint8_t *strB, uint32_t lengthB)
{
    return CompareStringsImpl(strA, lengthA, strB, lengthB);
}

//
// Check if a string is an positive int32_t value.
//

template <typename StrT>
static bool
IsInt32IdStringImpl(const StrT &str, uint32_t length, int32_t *val)
{
    if (length == 0)
        return false;

    uint16_t firstCh = str[0];

    if (length == 1)
        return firstCh == '0';

    // Only id that can start with '0' is '0' itself.
    if (firstCh == '0')
        return false;

    // Consume first digit (must not be 0)
    if (!isdigit(firstCh))
        return false;

    // Initialize accumulator, accumulate number.
    uint32_t accum = (firstCh - '0');
    for (uint32_t idx = 1; idx < length; idx++) {
        uint16_t ch = str[idx];
        if (!isdigit(ch))
            return false;

        uint32_t digit = ch - '0';
        WH_ASSERT(digit < 10);

        // Check if multiplying accum by 10 will overflow.
        if (accum > ToUInt32(INT32_MAX / 10))
            return false;
        accum *= 10;

        // Check if adding digit will overflow.
        if (accum + digit > ToUInt32(INT32_MAX))
            return false;
        accum += digit;
    }

    *val = accum;
    return true;
}

bool
IsInt32IdString(const uint8_t *str, uint32_t length, int32_t *val)
{
    return IsInt32IdStringImpl(str, length, val);
}

bool
IsInt32IdString(const uint16_t *str, uint32_t length, int32_t *val)
{
    return IsInt32IdStringImpl(str, length, val);
}

bool
IsInt32IdString(HeapString *str, uint32_t length, int32_t *val)
{
    return IsInt32IdStringImpl(StrWrap(str), str->length(), val);
}



} // namespace VM
} // namespace Whisper
