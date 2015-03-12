#ifndef WHISPER__VM__WOBJECT_HPP
#define WHISPER__VM__WOBJECT_HPP

#include "vm/core.hpp"
#include "vm/box.hpp"
#include "vm/shype/shype.hpp"

/**
 * A Wobject is the base type for all objects visible to the runtime.
 * Every Wobject refers to a Shype to specify its behaviour.
 */

namespace Whisper {
namespace VM {


class Wobject
{
  friend class GC::TraceTraits<Wobject>;
  private:
    HeapField<Shype *> shype_;

  public:
    Wobject(Shype *shype)
      : shype_(shype)
    {
        WH_ASSERT(shype != nullptr);
    }

    Shype *shype() const {
        return shype_;
    }
};


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
            wobj.shype_.scan(scanner, start, end);
        }

        template <typename Updater>
        static void Update(Updater &updater, VM::Wobject &wobj,
                           const void *start, const void *end)
        {
            wobj.shype_.update(updater, start, end);
        }
    };

} // namespace GC
} // namespace Whisper


#endif // WHISPER__VM__WOBJECT_HPP
