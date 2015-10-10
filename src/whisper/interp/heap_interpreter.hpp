#ifndef WHISPER__INTERP__HEAP_INTERPRETER_HPP
#define WHISPER__INTERP__HEAP_INTERPRETER_HPP

#include "gc/local.hpp"
#include "vm/core.hpp"
#include "vm/predeclare.hpp"
#include "vm/source_file.hpp"
#include "vm/scope_object.hpp"
#include "vm/control_flow.hpp"
#include "vm/frame.hpp"

namespace Whisper {
namespace Interp {


VM::ControlFlow HeapInterpretSourceFile(ThreadContext* cx,
                                        Handle<VM::SourceFile*> frame,
                                        Handle<VM::ScopeObject*> scope);


} // namespace Interp
} // namespace Whisper

#endif // WHISPER__INTERP__HEAP_INTERPRETER_HPP
