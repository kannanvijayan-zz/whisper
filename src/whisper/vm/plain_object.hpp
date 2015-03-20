#ifndef WHISPER__VM__PLAIN_OBJECT_HPP
#define WHISPER__VM__PLAIN_OBJECT_HPP

#include "vm/core.hpp"
#include "vm/predeclare.hpp"
#include "vm/box.hpp"
#include "vm/wobject.hpp"
#include "vm/property_dict.hpp"
#include "vm/array.hpp"

namespace Whisper {
namespace VM {


class PlainObject : public Wobject
{
  friend class TraceTraits<PlainObject>;
  private:
    HeapField<Array<Wobject *> *> delegates_;
    HeapField<PropertyDict *> dict_;

    // Initial dictionary size is 8.
    static constexpr uint32_t InitialPropertyCapacity = 8;

  public:
    PlainObject(Handle<Array<Wobject *> *> delegates,
                Handle<PropertyDict *> dict)
      : Wobject(),
        delegates_(delegates),
        dict_(dict)
    {
        WH_ASSERT(delegates_ != nullptr);
        WH_ASSERT(dict_ != nullptr);
    }

    static Result<PlainObject *> Create(AllocationContext acx,
                                        Handle<Array<Wobject *> *> delegates);

    static void GetDelegates(ThreadContext *cx,
                             Handle<PlainObject *> obj,
                             MutHandle<Array<Wobject *> *> delegatesOut);

    static bool LookupPropertyIndex(Handle<PlainObject *> obj,
                                    Handle<String *> name,
                                    uint32_t *indexOut);

    static bool LookupProperty(ThreadContext *cx,
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
        obj.delegates_.scan(scanner, start, end);
        obj.dict_.scan(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater &updater, VM::PlainObject &obj,
                       const void *start, const void *end)
    {
        obj.delegates_.update(updater, start, end);
        obj.dict_.update(updater, start, end);
    }
};


} // namespace Whisper


#endif // WHISPER__VM__PLAIN_OBJECT_HPP
