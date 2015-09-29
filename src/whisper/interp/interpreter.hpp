#ifndef WHISPER__INTERP__INTERPRETER_HPP
#define WHISPER__INTERP__INTERPRETER_HPP

#include "gc/local.hpp"
#include "vm/core.hpp"
#include "vm/predeclare.hpp"
#include "vm/source_file.hpp"
#include "vm/scope_object.hpp"
#include "vm/control_flow.hpp"

namespace Whisper {
namespace Interp {


VM::ControlFlow InterpretSourceFile(ThreadContext* cx,
                                Handle<VM::SourceFile*> file,
                                Handle<VM::ScopeObject*> scope);

VM::ControlFlow InterpretSyntax(ThreadContext* cx,
                            Handle<VM::ScopeObject*> scope,
                            Handle<VM::PackedSyntaxTree*> pst,
                            uint32_t offset);

inline VM::ControlFlow InterpretSyntax(
        ThreadContext* cx,
        Handle<VM::ScopeObject*> scope,
        Handle<VM::SyntaxNodeRef> stRef)
{
    return InterpretSyntax(cx, scope, stRef->pst(), stRef->offset());
}

VM::ControlFlow DispatchSyntaxMethod(ThreadContext* cx,
                                 Handle<VM::ScopeObject*> scope,
                                 Handle<VM::String*> name,
                                 Handle<VM::PackedSyntaxTree*> pst,
                                 Handle<AST::PackedBaseNode> node);

VM::ControlFlow InvokeOperativeFunction(ThreadContext* cx,
                                    Handle<VM::ScopeObject*> callerScope,
                                    Handle<VM::FunctionObject*> funcObj,
                                    ArrayHandle<VM::SyntaxNodeRef> stRefs);

VM::ControlFlow InvokeApplicativeFunction(ThreadContext* cx,
                                    Handle<VM::ScopeObject*> callerScope,
                                    Handle<VM::FunctionObject*> funcObj,
                                    ArrayHandle<VM::SyntaxNodeRef> stRefs);

VM::ControlFlow GetValueProperty(ThreadContext* cx,
                                 Handle<VM::ValBox> value,
                                 Handle<VM::String*> name);

VM::ControlFlow GetObjectProperty(ThreadContext* cx,
                                  Handle<VM::Wobject*> object,
                                  Handle<VM::String*> name);


} // namespace Interp
} // namespace Whisper

#endif // WHISPER__INTERP__INTERPRETER_HPP
