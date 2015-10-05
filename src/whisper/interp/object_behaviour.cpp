
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
BindRootDelegateMethods(AllocationContext acx, VM::Wobject* obj);

static OkResult
BindImmIntMethods(AllocationContext acx, VM::Wobject* obj);

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
    if (!BindRootDelegateMethods(acx, obj)) {
        SpewInterpError("Failed to bind root delegate syntax handlers.");
        return ErrorVal();
    }

    SpewInterpNote("Created root delegate.");
    return OkVal(obj.get());
}

Result<VM::Wobject*>
CreateImmIntDelegate(AllocationContext acx,
                     Handle<VM::Wobject*> rootDelegate)
{
    // Create an empty array of delegates.
    Local<VM::Array<VM::Wobject*>*> delegates(acx);
    if (!delegates.setResult(VM::Array<VM::Wobject*>::CreateCopy(acx,
                                ArrayHandle<VM::Wobject*>(rootDelegate))))
    {
        SpewInterpError("Could not allocate immediate integer delegate's "
                        "delegate array.");
        return ErrorVal();
    }

    // Create a plain object.
    Local<VM::PlainObject*> plainObj(acx);
    if (!plainObj.setResult(VM::PlainObject::Create(acx, delegates))) {
        SpewInterpError("Could not allocate immediate integer delegate.");
        return ErrorVal();
    }

    Local<VM::Wobject*> obj(acx, plainObj.get());
    // Bind root delegate syntax handler onto it.
    if (!BindImmIntMethods(acx, obj)) {
        SpewInterpError("Failed to bind immediate integer methods.");
        return ErrorVal();
    }

    SpewInterpNote("Created immediate integer delegate.");
    return OkVal(obj.get());
}

// Declare a lift function for each syntax node type.
#define DECLARE_OPERATIVE_FN_(name) \
    static VM::ControlFlow name( \
        ThreadContext* cx, \
        Handle<VM::NativeCallInfo> callInfo, \
        ArrayHandle<VM::SyntaxNodeRef> args);

#define DECLARE_APPLICATIVE_FN_(name) \
    static VM::ControlFlow name( \
        ThreadContext* cx, \
        Handle<VM::NativeCallInfo> callInfo, \
        ArrayHandle<VM::ValBox> args);

    DECLARE_OPERATIVE_FN_(ObjSyntax_DotExpr)

    DECLARE_APPLICATIVE_FN_(ImmInt_PosExpr)
    DECLARE_APPLICATIVE_FN_(ImmInt_NegExpr)
    DECLARE_APPLICATIVE_FN_(ImmInt_AddExpr)
    DECLARE_APPLICATIVE_FN_(ImmInt_SubExpr)
    DECLARE_APPLICATIVE_FN_(ImmInt_MulExpr)
    DECLARE_APPLICATIVE_FN_(ImmInt_DivExpr)

#undef DECLARE_OPERATIVE_FN_
#undef DECLARE_APPLICATIVE_FN_

