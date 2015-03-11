#ifndef WHISPER__VM__SHYPE__SHYPE_HPP
#define WHISPER__VM__SHYPE__SHYPE_HPP

#include "vm/core.hpp"
#include "vm/properties.hpp"

/**
 * A shype is an internal shape/type held by all whisper objects.
 */

namespace Whisper {
namespace VM {

class Wobject;

class Shype
{
  protected:
    Shype() {}

  public:
    bool lookupProperty(RunContext *cx,
                        Wobject *obj,
                        const PropertyName &name,
                        PropertyDescriptor *resultOut);

    bool defineProperty(RunContext *cx,
                        Wobject *obj,
                        const PropertyName &name,
                        const PropertyDescriptor &defn);
};


} // namespace VM
} // namespace Whisper


namespace Whisper {
namespace GC {

    template <>
    struct AllocThingTraits<VM::Shype>
    {
        AllocThingTraits() = delete;
        static constexpr bool Specialized = true;
    };

} // namespace GC
} // namespace Whisper


#endif // WHISPER__VM__SHYPE__SHYPE_HPP
