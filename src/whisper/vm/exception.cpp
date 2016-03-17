
#include "vm/core.hpp"
#include "vm/exception.hpp"
#include "runtime_inlines.hpp"

namespace Whisper {
namespace VM {

/* static */ Result<InternalException*>
InternalException::Create(AllocationContext acx,
                          char const* message,
                          uint32_t numArguments,
                          ArrayHandle<ValBox> const& arguments)
{
    return acx.createSized<InternalException>(CalculateSize(numArguments),
                                   message, numArguments, arguments);
}



} // namespace VM
} // namespace Whisper
