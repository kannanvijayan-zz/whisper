
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
    return linearStr->isInterned();
}


} // namespace VM
} // namespace Whisper
