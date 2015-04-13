#ifndef WHISPER__VM__INTERPRETER_HPP
#define WHISPER__VM__INTERPRETER_HPP

#include "gc/local.hpp"
#include "vm/core.hpp"
#include "vm/predeclare.hpp"
#include "vm/source_file.hpp"
#include "vm/scope_object.hpp"

namespace Whisper {
namespace VM {


OkResult InterpretSourceFile(ThreadContext *cx,
                             Handle<SourceFile *> file,
                             Handle<ScopeObject *> scope,
                             MutHandle<ValBox> resultOut);


OkResult InterpretSyntax(ThreadContext *cx,
                         Handle<ScopeObject *> scope,
                         Handle<PackedSyntaxTree *> pst,
                         uint32_t offset,
                         MutHandle<ValBox> resultOut);

OkResult DispatchSyntaxMethod(ThreadContext *cx,
                              Handle<ScopeObject *> scope,
                              Handle<String *> name,
                              Handle<PackedSyntaxTree *> pst,
                              Handle<AST::PackedBaseNode> node,
                              MutHandle<ValBox> resultOut);

OkResult InvokeOperativeFunction(ThreadContext *cx,
                                 Handle<LookupState *> lookupState,
                                 Handle<ScopeObject *> callerScope,
                                 Handle<Function *> func,
                                 Handle<Wobject *> receiver,
                                 Handle<SyntaxTreeRef> stRef,
                                 MutHandle<ValBox> resultOut);


} // namespace VM
} // namespace Whisper

#endif // WHISPER__VM__INTERPRETER_HPP
