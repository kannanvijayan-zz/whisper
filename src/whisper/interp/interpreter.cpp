
#include "common.hpp"
#include "runtime.hpp"
#include "runtime_inlines.hpp"
#include "rooting_inlines.hpp"
#include "vm/heap_thing_inlines.hpp"
#include "vm/stack_frame.hpp"
#include "interp/interpreter.hpp"

namespace Whisper {
namespace Interp {


bool
InterpretScript(RunContext *cx, VM::HandleScript script)
{
    WH_ASSERT(script->isTopLevel());

    // Create a new stack frame for this script.
    VM::StackFrame::Config config;
    config.maxStackDepth = script->maxStackDepth();
    config.numActualArgs = 0;

    uint32_t size = VM::StackFrame::CalculateSize(config);
    VM::RootedStackFrame stackFrame(cx,
        cx->createSized<VM::StackFrame>(true, size, script, config));

    return true;
}


} // namespace Interp
} // namespace Whisper
