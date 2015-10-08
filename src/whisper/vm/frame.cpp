
#include "runtime_inlines.hpp"
#include "vm/core.hpp"
#include "vm/predeclare.hpp"
#include "vm/frame.hpp"

namespace Whisper {
namespace VM {


/* static */ Result<Frame*>
Frame::CreateEval(AllocationContext acx,
                  Handle<Frame*> caller,
                  Handle<SyntaxTreeFragment*> anchor)
{
    return acx.create<Frame>(caller, Eval,
                             anchor.forceConvertTo<HeapThing*>());
}

/* static */ Result<Frame*>
Frame::CreateEval(AllocationContext acx,
                  Handle<SyntaxTreeFragment*> anchor)
{
    Local<Frame*> caller(acx, acx.threadContext()->maybeLastFrame());
    return CreateEval(acx, caller, anchor);
}

/* static */ Result<Frame*>
Frame::CreateFunc(AllocationContext acx,
                  Handle<Frame*> caller,
                  Handle<Function*> anchor)
{
    return acx.create<Frame>(caller, Func,
                             anchor.forceConvertTo<HeapThing*>());
}

/* static */ Result<Frame*>
Frame::CreateFunc(AllocationContext acx,
                  Handle<Function*> anchor)
{
    Local<Frame*> caller(acx, acx.threadContext()->maybeLastFrame());
    return CreateFunc(acx, caller, anchor);
}


} // namespace VM
} // namespace Whisper
