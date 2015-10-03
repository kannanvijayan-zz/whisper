#ifndef WHISPER__VM__PLAIN_OBJECT_HPP
#define WHISPER__VM__PLAIN_OBJECT_HPP

#include "vm/core.hpp"
#include "vm/predeclare.hpp"
#include "vm/hash_object.hpp"

namespace Whisper {
namespace VM {


class PlainObject : public HashObject
{
  friend class TraceTraits<PlainObject>;
  public:
    PlainObject(Handle<Array<Wobject*>*> delegates,
                Handle<PropertyDict*> dict)
      : HashObject(delegates, dict)
    {}

    static Result<PlainObject*> Create(AllocationContext acx,
                                       Handle<Array<Wobject*>*> delegates);

    WobjectHooks const* getPlainObjectHooks() const;

    static uint32_t NumDelegates(AllocationContext acx,
                                 Handle<PlainObject*> obj);

    static void GetDelegates(AllocationContext acx,
                             Handle<PlainObject*> obj,
                             MutHandle<Array<Wobject*>*> delegatesOut);

    static bool GetProperty(AllocationContext acx,
                            Handle<PlainObject*> obj,
                            Handle<String*> name,
                            MutHandle<PropertyDescriptor> result);

    static OkResult DefineProperty(AllocationContext acx,
                                   Handle<PlainObject*> obj,
                                   Handle<String*> name,
                                   Handle<PropertyDescriptor> defn);
};


} // namespace VM


//
// GC Specializations
//

template <>
struct TraceTraits<VM::PlainObject>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner& scanner, VM::PlainObject const& obj,
                     void const* start, void const* end)
    {
        TraceTraits<VM::HashObject>::Scan(scanner, obj, start, end);
    }

    template <typename Updater>
    static void Update(Updater& updater, VM::PlainObject& obj,
                       void const* start, void const* end)
    {
        TraceTraits<VM::HashObject>::Update(updater, obj, start, end);
    }
};


} // namespace Whisper


#endif // WHISPER__VM__PLAIN_OBJECT_HPP
