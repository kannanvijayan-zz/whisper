
#include "vm/wobject.hpp"
#include "vm/plain_object.hpp"
#include "vm/scope_object.hpp"
#include "vm/global_scope.hpp"
#include "vm/function.hpp"
#include "vm/lookup_state.hpp"
#include "vm/properties.hpp"

namespace Whisper {
namespace VM {


WobjectHooks const*
Wobject::getHooks() const
{
    HeapThing const* heapThing = HeapThing::From(this);

#define GET_HOOKS_(ObjKind) \
    if (heapThing->is##ObjKind()) \
        return reinterpret_cast<ObjKind const*>(this)->get##ObjKind##Hooks();
    WHISPER_DEFN_WOBJECT_KINDS(GET_HOOKS_)
#undef GET_HOOKS_

    WH_UNREACHABLE("Unknown object kind");
    return nullptr;
}

/* static */ uint32_t
Wobject::NumDelegates(AllocationContext acx,
                      Handle<Wobject*> obj)
{
    return obj->getHooks()->numDelegates(acx, obj);
}

/* static */ OkResult
Wobject::GetDelegates(AllocationContext acx,
                      Handle<Wobject*> obj,
                      MutHandle<Array<Wobject*>*> delegatesOut)
{
    return obj->getHooks()->getDelegates(acx, obj, delegatesOut);
}

/* static */ Result<bool>
Wobject::GetProperty(AllocationContext acx,
                     Handle<Wobject*> obj,
                     Handle<String*> name,
                     MutHandle<PropertyDescriptor> result)
{
    return obj->getHooks()->getProperty(acx, obj, name, result);
}

/* static */ OkResult
Wobject::DefineProperty(AllocationContext acx,
                        Handle<Wobject*> obj,
                        Handle<String*> name,
                        Handle<PropertyDescriptor> defn)
{
    return obj->getHooks()->defineProperty(acx, obj, name, defn);
}


/* static */ Result<bool>
Wobject::LookupProperty(
        AllocationContext acx,
        Handle<Wobject*> obj,
        Handle<String*> name,
        MutHandle<LookupState*> stateOut,
        MutHandle<PropertyDescriptor> defnOut)
{
    // Allocate a lookup state.
    Local<LookupState*> lookupState(acx);
    if (!lookupState.setResult(LookupState::Create(acx, obj, name)))
        return ErrorVal();

    // Recursive lookup through nodes.
    Local<LookupNode*> curNode(acx, lookupState->node());
    while (curNode.get() != nullptr) {
        // Check current node.
        Local<Wobject*> curObj(acx, curNode->object());
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
