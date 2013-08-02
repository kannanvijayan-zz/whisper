#ifndef WHISPER__VM__OBJECT_HPP
#define WHISPER__VM__OBJECT_HPP

#include "common.hpp"
#include "debug.hpp"
#include "value.hpp"
#include "rooting.hpp"
#include "vm/heap_thing.hpp"
#include "vm/shape_tree.hpp"
#include "vm/tuple.hpp"

namespace Whisper {
namespace VM {

//
// A regular javascript Object.
//
//      +---------------+
//      | header        |
//      +---------------+
//      | shape         |
//      | prototype     |
//      | dynamicSlots  |
//      | slotValue0    |
//      | ...           |
//      | slotValueN    |
//      +---------------+
//
// The header flags store the following info:
//      PreventExtensions
//
class Object : public ShapedHeapThing,
               public TypedHeapThing<HeapType::Object>
{
  public:
    enum Flags : uint8_t {
        PreventExtensions = 0x1
    };

    // Shape, Prototype, and DynamicSlots - 3 internal slots.
    static constexpr unsigned FixedSlotsStart = 3;
    
  private:
    NullableHeapThingValue<Object> prototype_;
    NullableHeapThingValue<Tuple> dynamicSlots_;

  public:
    Object(Shape *shape, Object *prototype);

    Object *prototype() const;

    bool hasDynamicSlots() const;
    Tuple *dynamicSlots() const;

    uint32_t numFixedSlots() const;
    uint32_t numDynamicSlots() const;
    uint32_t numSlots() const;

    const Value &fixedSlotValue(uint32_t idx) const;
    const Value &dynamicSlotValue(uint32_t idx) const;
    const Value &slotValue(uint32_t idx) const;
};

typedef HeapThingWrapper<Object> WrappedObject;


} // namespace VM
} // namespace Whisper

#endif // WHISPER__VM__OBJECT_HPP
