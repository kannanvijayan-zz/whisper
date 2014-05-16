#ifndef WHISPER__VM__STRING_HPP
#define WHISPER__VM__STRING_HPP


#include "common.hpp"
#include "debug.hpp"
#include "spew.hpp"
#include "slab.hpp"
#include "runtime.hpp"
#include "gc.hpp"

#include <new>

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
    String(uint32_t byteLength, const char *data);
    String(const char *data);
    String(const String &other);

    uint32_t length() const {
        return length_;
    }

    inline uint32_t byteLength() const;

    const uint8_t *bytes() const {
        return data_;
    }

    Cursor begin() const {
        return Cursor(0);
    }
    Cursor end() const {
        return Cursor(length());
    }

    void advance(Cursor &cursor) const;
    unic_t read(const Cursor &cursor) const;
    unic_t readAdvance(Cursor &cursor) const;
};


} // namespace VM
} // namespace Whisper

// Include array template specializations for describing strings to
// the GC.
#include "vm/string_specializations.hpp"


namespace Whisper {
namespace VM {


inline uint32_t
String::byteLength() const
{
    uint32_t size = GC::AllocThing::From(this)->size();
    WH_ASSERT(size >= sizeof(VM::String));
    return size - sizeof(VM::String);
}


} // namespace VM
} // namespace Whisper


#endif // WHISPER__VM__STRING_HPP
