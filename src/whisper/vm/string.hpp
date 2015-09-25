#ifndef WHISPER__VM__STRING_HPP
#define WHISPER__VM__STRING_HPP


#include "vm/core.hpp"
#include "vm/predeclare.hpp"

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

        bool operator <(Cursor const& other) const {
            return offset_ < other.offset_;
        }
        bool operator <=(Cursor const& other) const {
            return offset_ <= other.offset_;
        }
        bool operator >(Cursor const& other) const {
            return offset_ > other.offset_;
        }
        bool operator >=(Cursor const& other) const {
            return offset_ >= other.offset_;
        }
        bool operator ==(Cursor const& other) const {
            return offset_ == other.offset_;
        }
        bool operator !=(Cursor const& other) const {
            return offset_ != other.offset_;
        }
    };

  private:
    // Length of the string in unicode codepoints.
    uint32_t length_;

    // The character data suffixes the structure.
    uint8_t data_[0];

  public:
    String(uint32_t byteLength, uint8_t const* data);

    static Result<String*> Create(AllocationContext acx,
                                   uint32_t byteLength,
                                   uint8_t const* data);
    static Result<String*> Create(AllocationContext acx,
                                   uint32_t byteLength,
                                   char const* data);
    static Result<String*> Create(AllocationContext acx,
                                   char const* data);
    static Result<String*> Create(AllocationContext acx,
                                   String const* other);

    static uint32_t CalculateSize(uint32_t byteLength) {
        // Always add 1 byte to the byteLength to allow for a terminating
        // null char.
        return sizeof(String) + byteLength + 1;
    }
    static uint32_t CalculateByteLength(uint32_t size) {
        // Always add 1 byte to the byteLength to allow for a terminating
        // null char.
        return size - (sizeof(String) + 1);
    }

    uint32_t length() const {
        return length_;
    }

    inline uint32_t byteLength() const
    {
        uint32_t size = HeapThing::From(this)->size();
        WH_ASSERT(size >= sizeof(VM::String));
        return CalculateByteLength(size);
    }

    uint8_t const* bytes() const {
        return data_;
    }
    char const* c_chars() const {
        return reinterpret_cast<char const*>(bytes());
    }

    bool equals(String const* other) const;
    bool equals(char const* str, uint32_t length) const;

    Cursor begin() const {
        return Cursor(0);
    }
    Cursor end() const {
        return Cursor(byteLength());
    }

    void advance(Cursor& cursor) const;
    unic_t read(Cursor const& cursor) const;
    unic_t readAdvance(Cursor& cursor) const;

    uint32_t fnvHash() const;
};


} // namespace VM


//
// GC Specializations
//

template <>
struct TraceTraits<VM::String> : public UntracedTraceTraits<VM::String>
{};


} // namespace Whisper


#endif // WHISPER__VM__STRING_HPP
