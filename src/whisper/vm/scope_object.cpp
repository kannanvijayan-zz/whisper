
#include "runtime_inlines.hpp"
#include "vm/scope_object.hpp"
#include "vm/global_scope.hpp"

namespace Whisper {
namespace VM {


/* static */ Result<CallScope*>
CallScope::Create(AllocationContext acx,
                  Handle<ScopeObject*> callerScope)
{
    // Allocate array of delegates containing caller scope.
    Local<Array<Wobject*>*> delegates(acx);
    if (!delegates.setResult(Array<Wobject*>::CreateFill(acx, 1, nullptr)))
        return ErrorVal();
    delegates->set(0, callerScope.get());

    // Allocate a dictionary.
    Local<PropertyDict*> props(acx);
    if (!props.setResult(PropertyDict::Create(acx, InitialPropertyCapacity)))
        return ErrorVal();

    return acx.create<CallScope>(delegates.handle(), props.handle());
}

/* static */ void
CallScope::GetDelegates(AllocationContext acx,
                        Handle<CallScope*> obj,
                        MutHandle<Array<Wobject*>*> delegatesOut)
{
    HashObject::GetDelegates(acx,
        Handle<HashObject*>::Convert(obj),
        delegatesOut);
}

/* static */ bool
CallScope::GetProperty(AllocationContext acx,
                       Handle<CallScope*> obj,
                       Handle<String*> name,
                       MutHandle<PropertyDescriptor> result)
{
    return HashObject::GetProperty(acx,
        Handle<HashObject*>::Convert(obj),
        name, result);
}

/* static */ OkResult
CallScope::DefineProperty(AllocationContext acx,
                          Handle<CallScope*> obj,
                           Handle<String*> name,
                           Handle<PropertyDescriptor> defn)
{
    return HashObject::DefineProperty(acx,
        Handle<HashObject*>::Convert(obj),
        name, defn);
}


/* static */ Result<ModuleScope*>
ModuleScope::Create(AllocationContext acx, Handle<GlobalScope*> global)
{
    // Allocate array of delegates containing caller scope.
    Local<Array<Wobject*>*> delegates(acx);
    if (!delegates.setResult(Array<Wobject*>::CreateFill(acx, 1, nullptr)))
        return ErrorVal();
    delegates->set(0, global.get());

    // Allocate a dictionary.
    Local<PropertyDict*> props(acx);
    if (!props.setResult(PropertyDict::Create(acx, InitialPropertyCapacity)))
        return ErrorVal();

    return acx.create<ModuleScope>(delegates.handle(), props.handle());
}

/* static */ void
ModuleScope::GetDelegates(AllocationContext acx,
                          Handle<ModuleScope*> obj,
                          MutHandle<Array<Wobject*>*> delegatesOut)
{
    HashObject::GetDelegates(acx,
        Handle<HashObject*>::Convert(obj),
        delegatesOut);
}

/* static */ bool
ModuleScope::GetProperty(AllocationContext acx,
                         Handle<ModuleScope*> obj,
                         Handle<String*> name,
                         MutHandle<PropertyDescriptor> result)
{
    return HashObject::GetProperty(acx,
        Handle<HashObject*>::Convert(obj),
        name, result);
}

/* static */ OkResult
ModuleScope::DefineProperty(AllocationContext acx,
                            Handle<ModuleScope*> obj,
                            Handle<String*> name,
                            Handle<PropertyDescriptor> defn)
{
    return HashObject::DefineProperty(acx,
        Handle<HashObject*>::Convert(obj),
        name, defn);
}


} // namespace VM
} // namespace Whisper
