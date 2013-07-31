#ifndef WHISPER__VM__TUPLE_HPP
#define WHISPER__VM__TUPLE_HPP

#include "common.hpp"
#include "debug.hpp"
#include "value.hpp"
#include "vm/heap_thing.hpp"

namespace Whisper {
namespace VM {

//
// A Tuple is a useful helper that's used to hold a bunch of
// different values.
//
class Tuple : public HeapThing<HeapType::Tuple>
{
  public:
    Tuple();
    Tuple(const Tuple &other);

    inline const Value &operator [](uint32_t idx) const;
    inline Value &operator [](uint32_t idx);

    inline uint32_t size() const;
};


} // namespace VM
} // namespace Whisper

#endif // WHISPER__VM__TUPLE_HPP
