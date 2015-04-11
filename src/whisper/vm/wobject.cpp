
#include "vm/wobject.hpp"
#include "vm/plain_object.hpp"
#include "vm/scope_object.hpp"
#include "vm/global_scope.hpp"
#include "vm/lookup_state.hpp"
#include "vm/properties.hpp"

namespace Whisper {
namespace VM {


/* static */ OkResult
Wobject::GetDelegates(AllocationContext acx,
                      Handle<Wobject *> obj,
                      MutHandle<Array<Wobject *> *> delegatesOut)
{
    HeapThing *heapThing = HeapThing::From(obj.get());
    if (heapThing->isPlainObject()) {
        Local<PlainObject *> plainObj(acx,
            reinterpret_cast<PlainObject *>(heapThing));
        PlainObject::GetDelegates(acx, plainObj, delegatesOut);
        return OkVal();
    }

    if (heapThing->isCallScope()) {
        Local<CallScope *> callObj(acx,
            reinterpret_cast<CallScope *>(heapThing));
        CallScope::GetDelegates(acx, callObj, delegatesOut);
        return OkVal();
    }

    if (heapThing->isModuleScope()) {
        Local<ModuleScope *> moduleObj(acx,
            reinterpret_cast<ModuleScope *>(heapThing));
        ModuleScope::GetDelegates(acx, moduleObj, delegatesOut);
        return OkVal();
    }

    if (heapThing->isGlobalScope()) {
        Local<GlobalScope *> globalObj(acx,
            reinterpret_cast<GlobalScope *>(heapThing));
        GlobalScope::GetDelegates(acx, globalObj, delegatesOut);
        return OkVal();
    }

    WH_UNREACHABLE("Unknown object kind");
    return ErrorVal();
}

/* static */ Result<bool>
Wobject::GetProperty(AllocationContext acx,
                     Handle<Wobject *> obj,
                     Handle<String *> name,
                     MutHandle<PropertyDescriptor> result)
{
    HeapThing *heapThing = HeapThing::From(obj.get());
    if (heapThing->isPlainObject()) {
        Local<PlainObject *> plainObj(acx,
            reinterpret_cast<PlainObject *>(heapThing));
        return OkVal(PlainObject::GetProperty(acx, plainObj, name, result));
    }

    if (heapThing->isCallScope()) {
        Local<CallScope *> callObj(acx,
            reinterpret_cast<CallScope *>(heapThing));
        return OkVal(CallScope::GetProperty(acx, callObj, name, result));
    }

    if (heapThing->isModuleScope()) {
        Local<ModuleScope *> moduleObj(acx,
            reinterpret_cast<ModuleScope *>(heapThing));
        return OkVal(ModuleScope::GetProperty(acx, moduleObj, name, result));
    }

    if (heapThing->isGlobalScope()) {
        Local<GlobalScope *> globalObj(acx,
            reinterpret_cast<GlobalScope *>(heapThing));
        return OkVal(GlobalScope::GetProperty(acx, globalObj, name, result));
    }

    WH_UNREACHABLE("Unknown object kind");
    return ErrorVal();
}

/* static */ OkResult
Wobject::DefineProperty(AllocationContext acx,
                        Handle<Wobject *> obj,
                        Handle<String *> name,
                        Handle<PropertyDescriptor> defn)
{
    HeapThing *heapThing = HeapThing::From(obj.get());
    if (heapThing->isPlainObject()) {
        Local<PlainObject *> plainObj(acx,
            reinterpret_cast<PlainObject *>(heapThing));
        return PlainObject::DefineProperty(acx, plainObj, name, defn);
    }

    if (heapThing->isCallScope()) {
        Local<CallScope *> callObj(acx,
            reinterpret_cast<CallScope *>(heapThing));
        return CallScope::DefineProperty(acx, callObj, name, defn);
    }

    if (heapThing->isModuleScope()) {
        Local<ModuleScope *> moduleObj(acx,
            reinterpret_cast<ModuleScope *>(heapThing));
        return ModuleScope::DefineProperty(acx, moduleObj, name, defn);
    }

    if (heapThing->isGlobalScope()) {
        Local<GlobalScope *> globalObj(acx,
            reinterpret_cast<GlobalScope *>(heapThing));
        return GlobalScope::DefineProperty(acx, globalObj, name, defn);
    }

    WH_UNREACHABLE("Unknown object kind");
    return ErrorVal();
}


/* static */ Result<bool>
Wobject::LookupProperty(
        AllocationContext acx,
        Handle<Wobject *> obj,
        Handle<String *> name,
        MutHandle<LookupState *> stateOut,
        MutHandle<PropertyDescriptor> defnOut)
{
    // Allocate a lookup state.
    Local<LookupState *> lookupState(acx);
    if (!lookupState.setResult(LookupState::Create(acx, obj, name)))
        return ErrorVal();

    // Recursive lookup through nodes.
    Local<LookupNode *> curNode(acx, lookupState->node());
    while (curNode.get() != nullptr) {
        // Check current node.
        Local<Wobject *> curObj(acx, curNode->object());
        Local<PropertyDescriptor> defn(acx);
        Result<bool> prop = Wobject::GetProperty(acx, curObj, name, &defn);
        if (!prop)
            return ErrorVal();

        // Property found on object.
        if (prop.value()) {
            defnOut.set(defn);
            stateOut.set(lookupState);
            return OkVal(true);
        }

        // Property not found on object, go to next lookup node.
        if (!LookupState::NextNode(acx, lookupState, &curNode))
            return ErrorVal();
    }

    // If we got here, no property was found.
    return OkVal(false);
}


} // namespace VM
} // namespace Whisper
