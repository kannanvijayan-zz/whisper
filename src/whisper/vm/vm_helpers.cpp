
#include "common.hpp"
#include "debug.hpp"
#include "value.hpp"
#include "value_inlines.hpp"
#include "vm/heap_thing_inlines.hpp"
#include "vm/string.hpp"
#include "vm/vm_helpers.hpp"

#include <ctype.h>

namespace Whisper {
namespace VM {


template <typename CharT>
bool
IsInt32IdString(const CharT *str, uint32_t length, int32_t *val)
{
    if (length == 0)
        return false;

    if (length == 1)
        return str[0] == '0';

    uint32_t idx = 0;

    // Consume any beginning '-'
    bool neg = (str[0] == '-');
    if (neg)
        idx += 1;

    // Consume first digit (must not be 0)
    if (!isdigit(str[idx]))
        return false;
    if (str[idx] == '0')
        return false;

    // Initialize accumulator, accumulate number.
    uint32_t uint31Max = (UINT32_MAX >> 1) + (neg ? 1u : 0u);
    uint32_t accum = (str[idx++] - '0');
    for (; idx < length; idx++) {
        if (!isdigit(str[idx]))
            return false;

        uint32_t digit = (str[idx] - '0');
        WH_ASSERT(digit < 10);

        // Check if multiplying accum by 10 will overflow.
        if (accum > (uint31Max / 10))
            return false;
        accum *= 10;

        // Check if adding digit will overflow.
        if (accum + digit > uint31Max)
            return false;
        accum += digit;
    }

    if (val)
        *val = neg ? -accum : accum;

    return true;
}

template bool
IsInt32IdString<uint8_t>(const uint8_t *str, uint32_t length, int32_t *val);

template bool
IsInt32IdString<uint16_t>(const uint16_t *str, uint32_t length, int32_t *val);



template <typename CharT>
bool
IsNormalizedPropertyId(const CharT *str, uint32_t length)
{
    // Strings in bijection with int32s aren't normalized property ids.
    if (IsInt32IdString(str, length, nullptr))
        return false;

    return true;
}

template bool
IsNormalizedPropertyId<uint8_t>(const uint8_t *str, uint32_t length);

template bool
IsNormalizedPropertyId<uint16_t>(const uint16_t *str, uint32_t length);


bool
IsNormalizedPropertyId(const Value &val)
{
    // All immediate strings are normalized property ids.
    if (val.isImmString())
        return true;

    // The only remaining strings should be heap strings.
    WH_ASSERT_IF(val.isString(), val.isHeapString());

    if (!val.isHeapString())
        return false;

    // All normalized heap strings are linear.
    VM::HeapString *heapStr = val.heapStringPtr();
    if (!heapStr->isLinearString())
        return false;

    // The linear string must be interned and identified as a property name.
    VM::LinearString *linearStr = heapStr->toLinearString();
    return linearStr->isInterned() && linearStr->inNameGroup();
}


} // namespace VM
} // namespace Whisper
