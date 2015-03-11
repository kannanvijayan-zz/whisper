#ifndef WHISPER__VM__STRING_HPP
#define WHISPER__VM__STRING_HPP


#include "vm/core.hpp"

#include <new>
#include <cstring>

namespace Whisper {
namespace VM {

//
// A UTF-8 String.
//
class String
{
  public:
    class Cursor
    {
      friend class String;
      private:
        uint32_t offset_;
        Cursor(uint32_t offset) : offset_(offset) {};

        void incrementOffset(uint32_t inc) {
            offset_ += inc;
        }

      public:
        uint32_t offset() const {
            return offset_;
        }

        bool operator <(const Cursor &other) const {
            return offset_ < other.offset_;
        }
        bool operator <=(const Cursor &other) const {
            return offset_ <= other.offset_;
        }
        bool operator >(const Cursor &other) const {
            return offset_ > other.offset_;
        }
        bool operator >=(const Cursor &other) const {
            return offset_ >= other.offset_;
        }
        bool operator ==(const Cursor &other) const {
            return offset_ == other.offset_;
        }
        bool operator !=(const Cursor &other) const {
            return offset_ != other.offset_;
        }
    };

  private:
    // Length of the string in unicode codepoints.
    uint32_t length_;

    // The character data suffixes the structure.
    uint8_t data_[0];

  public:
    String(uint32_t byteLength, const uint8_t *data);

    static String *Create(AllocationContext acx,
                          uint32_t byteLength, const uint8_t *data);
    static String *Create(AllocationContext acx,
                          uint32_t byteLength, const char *data);
    static String *Create(AllocationContext acx, const char *data);
    static String *Create(AllocationContext acx, const String *other);

    static uint32_t CalculateSize(uint32_t byteLength) {
        return sizeof(String) + byteLength;
    }

    uint32_t length() const {
        return length_;
    }

    inline uint32_t byteLength() const
    {
        uint32_t size = GC::AllocThing::From(this)->size();
        WH_ASSERT(size >= sizeof(VM::String));
        return size - sizeof(VM::String);
    }

    const uint8_t *bytes() const {
        return data_;
    }

    bool equals(const String *other) const {
        return (byteLength() == other->byteLength()) &&
               (memcmp(bytes(), other->bytes(), byteLength()) == 0);
    }
    bool equals(const char *str, uint32_t length) const {
        return (byteLength() == length) &&
               (memcmp(bytes(), str, length) == 0);
    }

    Cursor begin() const {
        return Cursor(0);
    }
    Cursor end() const {
        return Cursor(byteLength());
    }

    void advance(Cursor &cursor) const;
    unic_t read(const Cursor &cursor) const;
    unic_t readAdvance(Cursor &cursor) const;
};


} // namespace VM
} // namespace Whisper


namespace Whisper {
namespace GC {

    template <>
    struct HeapTraits<VM::String>
    {
        HeapTraits() = delete;

        static constexpr bool Specialized = true;
        static constexpr AllocFormat Format = AllocFormat::String;
        static constexpr bool VarSized = true;
    };

    template <>
    struct AllocFormatTraits<AllocFormat::String>
    {
        AllocFormatTraits() = delete;
        typedef VM::String Type;
    };

    template <>
    struct TraceTraits<VM::String> : public UntracedTraceTraits<VM::String>
    {};

} // namespace GC
} // namespace Whisper


#endif // WHISPER__VM__STRING_HPP