static OkResult
BindOperativeMethod(AllocationContext acx,
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
BindApplicativeMethod(AllocationContext acx,
                      Handle<VM::Wobject*> obj,
                      VM::String* name,
                      VM::NativeApplicativeFuncPtr appFunc)
{
    Local<VM::String*> rootedName(acx, name);

    // Allocate NativeFunction object.
    Local<VM::NativeFunction*> natF(acx);
    if (!natF.setResult(VM::NativeFunction::Create(acx, appFunc)))
        return ErrorVal();
    Local<VM::PropertyDescriptor> desc(acx, VM::PropertyDescriptor(natF.get()));

    // Bind method on global.
    if (!VM::Wobject::DefineProperty(acx, obj, rootedName, desc))
        return ErrorVal();

    return OkVal();
}

static OkResult
BindRootDelegateMethods(AllocationContext acx, VM::Wobject* obj)
{
    Local<VM::Wobject*> rootedObj(acx, obj);

    ThreadContext* cx = acx.threadContext();
    Local<VM::RuntimeState*> rtState(acx, cx->runtimeState());

#define BIND_OBJSYNTAX_METHOD_(name) \
    do { \
        if (!BindOperativeMethod(acx, rootedObj, rtState->nm_At##name(), \
                                 &ObjSyntax_##name)) \
        { \
            return ErrorVal(); \
        } \
    } while(false)

    BIND_OBJSYNTAX_METHOD_(DotExpr);

#undef BIND_OBJSYNTAX_METHOD_

    return OkVal();
}

static OkResult
BindImmIntMethods(AllocationContext acx, VM::Wobject* obj)
{
    Local<VM::Wobject*> rootedObj(acx, obj);

    ThreadContext* cx = acx.threadContext();
    Local<VM::RuntimeState*> rtState(acx, cx->runtimeState());

#define BIND_IMM_INT_METHOD_(name) \
    do { \
        if (!BindApplicativeMethod(acx, rootedObj, rtState->nm_At##name(), \
                                   &ImmInt_##name)) \
        { \
            return ErrorVal(); \
        } \
    } while(false)

    BIND_IMM_INT_METHOD_(PosExpr);
    BIND_IMM_INT_METHOD_(NegExpr);
    BIND_IMM_INT_METHOD_(AddExpr);
    BIND_IMM_INT_METHOD_(SubExpr);
    BIND_IMM_INT_METHOD_(MulExpr);
    BIND_IMM_INT_METHOD_(DivExpr);

#undef BIND_IMM_INT_METHOD_

    return OkVal();
}

#define IMPL_OPERATIVE_FN_(name) \
    static VM::ControlFlow name( \
        ThreadContext* cx, \
        Handle<VM::NativeCallInfo> callInfo, \
        ArrayHandle<VM::SyntaxNodeRef> args)

#define IMPL_APPLICATIVE_FN_(name) \
    static VM::ControlFlow name( \
        ThreadContext* cx, \
        Handle<VM::NativeCallInfo> callInfo, \
        ArrayHandle<VM::ValBox> args)

IMPL_OPERATIVE_FN_(ObjSyntax_DotExpr)
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

IMPL_APPLICATIVE_FN_(ImmInt_PosExpr)
{
    if (args.length() != 0) {
        return cx->setExceptionRaised(
            "immInt.@PosExpr called with wrong number of arguments.");
    }

    // Receiver should be immediate integer.
    Local<VM::ValBox> receiver(cx, callInfo->receiver());
    if (!receiver->isInteger()) {
        return cx->setExceptionRaised(
            "immInt.@PosExpr called on non-immediate-integer.");
    }

    // Posate the value.
    int64_t posInt = +receiver->integer();
    WH_ASSERT(VM::ValBox::IntegerInRange(posInt));
    return VM::ControlFlow::Value(VM::ValBox::Integer(posInt));
}

IMPL_APPLICATIVE_FN_(ImmInt_NegExpr)
{
    if (args.length() != 0) {
        return cx->setExceptionRaised(
            "immInt.@NegExpr called with wrong number of arguments.");
    }

    // Receiver should be immediate integer.
    Local<VM::ValBox> receiver(cx, callInfo->receiver());
    if (!receiver->isInteger()) {
        return cx->setExceptionRaised(
            "immInt.@NegExpr called on non-immediate-integer.");
    }

    // Negate the value.
    int64_t negInt = -receiver->integer();

    // TODO: On ImmInt negate overflow, create and return BigInt object.
    if (!VM::ValBox::IntegerInRange(negInt))
        return cx->setExceptionRaised("immInt.@NegExpr result overflows.");

    return VM::ControlFlow::Value(VM::ValBox::Integer(negInt));
}

IMPL_APPLICATIVE_FN_(ImmInt_AddExpr)
{
    if (args.length() != 1) {
        return cx->setExceptionRaised(
            "immInt.@AddExpr called with wrong number of arguments.");
    }

    // Receiver should be immediate integer.
    Local<VM::ValBox> receiver(cx, callInfo->receiver());
    if (!receiver->isInteger()) {
        return cx->setExceptionRaised(
            "immInt.@AddExpr called on non-immediate-integer.");
    }

    // Check args[0].
    Local<VM::ValBox> arg(cx, args[0]);
    if (arg->isInteger()) {
        int64_t lhsInt = receiver->integer();
        int64_t rhsInt = arg->integer();
        int64_t sumInt = lhsInt + rhsInt;
        // TODO: Handle overflow.
        if (!VM::ValBox::IntegerInRange(sumInt))
            return cx->setExceptionRaised("immInt.@AddExpr result overflows.");
        return VM::ControlFlow::Value(VM::ValBox::Integer(sumInt));
    }

    // TODO: On ImmInt negate overflow, create and return BigInt object.
    return cx->setExceptionRaised("Integer add can only handle "
                                  "integer immediates for now");
}

IMPL_APPLICATIVE_FN_(ImmInt_SubExpr)
{
    if (args.length() != 1) {
        return cx->setExceptionRaised(
            "immInt.@SubExpr called with wrong number of arguments.");
    }

    // Receiver should be immediate integer.
    Local<VM::ValBox> receiver(cx, callInfo->receiver());
    if (!receiver->isInteger()) {
        return cx->setExceptionRaised(
            "immInt.@SubExpr called on non-immediate-integer.");
    }

    // Check args[0].
    Local<VM::ValBox> arg(cx, args[0]);
    if (arg->isInteger()) {
        int64_t lhsInt = receiver->integer();
        int64_t rhsInt = arg->integer();
        int64_t diffInt = lhsInt - rhsInt;
        // TODO: Handle overflow.
        if (!VM::ValBox::IntegerInRange(diffInt))
            return cx->setExceptionRaised("immInt.@SubExpr result overflows.");
        return VM::ControlFlow::Value(VM::ValBox::Integer(diffInt));
    }

    // TODO: On ImmInt negate overflow, create and return BigInt object.
    return cx->setExceptionRaised("Integer subtract can only handle "
                                  "integer immediates for now");
}

IMPL_APPLICATIVE_FN_(ImmInt_MulExpr)
{
    if (args.length() != 1) {
        return cx->setExceptionRaised(
            "immInt.@MulExpr called with wrong number of arguments.");
    }

    // Receiver should be immediate integer.
    Local<VM::ValBox> receiver(cx, callInfo->receiver());
    if (!receiver->isInteger()) {
        return cx->setExceptionRaised(
            "immInt.@MulExpr called on non-immediate-integer.");
    }

    // Check args[0].
    Local<VM::ValBox> arg(cx, args[0]);
    if (arg->isInteger()) {
        int64_t lhsInt = receiver->integer();
        int64_t rhsInt = arg->integer();
        int64_t prodInt = lhsInt * rhsInt;
        // TODO: Handle overflow.
        if (!VM::ValBox::IntegerInRange(prodInt))
            return cx->setExceptionRaised("immInt.@MulExpr result overflows.");
        return VM::ControlFlow::Value(VM::ValBox::Integer(prodInt));
    }

    // TODO: On ImmInt negate overflow, create and return BigInt object.
    return cx->setExceptionRaised("Integer multiply can only handle "
                                  "integer immediates for now");
}

IMPL_APPLICATIVE_FN_(ImmInt_DivExpr)
{
    if (args.length() != 1) {
        return cx->setExceptionRaised(
            "immInt.@DivExpr called with wrong number of arguments.");
    }

    // Receiver should be immediate integer.
    Local<VM::ValBox> receiver(cx, callInfo->receiver());
    if (!receiver->isInteger()) {
        return cx->setExceptionRaised(
            "immInt.@DivExpr called on non-immediate-integer.");
    }

    // Check args[0].
    Local<VM::ValBox> arg(cx, args[0]);
    if (arg->isInteger()) {
        int64_t lhsInt = receiver->integer();
        int64_t rhsInt = arg->integer();
        int64_t quotInt = lhsInt / rhsInt;
        // TODO: Handle overflow, divide-by-zero, non-integer results,
        // basically everything!
        if (!VM::ValBox::IntegerInRange(quotInt))
            return cx->setExceptionRaised("immInt.@DivExpr result overflows.");
        return VM::ControlFlow::Value(VM::ValBox::Integer(quotInt));
    }

    // TODO: On ImmInt negate overflow, create and return BigInt object.
    return cx->setExceptionRaised("Integer divide can only handle "
                                  "integer immediates for now");
}

} // namespace Interp
} // namespace Whisper
