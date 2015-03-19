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
    Wobject() {}

  public:
    static OkResult GetDelegates(ThreadContext *cx,
                                 Handle<Wobject *> obj,
                                 MutHandle<Array<Wobject *> *> delegatesOut);

    static Result<bool> LookupProperty(ThreadContext *cx,
                                       Handle<Wobject *> obj,
                                       Handle<PropertyName> name,
                                       MutHandle<PropertyDescriptor> result);

    static OkResult DefineProperty(ThreadContext *cx,
                                   Handle<Wobject *> obj,
                                   Handle<PropertyName> name,
                                   Handle<PropertyDescriptor> defn);
};


} // namespace VM
} // namespace Whisper


#endif // WHISPER__VM__WOBJECT_HPP
