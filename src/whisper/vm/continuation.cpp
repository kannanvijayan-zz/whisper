
#include "runtime_inlines.hpp"
#include "vm/continuation.hpp"

namespace Whisper {
namespace VM {


/* static */ Result<Continuation*>
Continuation::Create(AllocationContext acx, Handle<Frame*> frame)
{
    return acx.create<Continuation>(frame);
}


} // namespace VM
} // namespace Whisper
