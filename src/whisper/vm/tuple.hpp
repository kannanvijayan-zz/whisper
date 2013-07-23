#ifndef WHISPER__VM__TUPLE_HPP
#define WHISPER__VM__TUPLE_HPP

#include "common.hpp"
#include "debug.hpp"
#include "value.hpp"
#include "rooting.hpp"

namespace Whisper {
namespace VM {

//
// A Tuple is a useful helper that's used to hold a bunch of
// different values.
//
class Tuple : public HeapThing<HeapType::Tuple>
{
  public:
    Tuple() {
        uint32_t vals = objectValueCount();
        for (uint32_t i = 0; i < vals; i++)
            valueRef(i) = UndefinedValue();
    }

    Tuple(const Tuple &other) {
        uint32_t vals = objectValueCount();
        uint32_t otherVals = other.objectValueCount();
        if (vals <= otherVals) {
            for (uint32_t i = 0; i < vals; i++)
                valueRef(i) = other.valueRef(i);
        } else {
            uint32_t i;
            for (i = 0; i < otherVals; i++)
                valueRef(i) = other.valueRef(i);
            for (; i < vals; i++)
                valueRef(i) = UndefinedValue();
        }
    }

    inline const Value &operator [](uint32_t idx) const {
        return valueRef(idx);
    }

    inline Value &operator [](uint32_t idx) {
        return valueRef(idx);
    }

    inline uint32_t size() const {
        return objectValueCount();
    }
};


} // namespace VM
} // namespace Whisper

#endif // WHISPER__VM__TUPLE_HPP
