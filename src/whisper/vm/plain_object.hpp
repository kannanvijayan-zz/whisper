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
    PlainObject(Handle<Array<Wobject *> *> delegates,
                Handle<PropertyDict *> dict)
      : HashObject(delegates, dict)
    {}

    static Result<PlainObject *> Create(AllocationContext acx,
                                        Handle<Array<Wobject *> *> delegates);

    static void GetDelegates(ThreadContext *cx,
                             Handle<PlainObject *> obj,
                             MutHandle<Array<Wobject *> *> delegatesOut);

    static bool GetProperty(ThreadContext *cx,
                            Handle<PlainObject *> obj,
                            Handle<String *> name,
                            MutHandle<PropertyDescriptor> result);

    static OkResult DefineProperty(ThreadContext *cx,
                                   Handle<PlainObject *> obj,
                                   Handle<String *> name,
                                   Handle<PropertyDescriptor> defn);
};


} // namespace VM


//
// GC Specializations
//

template <>
struct HeapFormatTraits<HeapFormat::PlainObject>
{
    HeapFormatTraits() = delete;
    typedef VM::PlainObject Type;
};

template <>
struct TraceTraits<VM::PlainObject>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner &scanner, const VM::PlainObject &obj,
                     const void *start, const void *end)
    {
        TraceTraits<VM::HashObject>::Scan(scanner, obj, start, end);
    }

    template <typename Updater>
    static void Update(Updater &updater, VM::PlainObject &obj,
                       const void *start, const void *end)
    {
        TraceTraits<VM::HashObject>::Update(updater, obj, start, end);
    }
};


} // namespace Whisper


#endif // WHISPER__VM__PLAIN_OBJECT_HPP
