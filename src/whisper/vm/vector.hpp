#ifndef WHISPER__VM__VECTOR_HPP
#define WHISPER__VM__VECTOR_HPP


#include "common.hpp"
#include "debug.hpp"
#include "slab.hpp"
#include "runtime.hpp"
#include "array.hpp"

#include <new>

namespace Whisper {

namespace VM {
    template <typename T> class Vector;
};

template <typename T>
struct SlabThingTraits<VM::Vector<T>>
  : public SlabThingTraitsHelper<VM::Vector<T>>
{};

template <typename T>
struct AllocationTypeTagTrait<VM::Vector<T>>
  : public AllocationTypeTagTraitHelper<VM::Vector<T>, SlabAllocType::Vector>
{};

// Vectors are traced by default, but array specializations for primitive types
// are not traced.
template <typename T>
struct AllocationTracedTrait<VM::Vector<T>>
{
    static constexpr bool TRACED = true;
};


namespace VM {


//
// A slab-allocated variable-length vector
//
template <typename T>
class Vector
{
  private:
    Heap<Array *> array_;

  public:
    inline Vector()
      : array_(nullptr)
    {}

    inline uint32_t size() const {
        return array_.size();
    }
};


} // namespace VM
} // namespace Whisper


#endif // WHISPER__VM__VECTOR_HPP
