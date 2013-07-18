#ifndef WHISPER__VM__DOUBLE_HPP
#define WHISPER__VM__DOUBLE_HPP

#include "common.hpp"
#include "debug.hpp"
#include "vm/heap_type_defn.hpp"
#include "vm/heap_thing.hpp"

#include <limits>

namespace Whisper {
namespace VM {


//
// Double objects are reasonably straightforward.
//
//      +-----------------------+
//      | Header                |
//      +-----------------------+
//      | Value                 |
//      +-----------------------+
//
struct HeapDouble : public HeapThingPayload
{
  private:
    double value_;

  public:
    HeapDouble(double val) : value_(val) {}

    inline double value() const {
        return value_;
    }
};

typedef HeapThingWrapper<HeapDouble, HeapType::HeapDouble> WrappedHeapDouble;


} // namespace VM
} // namespace Whisper

#endif // WHISPER__VM__DOUBLE_HPP
