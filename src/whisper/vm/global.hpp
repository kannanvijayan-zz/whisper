#ifndef WHISPER__VM__GLOBAL_HPP
#define WHISPER__VM__GLOBAL_HPP

#include "common.hpp"
#include "debug.hpp"
#include "value.hpp"
#include "rooting.hpp"
#include "vm/property_map_thing.hpp"

namespace Whisper {
namespace VM {

template <>
struct PropertyMapTypeTraits<HeapType::Global>
{
    static constexpr uint32_t NumInternalSlots = 0;
};

class Global : public PropertyMapThing,
               public TypedHeapThing<HeapType::Global>
{
  public:
    Global(Shape *shape, Global *prototype);
};

typedef HeapThingWrapper<Global> WrappedGlobal;


} // namespace VM
} // namespace Whisper

#endif // WHISPER__VM__GLOBAL_HPP
