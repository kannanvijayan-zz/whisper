
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

WobjectHooks const*
CallScope::getCallScopeHooks() const
{
    return hashObjectHooks();
}


/* static */ Result<BlockScope*>
BlockScope::Create(AllocationContext acx,
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

    return acx.create<BlockScope>(delegates.handle(), props.handle());
}

WobjectHooks const*
BlockScope::getBlockScopeHooks() const
{
    return hashObjectHooks();
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

WobjectHooks const*
ModuleScope::getModuleScopeHooks() const
{
    return hashObjectHooks();
}


} // namespace VM
} // namespace Whisper
