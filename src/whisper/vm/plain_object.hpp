#ifndef WHISPER__VM__PLAIN_OBJECT_HPP
#define WHISPER__VM__PLAIN_OBJECT_HPP

#include "vm/core.hpp"
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
    PlainObject(Array<Wobject *> *delegates, PropertyDict *dict)
      : Wobject(),
        delegates_(delegates),
        dict_(dict)
    {
        WH_ASSERT(delegates_ != nullptr);
        WH_ASSERT(dict_ != nullptr);
    }

    static PlainObject *Create(AllocationContext acx,
                               Handle<Array<Wobject *> *> delegates);

    static bool GetDelegates(ThreadContext *cx,
                             Handle<PlainObject *> obj,
                             MutHandle<Array<Wobject *> *> delegatesOut);

    static bool LookupPropertyIndex(Handle<PlainObject *> obj,
                                    Handle<PropertyName> name,
                                    uint32_t *indexOut);

    static bool LookupProperty(ThreadContext *cx,
                               Handle<PlainObject *> obj,
                               Handle<PropertyName> name,
                               MutHandle<PropertyDescriptor> result);

    static bool DefineProperty(ThreadContext *cx,
                               Handle<PlainObject *> obj,
                               Handle<PropertyName> name,
                               Handle<PropertyDescriptor> defn);
};


} // namespace VM


//
// GC Specializations
//

template <>
struct HeapTraits<VM::PlainObject>
{
    HeapTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr HeapFormat Format = HeapFormat::PlainObject;
    static constexpr bool VarSized = false;
};

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
