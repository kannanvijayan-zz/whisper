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


class Wobject
{
  protected:
    Wobject() {
        WH_ASSERT(IsWobject(HeapThing::From(this)));
    }

  public:
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
          case HeapFormat::PlainObject:
          case HeapFormat::CallScope:
          case HeapFormat::BlockScope:
          case HeapFormat::ModuleScope:
          case HeapFormat::GlobalScope:
          case HeapFormat::FunctionObject:
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
