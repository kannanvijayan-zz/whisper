
#include "runtime_inlines.hpp"
#include "vm/core.hpp"
#include "vm/predeclare.hpp"
#include "vm/function.hpp"

namespace Whisper {
namespace VM {


bool
Function::isApplicative() const
{
    if (isNative()) {
        return asNative()->isApplicative();
    }

    WH_UNREACHABLE("Unknown function type.");
    return false;
}


Result<NativeFunction *>
NativeFunction::Create(AllocationContext acx,
                       NativeApplicativeFuncPtr *applicative)
{
    return acx.create<NativeFunction>(applicative);
}

Result<NativeFunction *>
NativeFunction::Create(AllocationContext acx,
                       NativeOperativeFuncPtr *operative)
{
    return acx.create<NativeFunction>(operative);
}


} // namespace VM
} // namespace Whisper
