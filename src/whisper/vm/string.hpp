#ifndef WHISPER__VM__STRING_HPP
#define WHISPER__VM__STRING_HPP


#include "common.hpp"
#include "debug.hpp"
#include "slab.hpp"
#include "runtime.hpp"
#include "vm/hash_table.hpp"

#include <new>
#include <utility>
#include <cstring>

namespace Whisper {

namespace VM {
    // Base class for strings.
    class String;

    // Simple fix-length inline-allocated string.
    class FlatString;
};

// Specialize String and FlatString for SlabThingTraits
template <>
struct SlabThingTraits<VM::String>
{
    SlabThingTraits() = delete;
    SlabThingTraits(const SlabThingTraits &other) = delete;

    static constexpr bool SPECIALIZED = true;
};

template <>
struct SlabThingTraits<VM::FlatString>
{
    SlabThingTraits() = delete;
    SlabThingTraits(const SlabThingTraits &other) = delete;

    static constexpr bool SPECIALIZED = true;
};

// Specialize FlatString for AllocationTraits.
// String doesn't get AllocationTraits specialization because it's
// never instantiated directly.
template <>
struct AllocationTraits<VM::FlatString>
{
    static constexpr SlabAllocType ALLOC_TYPE = SlabAllocType::FlatString;
    static constexpr bool TRACED = false;
};


namespace VM {

//
// A slab-allocated fixed-length array
//
class String
{
  protected:
    String() {}

  public:
    bool isFlatString() const {
        return SlabThing::From(this)->isFlatString();
    }
    const FlatString *toFlatString() const {
        WH_ASSERT(isFlatString());
        return reinterpret_cast<const FlatString *>(this);
    }

    inline uint32_t bytes() const;
};

//
// A slab-allocated fixed-length array
//
class FlatString : public String
{
  private:
    uint8_t data_[0];

  public:
    static uint32_t CalculateSize(uint32_t bytes) {
        return sizeof(FlatString) + bytes;
    }

    inline FlatString(const uint8_t *data) : String() {
        uint32_t len = bytes();
        for (uint32_t i = 0; i < len; i++)
            data_[i] = data[i];
    }

    static FlatString *Create(AllocationContext &cx,
                              const uint8_t *data, uint32_t bytes)
    {
        return cx.createSized<FlatString>(CalculateSize(bytes), data);
    }

    inline uint32_t bytes() const {
        uint32_t size = SlabThing::From(this)->allocSize();
        return size - sizeof(FlatString);
    }

    inline const uint8_t *data() const {
        return &(data_[0]);
    }
};


} // namespace VM
} // namespace Whisper


#endif // WHISPER__VM__STRING_HPP
