
#include "vm/wobject.hpp"
#include "vm/plain_object.hpp"
#include "vm/scope_object.hpp"
#include "vm/lookup_state.hpp"
#include "vm/properties.hpp"

namespace Whisper {
namespace VM {


/* static */ OkResult
Wobject::GetDelegates(ThreadContext *cx,
                      Handle<Wobject *> obj,
                      MutHandle<Array<Wobject *> *> delegatesOut)
{
    HeapThing *heapThing = HeapThing::From(obj.get());
    if (heapThing->isPlainObject()) {
        Local<PlainObject *> plainObj(cx,
            reinterpret_cast<PlainObject *>(heapThing));
        PlainObject::GetDelegates(cx, plainObj, delegatesOut);
        return OkResult::Ok();
    }

    if (heapThing->isCallScope()) {
        Local<CallScope *> callObj(cx,
            reinterpret_cast<CallScope *>(heapThing));
        CallScope::GetDelegates(cx, callObj, delegatesOut);
        return OkResult::Ok();
    }

    if (heapThing->isModuleScope()) {
        Local<ModuleScope *> moduleObj(cx,
            reinterpret_cast<ModuleScope *>(heapThing));
        ModuleScope::GetDelegates(cx, moduleObj, delegatesOut);
        return OkResult::Ok();
    }

    if (heapThing->isGlobalScope()) {
        Local<GlobalScope *> globalObj(cx,
            reinterpret_cast<GlobalScope *>(heapThing));
        GlobalScope::GetDelegates(cx, globalObj, delegatesOut);
        return OkResult::Ok();
    }

    WH_UNREACHABLE("Unknown object kind");
    return OkResult::Error();
}

/* static */ Result<bool>
Wobject::GetProperty(ThreadContext *cx,
                     Handle<Wobject *> obj,
                     Handle<String *> name,
                     MutHandle<PropertyDescriptor> result)
{
    HeapThing *heapThing = HeapThing::From(obj.get());
    if (heapThing->isPlainObject()) {
        Local<PlainObject *> plainObj(cx,
            reinterpret_cast<PlainObject *>(heapThing));
        return Result<bool>::Value(PlainObject::GetProperty(cx, plainObj,
                                                            name, result));
    }

    if (heapThing->isCallScope()) {
        Local<CallScope *> callObj(cx,
            reinterpret_cast<CallScope *>(heapThing));
        return Result<bool>::Value(CallScope::GetProperty(cx, callObj,
                                                          name, result));
    }

    if (heapThing->isModuleScope()) {
        Local<ModuleScope *> moduleObj(cx,
            reinterpret_cast<ModuleScope *>(heapThing));
        return Result<bool>::Value(ModuleScope::GetProperty(cx, moduleObj,
                                                            name, result));
    }

    if (heapThing->isGlobalScope()) {
        Local<GlobalScope *> globalObj(cx,
            reinterpret_cast<GlobalScope *>(heapThing));
        return Result<bool>::Value(GlobalScope::GetProperty(cx, globalObj,
                                                             name, result));
    }

    WH_UNREACHABLE("Unknown object kind");
    return Result<bool>::Error();
}

/* static */ OkResult
Wobject::DefineProperty(ThreadContext *cx,
                        Handle<Wobject *> obj,
                        Handle<String *> name,
                        Handle<PropertyDescriptor> defn)
{
    HeapThing *heapThing = HeapThing::From(obj.get());
    if (heapThing->isPlainObject()) {
        Local<PlainObject *> plainObj(cx,
            reinterpret_cast<PlainObject *>(heapThing));
        return PlainObject::DefineProperty(cx, plainObj, name, defn);
    }

    if (heapThing->isCallScope()) {
        Local<CallScope *> callObj(cx,
            reinterpret_cast<CallScope *>(heapThing));
        return CallScope::DefineProperty(cx, callObj, name, defn);
    }

    if (heapThing->isModuleScope()) {
        Local<ModuleScope *> moduleObj(cx,
            reinterpret_cast<ModuleScope *>(heapThing));
        return ModuleScope::DefineProperty(cx, moduleObj, name, defn);
    }

    if (heapThing->isGlobalScope()) {
        Local<GlobalScope *> globalObj(cx,
            reinterpret_cast<GlobalScope *>(heapThing));
        return GlobalScope::DefineProperty(cx, globalObj, name, defn);
    }

    WH_UNREACHABLE("Unknown object kind");
    return OkResult::Error();
}


/* static */ Result<bool>
Wobject::LookupProperty(
        ThreadContext* cx,
        Handle<Wobject *> obj,
        Handle<String *> name,
        MutHandle<LookupState *> stateOut,
        MutHandle<PropertyDescriptor> defnOut)
{
    AllocationContext acx = cx->inHatchery();

    // Allocate a lookup state.
    Local<LookupState *> lookupState(cx);
    if (!lookupState.setResult(LookupState::Create(acx, obj, name)))
        return Result<bool>::Error();

    // Recursive lookup through nodes.
    Local<LookupNode *> curNode(cx, lookupState->node());
    for (;;) {
        // Check current node.
        Local<Wobject *> curObj(cx, curNode->object());
        Local<PropertyDescriptor> defn(cx);
        Result<bool> prop = Wobject::GetProperty(cx, obj, name, &defn);
        if (!prop)
            return Result<bool>::Error();

        // Property found on object.
        if (prop.value()) {
            defnOut.set(defn);
            stateOut.set(lookupState);
            return Result<bool>::Value(true);
        }

        // Property not found on object, go to next lookup node.
        if (!LookupState::NextNode(acx, lookupState, &curNode))
            return Result<bool>::Error();
    }

    // If we got here, no property was found.
    return Result<bool>::Value(false);
}


} // namespace VM
} // namespace Whisper
