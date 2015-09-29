
#include "parser/packed_syntax.hpp"
#include "runtime_inlines.hpp"
#include "vm/core.hpp"
#include "vm/function.hpp"

namespace Whisper {
namespace VM {


bool
Function::isApplicative() const
{
    if (isNative())
        return asNative()->isApplicative();

    if (isScripted())
        return asScripted()->isApplicative();

    WH_UNREACHABLE("Unknown function type.");
    return false;
}


/* static */ Result<NativeFunction*>
NativeFunction::Create(AllocationContext acx, NativeApplicativeFuncPtr app)
{
    return acx.create<NativeFunction>(app);
}

/* static */ Result<NativeFunction*>
NativeFunction::Create(AllocationContext acx, NativeOperativeFuncPtr oper)
{
    return acx.create<NativeFunction>(oper);
}


/* static */ Result<ScriptedFunction*>
ScriptedFunction::Create(AllocationContext acx,
                         Handle<PackedSyntaxTree*> pst,
                         uint32_t offset,
                         Handle<ScopeObject*> scopeChain,
                         bool isOperative)
{
    WH_ASSERT(SyntaxNodeRef(pst, offset).nodeType() == AST::DefStmt);
    return acx.create<ScriptedFunction>(pst, offset, scopeChain, isOperative);
}


/* static */ Result<FunctionObject*>
FunctionObject::Create(AllocationContext acx,
                       Handle<Function*> func,
                       Handle<Wobject*> receiver,
                       Handle<LookupState*> lookupState)
{
    // Allocate empty array of delegates.
    // TODO: set delegates to the default function delegate.
    Local<Array<Wobject*>*> delegates(acx);
    if (!delegates.setResult(Array<Wobject*>::CreateEmpty(acx)))
        return ErrorVal();

    // Allocate a dictionary.
    Local<PropertyDict*> props(acx);
    if (!props.setResult(PropertyDict::Create(acx, InitialPropertyCapacity)))
        return ErrorVal();

    return acx.create<FunctionObject>(delegates.handle(), props.handle(),
                                      func, receiver, lookupState);
}

/* static */ uint32_t
FunctionObject::NumDelegates(AllocationContext acx,
                             Handle<FunctionObject*> obj)
{
    return HashObject::NumDelegates(acx, Handle<HashObject*>::Convert(obj));
}

/* static */ void
FunctionObject::GetDelegates(AllocationContext acx,
                             Handle<FunctionObject*> obj,
                             MutHandle<Array<Wobject*>*> delegatesOut)
{
    HashObject::GetDelegates(acx,
        Handle<HashObject*>::Convert(obj),
        delegatesOut);
}

/* static */ bool
FunctionObject::GetProperty(AllocationContext acx,
                            Handle<FunctionObject*> obj,
                            Handle<String*> name,
                            MutHandle<PropertyDescriptor> result)
{
    return HashObject::GetProperty(acx,
        Handle<HashObject*>::Convert(obj),
        name, result);
}

/* static */ OkResult
FunctionObject::DefineProperty(AllocationContext acx,
                               Handle<FunctionObject*> obj,
                               Handle<String*> name,
                               Handle<PropertyDescriptor> defn)
{
    return HashObject::DefineProperty(acx,
        Handle<HashObject*>::Convert(obj),
        name, defn);
}


} // namespace VM
} // namespace Whisper
