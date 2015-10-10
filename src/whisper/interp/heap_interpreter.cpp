
#include "parser/packed_syntax.hpp"
#include "vm/runtime_state.hpp"
#include "interp/heap_interpreter.hpp"

namespace Whisper {
namespace Interp {


VM::ControlFlow
HeapInterpretSourceFile(ThreadContext* cx,
                        Handle<VM::SourceFile*> frame,
                        Handle<VM::ScopeObject*> scope)
{
    return VM::ControlFlow::Error();
}


} // namespace Interp
} // namespace Whisper
