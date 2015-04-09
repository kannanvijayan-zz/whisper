
#include <iostream>

#include "parser/syntax_defn.hpp"
#include "parser/packed_syntax.hpp"
#include "runtime_inlines.hpp"
#include "vm/global_scope.hpp"
#include "vm/runtime_state.hpp"
#include "vm/function.hpp"

namespace Whisper {
namespace VM {


/* static */ Result<GlobalScope *>
GlobalScope::Create(AllocationContext acx)
{
    // Allocate empty array of delegates.
    Local<Array<Wobject *> *> delegates(acx);
    if (!delegates.setResult(Array<Wobject *>::CreateEmpty(acx)))
        return ErrorVal();

    // Allocate a dictionary.
    Local<PropertyDict *> props(acx);
    if (!props.setResult(PropertyDict::Create(acx, InitialPropertyCapacity)))
        return ErrorVal();

    // Allocate global.
    Local<GlobalScope *> global(acx);
    if (!global.setResult(acx.create<GlobalScope>(delegates.handle(),
                                                  props.handle())))
    {
        return ErrorVal();
    }

    // Bind syntax handlers to global.
    if (!BindSyntaxHandlers(acx, global))
        return ErrorVal();

    return OkVal(global.get());
}

/* static */ void
GlobalScope::GetDelegates(ThreadContext *cx,
                          Handle<GlobalScope *> obj,
                          MutHandle<Array<Wobject *> *> delegatesOut)
{
    HashObject::GetDelegates(cx,
        Handle<HashObject *>::Convert(obj),
        delegatesOut);
}

/* static */ bool
GlobalScope::GetProperty(ThreadContext *cx,
                         Handle<GlobalScope *> obj,
                         Handle<String *> name,
                         MutHandle<PropertyDescriptor> result)
{
    return HashObject::GetProperty(cx,
        Handle<HashObject *>::Convert(obj),
        name, result);
}

/* static */ OkResult
GlobalScope::DefineProperty(ThreadContext *cx,
                            Handle<GlobalScope *> obj,
                            Handle<String *> name,
                            Handle<PropertyDescriptor> defn)
{
    return HashObject::DefineProperty(cx,
        Handle<HashObject *>::Convert(obj),
        name, defn);
}

// Declare a lift function for each syntax node type.
#define DECLARE_LIFT_FN_(name) \
    static OkResult Lift_##name( \
        ThreadContext *cx, \
        Handle<VM::LookupState *> lookupState, \
        Handle<VM::ScopeObject *> callerScope, \
        Handle<VM::NativeFunction *> nativeFunc, \
        Handle<VM::Wobject *> receiver, \
        ArrayHandle<VM::SyntaxTreeFragment *> stFrag, \
        MutHandle<VM::Box> resultOut);

    DECLARE_LIFT_FN_(File)
    //WHISPER_DEFN_SYNTAX_NODES(DECLARE_LIFT_FN_)

#undef DECLARE_LIFT_FN_

static OkResult
BindGlobalMethod(AllocationContext acx,
                 Handle<VM::GlobalScope *> obj,
                 VM::String *name,
                 VM::NativeOperativeFuncPtr opFunc)
{
    ThreadContext *cx = acx.threadContext();
    Local<VM::String *> rootedName(acx, name);

    // Allocate NativeFunction object.
    Local<VM::NativeFunction *> natF(cx);
    if (!natF.setResult(VM::NativeFunction::Create(acx, opFunc)))
        return ErrorVal();
    Local<VM::PropertyDescriptor> desc(cx, VM::PropertyDescriptor(natF.get()));

    // Bind method on global.
    if (!VM::GlobalScope::DefineProperty(cx, obj, rootedName, desc))
        return ErrorVal();

    return OkVal();
}

/* static */ OkResult
GlobalScope::BindSyntaxHandlers(AllocationContext acx,
                                Handle<GlobalScope *> obj)
{
    ThreadContext *cx = acx.threadContext();
    Local<VM::RuntimeState *> rtState(acx, cx->runtimeState());
    if (!BindGlobalMethod(acx, obj, rtState->nm_AtFile(), &Lift_File))
        return ErrorVal();

    return OkVal();
}

#define IMPL_LIFT_FN_(name) \
    static OkResult Lift_##name( \
        ThreadContext *cx, \
        Handle<VM::LookupState *> lookupState, \
        Handle<VM::ScopeObject *> callerScope, \
        Handle<VM::NativeFunction *> nativeFunc, \
        Handle<VM::Wobject *> receiver, \
        ArrayHandle<VM::SyntaxTreeFragment *> stFrag, \
        MutHandle<VM::Box> resultOut)

IMPL_LIFT_FN_(File)
{
    if (stFrag.length() != 1) {
        return cx->setExceptionRaised(
            "@File called with wrong number of arguments.");
    }

    WH_ASSERT(stFrag.get(0)->nodeType() == AST::File);

    Local<AST::PackedFileNode> fileNode(cx,
        AST::PackedFileNode(stFrag.get(0)->pst()->data(),
                            stFrag.get(0)->offset()));

    std::cerr << "Lift_File: Interpreting "
              << fileNode->numStatements()
              << " statements" << std::endl;
    for (uint32_t i = 0; i < fileNode->numStatements(); i++) {
        Local<AST::PackedBaseNode> stmtNode(cx, fileNode->statement(i));
        std::cerr << "Lift_File statement " << i << ":"
                  << AST::NodeTypeString(stmtNode->type())
                  << std::endl;
    }
    return OkVal();
}


} // namespace VM
} // namespace Whisper
