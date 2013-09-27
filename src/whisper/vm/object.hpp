#ifndef WHISPER__VM__OBJECT_HPP
#define WHISPER__VM__OBJECT_HPP

#include "common.hpp"
#include "debug.hpp"
#include "value.hpp"
#include "rooting.hpp"
#include "vm/property_map_thing.hpp"

namespace Whisper {
namespace VM {


class ObjectImpl : public PropertyMapThing
{
  public:
    ObjectImpl(Shape *shape);
};

template <>
struct PropertyMapTypeTraits<HeapType::Object>
{
    static constexpr uint32_t NumInternalSlots = 0;
};

class Object : public ObjectImpl, public TypedHeapThing<HeapType::Object>
{
  public:
    Object(Shape *shape);
};

typedef HeapThingWrapper<Object> WrappedObject;


} // namespace VM
} // namespace Whisper

#endif // WHISPER__VM__OBJECT_HPP
