
#include "runtime_inlines.hpp"
#include "vm/core.hpp"
#include "vm/function.hpp"

namespace Whisper {
namespace VM {


bool
Function::isApplicative() const
{
    if (isNative()) {
        return asNative()->isApplicative();
    }

    WH_UNREACHABLE("Unknown function type.");
    return false;
}


Result<NativeFunction *>
NativeFunction::Create(AllocationContext acx,
                       NativeApplicativeFuncPtr *applicative)
{
    return acx.create<NativeFunction>(applicative);
}

Result<NativeFunction *>
NativeFunction::Create(AllocationContext acx,
                       NativeOperativeFuncPtr *operative)
{
    return acx.create<NativeFunction>(operative);
}


/* static */ Result<ScriptedFunction *>
ScriptedFunction::Create(AllocationContext acx,
                         Handle<SyntaxTreeFragment *> definition,
                         Handle<ScopeObject *> scopeChain,
                         bool isOperative)
{
    return acx.create<ScriptedFunction>(definition, scopeChain, isOperative);
}


/* static */ Result<FunctionObject *>
FunctionObject::Create(AllocationContext acx, Handle<Function *> func)
{
    // Allocate empty array of delegates.
    Local<Array<Wobject *> *> delegates(acx);
    if (!delegates.setResult(Array<Wobject *>::Create(acx, 0,
            static_cast<Wobject*>(nullptr))))
    {
        return Result<FunctionObject *>::Error();
    }

    // Allocate a dictionary.
    Local<PropertyDict *> props(acx);
    if (!props.setResult(PropertyDict::Create(acx, InitialPropertyCapacity)))
        return Result<FunctionObject *>::Error();

    return acx.create<FunctionObject>(delegates.handle(), props.handle(),
                                      func);
}

/* static */ void
FunctionObject::GetDelegates(ThreadContext *cx,
                             Handle<FunctionObject *> obj,
                             MutHandle<Array<Wobject *> *> delegatesOut)
{
    HashObject::GetDelegates(cx,
        Handle<HashObject *>::Convert(obj),
        delegatesOut);
}

/* static */ bool
FunctionObject::GetProperty(ThreadContext *cx,
                            Handle<FunctionObject *> obj,
                            Handle<String *> name,
                            MutHandle<PropertyDescriptor> result)
{
    return HashObject::GetProperty(cx,
        Handle<HashObject *>::Convert(obj),
        name, result);
}

/* static */ OkResult
FunctionObject::DefineProperty(ThreadContext *cx,
                               Handle<FunctionObject *> obj,
                               Handle<String *> name,
                               Handle<PropertyDescriptor> defn)
{
    return HashObject::DefineProperty(cx,
        Handle<HashObject *>::Convert(obj),
        name, defn);
}


} // namespace VM
} // namespace Whisper
