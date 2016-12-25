#ifndef WHISPER__INTERP__HEAP_INTERPRETER_HPP
#define WHISPER__INTERP__HEAP_INTERPRETER_HPP

#include "gc/local.hpp"
#include "vm/core.hpp"
#include "vm/predeclare.hpp"
#include "vm/source_file.hpp"
#include "vm/scope_object.hpp"
#include "vm/properties.hpp"
#include "vm/lookup_state.hpp"
#include "vm/control_flow.hpp"
#include "vm/frame.hpp"
#include "vm/box.hpp"

namespace Whisper {
namespace Interp {

VM::EvalResult HeapInterpretSourceFile(ThreadContext* cx,
                                       Handle<VM::SourceFile*> file,
                                       Handle<VM::ScopeObject*> scope);

VM::EvalResult HeapInterpretSourceFile(ThreadContext* cx,
                                       Handle<VM::Frame*> frame,
                                       Handle<VM::SourceFile*> file,
                                       Handle<VM::ScopeObject*> scope);

VM::EvalResult HeapInterpretLoop(ThreadContext* cx,
                                 Handle<VM::Frame*> frame);

Result<VM::Frame*> CreateInitialSyntaxFrame(
        ThreadContext* cx,
        Handle<VM::Frame*> parent,
        Handle<VM::EntryFrame*> entryFrame);

Maybe<VM::FunctionObject*> FunctionObjectForValue(ThreadContext* cx,
                                                  Handle<VM::ValBox> value);

VM::CallResult InvokeOperativeValue(
        ThreadContext* cx,
        Handle<VM::Frame*> frame,
        Handle<VM::ScopeObject*> callerScope,
        Handle<VM::ValBox> callee,
        ArrayHandle<VM::SyntaxNode*> args);

VM::CallResult InvokeOperativeFunction(
        ThreadContext* cx,
        Handle<VM::Frame*> frame,
        Handle<VM::ScopeObject*> callerScope,
        Handle<VM::ValBox> callee,
        Handle<VM::FunctionObject*> calleeFunc,
        ArrayHandle<VM::SyntaxNode*> args);

VM::CallResult InvokeApplicativeValue(
        ThreadContext* cx,
        Handle<VM::Frame*> frame,
        Handle<VM::ScopeObject*> callerScope,
        Handle<VM::ValBox> callee,
        ArrayHandle<VM::ValBox> args);

VM::CallResult InvokeScriptedApplicativeFunction(
        ThreadContext* cx,
        Handle<VM::Frame*> frame,
        Handle<VM::ScopeObject*> callerScope,
        Handle<VM::ValBox> callee,
        Handle<VM::ScriptedFunction*> calleeScript,
        ArrayHandle<VM::ValBox> args);

VM::CallResult InvokeApplicativeFunction(
        ThreadContext* cx,
        Handle<VM::Frame*> frame,
        Handle<VM::ScopeObject*> callerScope,
        Handle<VM::ValBox> callee,
        Handle<VM::FunctionObject*> calleeFunc,
        ArrayHandle<VM::ValBox> args);

VM::PropertyLookupResult GetValueProperty(ThreadContext* cx,
                                          Handle<VM::ValBox> value,
                                          Handle<VM::String*> name);

VM::PropertyLookupResult GetObjectProperty(ThreadContext* cx,
                                           Handle<VM::Wobject*> object,
                                           Handle<VM::String*> name);

OkResult DefValueProperty(ThreadContext* cx,
                          Handle<VM::ValBox> value,
                          Handle<VM::String*> name,
                          Handle<VM::PropertyDescriptor> descr);

OkResult DefObjectProperty(ThreadContext* cx,
                           Handle<VM::Wobject*> object,
                           Handle<VM::String*> name,
                           Handle<VM::PropertyDescriptor> descr);


} // namespace Interp
} // namespace Whisper

#endif // WHISPER__INTERP__HEAP_INTERPRETER_HPP
