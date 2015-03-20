
#include "vm/wobject.hpp"
#include "vm/plain_object.hpp"
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

    WH_UNREACHABLE("Unknown object kind");
    return OkResult::Error();
}

/* static */ Result<bool>
Wobject::LookupProperty(ThreadContext *cx,
                        Handle<Wobject *> obj,
                        Handle<String *> name,
                        MutHandle<PropertyDescriptor> result)
{
    HeapThing *heapThing = HeapThing::From(obj.get());
    if (heapThing->isPlainObject()) {
        Local<PlainObject *> plainObj(cx,
            reinterpret_cast<PlainObject *>(heapThing));
        bool got = PlainObject::LookupProperty(cx, plainObj, name, result);
        return Result<bool>::Value(got);
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

    WH_UNREACHABLE("Unknown object kind");
    return OkResult::Error();
}


/* static */ Result<bool>
Wobject::GetPropertyDescriptor(
        ThreadContext* cx,
        Handle<Wobject *> obj,
        Handle<String *> name,
        MutHandle<LookupState *> stateOut,
        MutHandle<PropertyDescriptor> defnOut)
{
    AllocationContext acx = cx->inHatchery();

    // Allocate a lookup state.
    Result<LookupState *> maybeLookupState =
        LookupState::Create(acx, obj, name);
    if (!maybeLookupState)
        return Result<bool>::Error();
    Local<LookupState *> lookupState(cx, maybeLookupState.value());

    // Recursive lookup through nodes.
    Local<LookupNode *> curNode(cx, lookupState->node());
    for (;;) {
        // Check current node.
        Local<Wobject *> curObj(cx, curNode->object());
        Local<PropertyDescriptor> defn(cx);
        Result<bool> prop = Wobject::LookupProperty(cx, obj, name,
                                                    defn.mutHandle());
        if (!prop)
            return Result<bool>::Error();

        // Property found on object.
        if (prop.value()) {
            defnOut.set(defn);
            stateOut.set(lookupState);
            return Result<bool>::Value(true);
        }

        // Property not found on object, go to next lookup node.
        if (!LookupState::NextNode(acx, lookupState, curNode))
            return Result<bool>::Error();
    }

    // If we got here, no property was found.
    return Result<bool>::Value(false);
}


} // namespace VM
} // namespace Whisper
