#ifndef WHISPER__VM__WOBJECT_HPP
#define WHISPER__VM__WOBJECT_HPP

#include "vm/core.hpp"
#include "vm/properties.hpp"
#include "vm/array.hpp"

/**
 * A Wobject is the base type for all objects visible to the runtime.
 */

namespace Whisper {
namespace VM {


class Wobject
{
  friend class GC::TraceTraits<Wobject>;
  private:

  public:
    Wobject() {}

    static bool GetDelegates(RunContext *cx,
                             Handle<Wobject *> obj,
                             MutHandle<Array<Wobject *> *> delegatesOut);

    static bool LookupProperty(RunContext *cx,
                               Handle<Wobject *> obj,
                               Handle<PropertyName> name,
                               MutHandle<PropertyDescriptor> result);

    static bool DefineProperty(RunContext *cx,
                               Handle<Wobject *> obj,
                               Handle<PropertyName> name,
                               Handle<PropertyDescriptor> defn);
};


//
// Procedures to manipulate objects.
//


} // namespace VM
} // namespace Whisper


namespace Whisper {
namespace GC {

    template <>
    struct AllocThingTraits<VM::Wobject>
    {
        AllocThingTraits() = delete;
        static constexpr bool Specialized = true;
    };

    template <>
    struct TraceTraits<VM::Wobject>
    {
        TraceTraits() = delete;

        static constexpr bool Specialized = true;
        static constexpr bool IsLeaf = false;

        template <typename Scanner>
        static void Scan(Scanner &scanner, const VM::Wobject &wobj,
                         const void *start, const void *end)
        {
        }

        template <typename Updater>
        static void Update(Updater &updater, VM::Wobject &wobj,
                           const void *start, const void *end)
        {
        }
    };

} // namespace GC
} // namespace Whisper


#endif // WHISPER__VM__WOBJECT_HPP
