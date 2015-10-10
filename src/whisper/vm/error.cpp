#include "vm/core.hpp"
#include "vm/error.hpp"
#include "runtime_inlines.hpp"

namespace Whisper {
namespace VM {


/* static */ Result<Error*>
Error::Create(AllocationContext acx)
{
    return acx.create<String>();
}


} // namespace VM
} // namespace Whisper
