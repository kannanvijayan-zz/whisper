#ifndef WHISPER__VM__TUPLE_HPP
#define WHISPER__VM__TUPLE_HPP

#include "common.hpp"
#include "debug.hpp"
#include "value.hpp"
#include "rooting.hpp"
#include "vm/heap_thing.hpp"

namespace Whisper {
namespace VM {

//
// A Tuple is a useful helper that's used to hold a bunch of
// different values.
//
class Tuple : public HeapThing, public TypedHeapThing<HeapType::Tuple>
{
  public:
    Tuple();
    Tuple(const Tuple &other);

    uint32_t size() const;

    Handle<Value> get(uint32_t idx) const;
    Handle<Value> operator [](uint32_t idx) const;
    void set(uint32_t idx, const Value &val);

  private:
    const Heap<Value> &element(uint32_t idx) const;
    Heap<Value> &element(uint32_t idx);
};


} // namespace VM
} // namespace Whisper

#endif // WHISPER__VM__TUPLE_HPP
