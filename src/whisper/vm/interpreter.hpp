#ifndef WHISPER__VM__INTERPRETER_HPP
#define WHISPER__VM__INTERPRETER_HPP

#include "gc/local.hpp"
#include "vm/core.hpp"
#include "vm/predeclare.hpp"
#include "vm/source_file.hpp"
#include "vm/scope_object.hpp"
#include "vm/control_flow.hpp"

namespace Whisper {
namespace VM {


ControlFlow InterpretSourceFile(ThreadContext *cx,
                                Handle<SourceFile *> file,
                                Handle<ScopeObject *> scope);

ControlFlow InterpretSyntax(ThreadContext *cx,
                            Handle<ScopeObject *> scope,
                            Handle<PackedSyntaxTree *> pst,
                            uint32_t offset);

ControlFlow DispatchSyntaxMethod(ThreadContext *cx,
                                 Handle<ScopeObject *> scope,
                                 Handle<String *> name,
                                 Handle<PackedSyntaxTree *> pst,
                                 Handle<AST::PackedBaseNode> node);

ControlFlow InvokeOperativeFunction(ThreadContext *cx,
                                    Handle<LookupState *> lookupState,
                                    Handle<ScopeObject *> callerScope,
                                    Handle<Function *> func,
                                    Handle<Wobject *> receiver,
                                    Handle<SyntaxTreeRef> stRef);


} // namespace VM
} // namespace Whisper

#endif // WHISPER__VM__INTERPRETER_HPP
