
#include "runtime_inlines.hpp"
#include "vm/core.hpp"
#include "vm/predeclare.hpp"
#include "vm/frame.hpp"

namespace Whisper {
namespace VM {


/* static */ Result<EvalFrame*>
EvalFrame::Create(AllocationContext acx,
                  Handle<Frame*> caller,
                  Handle<SyntaxTreeFragment*> syntax)
{
    return acx.create<EvalFrame>(caller, syntax);
}

/* static */ Result<EvalFrame*>
EvalFrame::Create(AllocationContext acx, Handle<SyntaxTreeFragment*> syntax)
{
    Local<Frame*> caller(acx, acx.threadContext()->maybeLastFrame());
    return Create(acx, caller, syntax);
}

/* static */ Result<FunctionFrame*>
FunctionFrame::Create(AllocationContext acx,
                      Handle<Frame*> caller,
                      Handle<Function*> function)
{
    return acx.create<FunctionFrame>(caller, function);
}

/* static */ Result<FunctionFrame*>
FunctionFrame::Create(AllocationContext acx, Handle<Function*> function)
{
    Local<Frame*> caller(acx, acx.threadContext()->maybeLastFrame());
    return Create(acx, caller, function);
}


} // namespace VM
} // namespace Whisper
