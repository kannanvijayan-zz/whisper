
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
LinearString::initializeFlags(bool interned, Group group)
{
    uint32_t flags = 0;
    if (interned)
        flags |= InternedFlagMask;
    flags |= static_cast<uint32_t>(group) << GroupShift;
    initFlags(flags);
}

uint16_t *
LinearString::writableData()
{
    return recastThis<uint16_t>();
}

LinearString::LinearString(const HeapString *str, bool interned, Group group)
{
    WH_ASSERT(length() == str->length());

    initializeFlags(interned, group);

    // Only LinearString possible for now.
    WH_ASSERT(str->isLinearString());
    const LinearString *linStr = str->toLinearString();

    const uint16_t *data = linStr->data();
    std::copy(data, data + length(), writableData());

}

LinearString::LinearString(const uint8_t *data, bool interned, Group group)
{
    initializeFlags(interned, group);
    std::copy(data, data + length(), writableData());
}

LinearString::LinearString(const uint16_t *data, bool interned, Group group)
{
    initializeFlags(interned, group);
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

LinearString::Group
LinearString::group() const
{
    return static_cast<Group>((flags() >> GroupShift) & GroupMask);
}

bool
LinearString::inUnknownGroup() const
{
    return group() == Group::Unknown;
}

bool
LinearString::inIndexGroup() const
{
    return group() == Group::Index;
}

bool
LinearString::inNameGroup() const
{
    return group() == Group::Name;
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
FNVHashString(uint32_t spoiler, uint32_t length, const StrT &data)
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
        return FNVHashString(spoiler, length, buf);
    }

    WH_ASSERT(strVal.isHeapString());
    return FNVHashString(spoiler, strVal.heapStringPtr());
}

uint32_t
FNVHashString(uint32_t spoiler, const HeapString *heapStr)
{
    return FNVHashString(spoiler, heapStr->length(), StrWrap(heapStr));
}

uint32_t
FNVHashString(uint32_t spoiler, uint32_t length, const uint8_t *str)
{
    return FNVHashString(spoiler, length, str);
}

uint32_t
FNVHashString(uint32_t spoiler, uint32_t length, const uint16_t *str)
{
    return FNVHashString(spoiler, length, str);
}

//
// String comparison.
//

template <typename StrT1, typename StrT2>
static int
CompareStringsImpl(uint32_t len1, const StrT1 &str1,
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
CompareStrings(const Value &strA, uint32_t lengthB, const uint8_t *strB)
{
    WH_ASSERT(strA.isString());

    if (strA.isImmString()) {
        uint16_t bufA[Value::ImmStringMaxLength];
        uint32_t lengthA = strA.readImmString(bufA);
        return CompareStringsImpl(lengthA, bufA, lengthB, strB);
    }

    WH_ASSERT(strA.isHeapString());
    return CompareStrings(strA.heapStringPtr(), lengthB, strB);
}

int
CompareStrings(uint32_t lengthA, const uint8_t *strA, const Value &strB)
{
    return -CompareStrings(strB, lengthA, strA);
}

int
CompareStrings(const Value &strA, uint32_t lengthB, const uint16_t *strB)
{
    WH_ASSERT(strA.isString());

    if (strA.isImmString()) {
        uint16_t bufA[Value::ImmStringMaxLength];
        uint32_t lengthA = strA.readImmString(bufA);
        return CompareStringsImpl(lengthA, bufA, lengthB, strB);
    }

    WH_ASSERT(strA.isHeapString());
    return CompareStrings(strA.heapStringPtr(), lengthB, strB);
}

int
CompareStrings(uint32_t lengthA, const uint16_t *strA, const Value &strB)
{
    return -CompareStrings(strB, lengthA, strA);
}

int
CompareStrings(const HeapString *strA,
               uint32_t lengthB, const uint8_t *strB)
{
    return CompareStringsImpl(strA->length(), StrWrap(strA), lengthB, strB);
}

int
CompareStrings(uint32_t lengthA, const uint8_t *strA,
               const HeapString *strB)
{
    return -CompareStrings(strB, lengthA, strA);
}

int
CompareStrings(const HeapString *strA,
               uint32_t lengthB, const uint16_t *strB)
{
    return CompareStringsImpl(strA->length(), StrWrap(strA), lengthB, strB);
}

int
CompareStrings(uint32_t lengthA, const uint16_t *strA,
               const HeapString *strB)
{
    return -CompareStrings(strB, lengthA, strA);
}

int
CompareStrings(const Value &strA, const HeapString *strB)
{
    WH_ASSERT(strA.isString());

    if (strA.isImmString()) {
        uint16_t bufA[Value::ImmStringMaxLength];
        uint32_t lengthA = strA.readImmString(bufA);
        return CompareStringsImpl(lengthA, bufA,
                                  strB->length(), StrWrap(strB));
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
            return CompareStrings(lengthA, bufA, lengthB, bufB);
        }

        WH_ASSERT(strB.isHeapString());
        HeapString *heapB = strB.heapStringPtr();

        return CompareStringsImpl(lengthA, bufA,
                                  heapB->length(), StrWrap(heapB));
    }

    WH_ASSERT(strA.isHeapString());
    HeapString *heapA = strA.heapStringPtr();

    if (strB.isImmString()) {
        uint16_t bufB[Value::ImmStringMaxLength];
        uint32_t lengthB = strB.readImmString(bufB);
        return CompareStringsImpl(heapA->length(), StrWrap(heapA),
                                  lengthB, bufB);
    }

    WH_ASSERT(strB.isHeapString());
    HeapString *heapB = strB.heapStringPtr();

    return CompareStrings(heapA, heapB);
}

int
CompareStrings(const HeapString *strA, const HeapString *strB)
{
    return CompareStringsImpl(strA->length(), StrWrap(strA),
                              strB->length(), StrWrap(strB));
}

int
CompareStrings(uint32_t lengthA, const uint8_t *strA,
               uint32_t lengthB, const uint8_t *strB)
{
    return CompareStringsImpl(lengthA, strA, lengthB, strB);
}

int
CompareStrings(uint32_t lengthA, const uint16_t *strA,
               uint32_t lengthB, const uint16_t *strB)
{
    return CompareStringsImpl(lengthA, strA, lengthB, strB);
}

int
CompareStrings(uint32_t lengthA, const uint8_t *strA,
               uint32_t lengthB, const uint16_t *strB)
{
    return CompareStringsImpl(lengthA, strA, lengthB, strB);
}

int
CompareStrings(uint32_t lengthA, const uint16_t *strA,
               uint32_t lengthB, const uint8_t *strB)
{
    return CompareStringsImpl(lengthA, strA, lengthB, strB);
}


} // namespace VM
} // namespace Whisper
