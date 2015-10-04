
#include <iostream>

#include "parser/syntax_defn.hpp"
#include "parser/packed_syntax.hpp"
#include "runtime_inlines.hpp"
#include "vm/global_scope.hpp"
#include "vm/runtime_state.hpp"
#include "vm/function.hpp"
#include "vm/plain_object.hpp"
#include "interp/interpreter.hpp"
#include "interp/object_behaviour.hpp"

namespace Whisper {
namespace Interp {


static OkResult
BindRootDelegateSyntaxHandlers(AllocationContext acx, VM::Wobject* obj);

Result<VM::Wobject*>
CreateRootDelegate(AllocationContext acx)
{
    // Create an empty array of delegates.
    Local<VM::Array<VM::Wobject*>*> delegates(acx);
    if (!delegates.setResult(VM::Array<VM::Wobject*>::CreateEmpty(acx))) {
        SpewInterpError("Could not allocate root delegate's "
                        "empty delegate array.");
        return ErrorVal();
    }

    // Create a plain object.
    Local<VM::PlainObject*> plainObj(acx);
    if (!plainObj.setResult(VM::PlainObject::Create(acx, delegates))) {
        SpewInterpError("Could not allocate root delegate.");
        return ErrorVal();
    }

    Local<VM::Wobject*> obj(acx, plainObj.get());
    // Bind root delegate syntax handler onto it.
    if (!BindRootDelegateSyntaxHandlers(acx, obj)) {
        SpewInterpError("Failed to bind root delegate syntax handlers.");
        return ErrorVal();
    }

    SpewInterpNote("Created root delegate.");
    return OkVal(obj.get());
}

// Declare a lift function for each syntax node type.
#define DECLARE_OBJSYNTAX_FN_(name) \
    static VM::ControlFlow ObjSyntax_##name( \
        ThreadContext* cx, \
        Handle<VM::NativeCallInfo> callInfo, \
        ArrayHandle<VM::SyntaxNodeRef> args);

    DECLARE_OBJSYNTAX_FN_(DotExpr)

#undef DECLARE_OBJSYNTAX_FN_

static OkResult
BindRootDelegateSyntaxMethod(AllocationContext acx,
                             Handle<VM::Wobject*> obj,
                             VM::String* name,
                             VM::NativeOperativeFuncPtr opFunc)
{
    Local<VM::String*> rootedName(acx, name);

    // Allocate NativeFunction object.
    Local<VM::NativeFunction*> natF(acx);
    if (!natF.setResult(VM::NativeFunction::Create(acx, opFunc)))
        return ErrorVal();
    Local<VM::PropertyDescriptor> desc(acx, VM::PropertyDescriptor(natF.get()));

    // Bind method on global.
    if (!VM::Wobject::DefineProperty(acx, obj, rootedName, desc))
        return ErrorVal();

    return OkVal();
}

static OkResult
BindRootDelegateSyntaxHandlers(AllocationContext acx, VM::Wobject* obj)
{
    Local<VM::Wobject*> rootedObj(acx, obj);

    ThreadContext* cx = acx.threadContext();
    Local<VM::RuntimeState*> rtState(acx, cx->runtimeState());

#define BIND_OBJSYNTAX_METHOD_(name) \
    do { \
        if (!BindRootDelegateSyntaxMethod(acx, rootedObj, \
                rtState->nm_At##name(), &ObjSyntax_##name)) \
        { \
            return ErrorVal(); \
        } \
    } while(false)

    BIND_OBJSYNTAX_METHOD_(DotExpr);

#undef BIND_OBJSYNTAX_METHOD_

    return OkVal();
}

#define IMPL_OBJSYNTAX_FN_(name) \
    static VM::ControlFlow ObjSyntax_##name( \
        ThreadContext* cx, \
        Handle<VM::NativeCallInfo> callInfo, \
        ArrayHandle<VM::SyntaxNodeRef> args)

IMPL_OBJSYNTAX_FN_(DotExpr)
{
    if (args.length() != 1) {
        return cx->setExceptionRaised(
            "object.@DotExpr called with wrong number of arguments.");
    }

    WH_ASSERT(args.get(0).nodeType() == AST::DotExpr);

    Local<VM::SyntaxNodeRef> stRef(cx, args.get(0));
    Local<VM::PackedSyntaxTree*> pst(cx, stRef->pst());
    Local<AST::PackedDotExprNode> dotExprNode(cx,
        AST::PackedDotExprNode(pst->data(), stRef->offset()));

    // Look up the name on the receiver.
    Local<VM::ValBox> receiver(cx, callInfo->receiver());
    Local<VM::String*> name(cx, pst->getConstantString(dotExprNode->nameCid()));

    VM::ControlFlow lookupFlow = GetValueProperty(cx, receiver, name);
    WH_ASSERT(lookupFlow.isPropertyLookupResult());

    if (lookupFlow.isVoid())
        return cx->setExceptionRaised("Name not found on object.");

    return lookupFlow;
}


} // namespace Interp
} // namespace Whisper
