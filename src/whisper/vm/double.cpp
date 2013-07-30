
#include "vm/double.hpp"
#include "vm/heap_thing_inlines.hpp"

namespace Whisper {
namespace VM {


HeapDouble::HeapDouble(double val) : value_(val) {}

double
HeapDouble::value() const
{
    return value_;
}


} // namespace VM
} // namespace Whisper
