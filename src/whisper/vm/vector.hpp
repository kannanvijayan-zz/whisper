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

// Specialize Vector for SlabThingTraits
template <typename T>
struct SlabThingTraits<VM::Vector<T>>
  : public SlabThingTraitsHelper<VM::Vector<T>>
{};

// Specialize Vector for AllocationTraits
template <typename T>
struct AllocationTraits<VM::Vector<T>>
{
    static constexpr bool ALLOC_TYPE = SlabAllocType::Array;
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
