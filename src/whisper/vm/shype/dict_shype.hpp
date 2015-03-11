#ifndef WHISPER__VM__SHYPE__DICT_SHYPE_HPP
#define WHISPER__VM__SHYPE__DICT_SHYPE_HPP

#include "vm/core.hpp"
#include "vm/string.hpp"
#include "vm/shype/shype.hpp"

/**
 * The shype used by global objects.
 */

namespace Whisper {
namespace VM {


class DictShype : public Shype
{
  friend class GC::TraceTraits<DictShype>;
  public:
    DictShype()
      : Shype()
    {}

    bool lookupDictProperty(RunContext *cx,
                            Handle<Wobject *> obj,
                            Handle<PropertyName> name,
                            MutHandle<PropertyDescriptor> result);

    bool defineDictProperty(RunContext *cx,
                            Handle<Wobject *> obj,
                            Handle<PropertyName> name,
                            Handle<PropertyDescriptor> defn);
};


} // namespace VM
} // namespace Whisper


namespace Whisper {
namespace GC {


    template <>
    struct HeapTraits<VM::DictShype>
    {
        HeapTraits() = delete;

        static constexpr bool Specialized = true;
        static constexpr AllocFormat Format = AllocFormat::DictShype;
        static constexpr bool VarSized = false;
    };

    template <>
    struct AllocFormatTraits<AllocFormat::DictShype>
    {
        AllocFormatTraits() = delete;
        typedef VM::DictShype Type;
    };

    template <>
    struct TraceTraits<VM::DictShype>
    {
        TraceTraits() = delete;

        static constexpr bool Specialized = true;
        static constexpr bool IsLeaf = false;

        template <typename Scanner>
        static void Scan(Scanner &scanner, const VM::DictShype &shype,
                         const void *start, const void *end)
        {}

        template <typename Updater>
        static void Update(Updater &updater, VM::DictShype &shype,
                           const void *start, const void *end)
        {}
    };

} // namespace GC
} // namespace Whisper


#endif // WHISPER__VM__SHYPE__DICT_SHYPE_HPP
