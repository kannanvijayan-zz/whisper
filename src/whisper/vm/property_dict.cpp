
#include "runtime_inlines.hpp"
#include "vm/property_dict.hpp"

namespace Whisper {
namespace VM {


/* static */ Result<PropertyDict*>
PropertyDict::Create(AllocationContext acx, uint32_t capacity)
{
    return acx.createSized<PropertyDict>(CalculateSize(capacity), capacity);
}


} // namespace VM
} // namespace Whisper
