#ifndef WHISPER__VM__WOBJECT_HPP
#define WHISPER__VM__WOBJECT_HPP

#include "vm/core.hpp"
#include "vm/predeclare.hpp"
#include "vm/properties.hpp"
#include "vm/array.hpp"

/**
 * A Wobject is the base type for all objects visible to the runtime.
 */

namespace Whisper {
namespace VM {

typedef uint32_t (*WobjectHook_NumDelegates)(
        AllocationContext acx,
        Handle<Wobject*> obj);

typedef OkResult (*WobjectHook_GetDelegates)(
        AllocationContext acx,
        Handle<Wobject*> obj,
        MutHandle<Array<Wobject*>*> delegatesOut);

typedef Result<bool> (*WobjectHook_GetProperty)(
        AllocationContext acx,
        Handle<Wobject*> obj,
        Handle<String*> name,
        MutHandle<PropertyDescriptor> result);

typedef OkResult (*WobjectHook_DefineProperty)(
        AllocationContext acx,
        Handle<Wobject*> obj,
        Handle<String*> name,
        Handle<PropertyDescriptor> defn);

struct WobjectHooks
{
    WobjectHook_NumDelegates   numDelegates;
    WobjectHook_GetDelegates   getDelegates;
    WobjectHook_GetProperty    getProperty;
    WobjectHook_DefineProperty defineProperty;
};

#define WHISPER_DEFN_WOBJECT_KINDS(_) \
    _(PlainObject) \
    _(CallScope) \
    _(BlockScope) \
    _(ModuleScope) \
    _(GlobalScope) \
    _(FunctionObject) \
    _(ContObject)

class Wobject
{
  protected:
    Wobject() {
        WH_ASSERT(IsWobject(HeapThing::From(this)));
    }

  public:
    WobjectHooks const* getHooks() const;

    static uint32_t NumDelegates(
            AllocationContext acx,
            Handle<Wobject*> obj);

    static OkResult GetDelegates(
            AllocationContext acx,
            Handle<Wobject*> obj,
            MutHandle<Array<Wobject*>*> delegatesOut);

    static Result<bool> GetProperty(
            AllocationContext acx,
            Handle<Wobject*> obj,
            Handle<String*> name,
            MutHandle<PropertyDescriptor> result);

    static OkResult DefineProperty(
            AllocationContext acx,
            Handle<Wobject*> obj,
            Handle<String*> name,
            Handle<PropertyDescriptor> defn);

    static Result<bool> LookupProperty(
            AllocationContext acx,
            Handle<Wobject*> obj,
            Handle<String*> name,
            MutHandle<LookupState*> stateOut,
            MutHandle<PropertyDescriptor> defnOut);

    static bool IsWobjectFormat(HeapFormat format) {
        switch (format) {
#define CASE_WOBJECT_TYPE_(wobjtype) \
          case HeapFormat::wobjtype:
    WHISPER_DEFN_WOBJECT_KINDS(CASE_WOBJECT_TYPE_)
#undef CASE_WOBJECT_TYPE_
            return true;
          default:
            return false;
        }
    }
    static bool IsWobject(HeapThing* heapThing) {
        return IsWobjectFormat(heapThing->format());
    }
};


} // namespace VM
} // namespace Whisper


#endif // WHISPER__VM__WOBJECT_HPP
