#ifndef WHISPER__VM__PLAIN_OBJECT_HPP
#define WHISPER__VM__PLAIN_OBJECT_HPP

#include "vm/core.hpp"
#include "vm/box.hpp"
#include "vm/wobject.hpp"
#include "vm/property_dict.hpp"

namespace Whisper {
namespace VM {


class PlainObject : public Wobject
{
  friend class GC::TraceTraits<PlainObject>;
  private:
    HeapField<PropertyDict *> dict_;

  public:
    PlainObject(PropertyDict *dict)
      : Wobject(),
        dict_(dict)
    {}

    static bool LookupPropertyIndex(Handle<PlainObject *> obj,
                                    Handle<PropertyName> name,
                                    uint32_t *indexOut);

    static bool LookupProperty(Handle<PlainObject *> obj,
                               Handle<PropertyName> name,
                               MutHandle<PropertyDescriptor> result);

    static bool DefineProperty(RunContext *cx,
                               Handle<PlainObject *> obj,
                               Handle<PropertyName> name,
                               Handle<PropertyDescriptor> defn);
};


} // namespace VM
} // namespace Whisper


namespace Whisper {
namespace GC {

    template <>
    struct HeapTraits<VM::PlainObject>
    {
        HeapTraits() = delete;

        static constexpr bool Specialized = true;
        static constexpr AllocFormat Format = AllocFormat::PlainObject;
        static constexpr bool VarSized = false;
    };

    template <>
    struct AllocFormatTraits<AllocFormat::PlainObject>
    {
        AllocFormatTraits() = delete;
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
            TraceTraits<VM::Wobject>::Scan(scanner, obj, start, end);
            obj.dict_.scan(scanner, start, end);
        }

        template <typename Updater>
        static void Update(Updater &updater, VM::PlainObject &obj,
                           const void *start, const void *end)
        {
            TraceTraits<VM::Wobject>::Update(updater, obj, start, end);
            obj.dict_.update(updater, start, end);
        }
    };

} // namespace GC
} // namespace Whisper


#endif // WHISPER__VM__PLAIN_OBJECT_HPP
