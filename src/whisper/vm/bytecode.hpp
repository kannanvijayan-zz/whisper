#ifndef WHISPER__VM__BYTECODE_HPP
#define WHISPER__VM__BYTECODE_HPP

#include "common.hpp"
#include "debug.hpp"
#include "vm/heap_type_defn.hpp"
#include "vm/heap_thing.hpp"
#include "rooting.hpp"

namespace Whisper {
namespace VM {


//
// Bytecode objects store the raw interpreter bytecode for scripts.
//
struct Bytecode : public HeapThing, public TypedHeapThing<HeapType::Bytecode>
{
  public:
    Bytecode();

    const uint8_t *data() const;
    const uint8_t *dataAt(uint32_t pcOffset) const;
    const uint8_t *dataEnd() const;
    uint8_t *writableData();

    uint32_t length() const;
};

typedef HeapThingWrapper<Bytecode> WrappedBytecode;
typedef Root<Bytecode *> RootedBytecode;
typedef Handle<Bytecode *> HandleBytecode;
typedef MutableHandle<Bytecode *> MutHandleBytecode;

void SpewBytecodeObject(Bytecode *bc);


} // namespace VM
} // namespace Whisper

#endif // WHISPER__VM__BYTECODE_HPP
