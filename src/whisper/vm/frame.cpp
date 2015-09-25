
#include "runtime_inlines.hpp"
#include "vm/core.hpp"
#include "vm/predeclare.hpp"
#include "vm/frame.hpp"

namespace Whisper {
namespace VM {


/* static */ Result<Frame*>
Frame::Create(AllocationContext acx,
              Handle<Frame*> caller,
              Handle<ScriptedFunction*> func,
              Handle<CallScope*> scope,
              uint32_t maxStackDepth,
              uint32_t maxEvalDepth)
{
    uint32_t size = CalculateSize(maxStackDepth, maxEvalDepth);
    return acx.createSized<Frame>(size, caller, func, scope,
                                  maxStackDepth, maxEvalDepth);
}


} // namespace VM
} // namespace Whisper
