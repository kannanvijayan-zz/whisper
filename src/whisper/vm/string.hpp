#ifndef WHISPER__VM__STRING_HPP
#define WHISPER__VM__STRING_HPP


#include "common.hpp"
#include "debug.hpp"
#include "slab.hpp"
#include "runtime.hpp"

#include <new>

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

    inline uint32_t length() const;
    inline uint32_t charAt(uint32_t idx) const;
};

//
// A slab-allocated fixed-length array
//
class FlatString : public String
{
  public:
    static constexpr uint8_t FLAG_16BIT = 0x01;

  private:
    uint16_t data_[0];

    inline uint16_t *data16() {
        return &(data_[0]);
    }
    inline const uint16_t *data16() const {
        return &(data_[0]);
    }
    inline uint16_t &data16(uint32_t index) {
        return data16()[index];
    }

    inline uint8_t *data8() {
        return reinterpret_cast<uint8_t *>(&(data_[0]));
    }
    inline const uint8_t *data8() const {
        return reinterpret_cast<const uint8_t *>(&(data_[0]));
    }
    inline uint8_t &data8(uint32_t index) {
        return data8()[index];
    }

  public:
    static uint32_t CalculateSize8(uint32_t length) {
        return sizeof(FlatString) + length;
    }
    static uint32_t CalculateSize16(uint32_t length) {
        return sizeof(FlatString) + (length * 2);
    }

    inline FlatString(const uint8_t *data) : String() {
        WH_ASSERT(is8Bit());
        uint32_t len = length();
        for (uint32_t i = 0; i < len; i++)
            data8(i) = data[i];
    }

    inline FlatString(const uint16_t *data) : String() {
        WH_ASSERT(is16Bit());
        uint32_t len = length();
        for (uint32_t i = 0; i < len; i++)
            data16(i) = data[i];
    }

    static FlatString *Create(AllocationContext &cx,
                              const uint8_t *data, uint32_t length)
    {
        return cx.createSized<FlatString>(CalculateSize8(length), data);
    }

    static FlatString *Create(AllocationContext &cx,
                              const uint16_t *data, uint32_t length)
    {
        return cx.createSizedFlagged<FlatString>(CalculateSize16(length),
                                                 FLAG_16BIT, data);
    }

    inline uint32_t length() const {
        uint32_t size = SlabThing::From(this)->allocSize();
        if (is16Bit()) {
            WH_ASSERT((size & 1) == 0);
            size /= 2;
        }
        return size;
    }

    inline bool is16Bit() const {
        return SlabThing::From(this)->flags() & FLAG_16BIT;
    }
    inline bool is8Bit() const {
        return !is16Bit();
    }

    inline const uint8_t *data8Bit() const {
        WH_ASSERT(is8Bit());
        return data8();
    }

    inline const uint16_t *data16Bit() const {
        WH_ASSERT(is16Bit());
        return data16();
    }

    inline uint32_t charAt(uint32_t idx) const {
        WH_ASSERT(idx < length());
        if (is16Bit())
            return data16Bit()[idx];
        return data8Bit()[idx];
    }
};


} // namespace VM
} // namespace Whisper


#endif // WHISPER__VM__STRING_HPP
