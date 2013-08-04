#ifndef WHISPER__VM__BYTECODE_HPP
#define WHISPER__VM__BYTECODE_HPP

#include "common.hpp"
#include "debug.hpp"
#include "vm/heap_type_defn.hpp"
#include "vm/heap_thing.hpp"

namespace Whisper {
namespace VM {


//
// Bytecode objects store the raw interpreter bytecode for scripts.
//
struct Bytecode : public HeapThing, public TypedHeapThing<HeapType::Bytecode>
{
  protected:
    uint8_t *writableData();
    
  public:
    Bytecode(const uint8_t *data);

    const uint8_t *data() const;

    uint32_t length() const;
};

typedef HeapThingWrapper<Bytecode> WrappedBytecode;


} // namespace VM
} // namespace Whisper

#endif // WHISPER__VM__BYTECODE_HPP
