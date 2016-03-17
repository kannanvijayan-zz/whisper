
#include "vm/core.hpp"
#include "vm/exception.hpp"
#include "runtime_inlines.hpp"

namespace Whisper {
namespace VM {

/* static */ Result<InternalException*>
InternalException::Create(AllocationContext acx,
                          char const* message,
                          ArrayHandle<Box> const& arguments)
{
    uint32_t numArguments = arguments.length();
    return acx.createSized<InternalException>(CalculateSize(numArguments),
                message, numArguments, arguments);
}



} // namespace VM
} // namespace Whisper
