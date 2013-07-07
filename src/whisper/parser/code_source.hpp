#ifndef WHISPER__PARSER__CODE_SOURCE_HPP
#define WHISPER__PARSER__CODE_SOURCE_HPP

#include <limits>
#include "common.hpp"
#include "debug.hpp"

//
// A code source represents an input source that can product a character
// stream of source code.
//

namespace Whisper {


//
// CodeSource
//
// Represents 
// All methods on this class are pure virtual.
//
// Code sources do not provide general random access, but they
// allow users to mark a location.  At a later point, the stream
// can be rewound to any position at or after the marked location.
//
// A set mark may be erased at a later point with 'erase'.
//
class CodeSource
{
  protected:
    const char *name_;

    const uint8_t *data_;
    const uint8_t *dataEnd_;
    uint32_t dataSize_;

    inline CodeSource(const char *name)
      : name_(name)
    {}

    inline ~CodeSource() {}

  public:
    inline const char *name() const {
        return name_;
    }

    inline const uint8_t *data() const {
        return data_;
    }

    inline uint32_t dataSize() const {
        return dataSize_;
    }

    inline const uint8_t *dataEnd() const {
        return dataEnd_;
    }
};

//
// FileCodeSource
//
// Code source that reads from a file.
//
class FileCodeSource : public CodeSource
{
  private:
    int fd_ = -1;

  public:
    inline FileCodeSource(const char *filename) : CodeSource(filename) {}

    inline ~FileCodeSource() {
        finalize();
    }

    void finalize();

  public:
    bool initialize();
};

//
// SourceStream
//
// A stream API built on top of a CodeStream to allow byte-by-byte
// access.  The read interface is buffered via a direct, stack-allocated
// buffer to avoid virtual-function-call overhead for readByte()
//
class SourceStream
{
  private:
    CodeSource &source_;
    const uint8_t *cursor_;

  public:
    inline SourceStream(CodeSource &source)
        : source_(source),
          cursor_(source_.data())
    {}

    inline CodeSource &source() const {
        return source_;
    }

    inline const uint8_t *cursor() const {
        return cursor_;
    }

    inline uint32_t positionOf(const uint8_t *ptr) const {
        WH_ASSERT(ptr >= source_.data() && ptr <= source_.dataEnd());
        return ptr - source_.data();
    }
    inline uint32_t position() const {
        return positionOf(cursor_);
    }

    inline bool atEnd() const {
        return cursor_ == source_.dataEnd();
    }

    inline uint8_t readByte() {
        // readByte() should never be called on an EOLed stream.
        WH_ASSERT(!atEnd());
        return *cursor_++;
    }

    inline void rewindTo(uint32_t pos) {
        WH_ASSERT(pos <= position());
        cursor_ = source_.data() + pos;
    }

    inline void advanceTo(uint32_t pos) {
        WH_ASSERT(pos >= position());
        cursor_ = source_.data() + pos;
    }

    inline void rewindBy(uint32_t count) {
        WH_ASSERT(count <= position());
        cursor_ -= count;
    }
};


} // namespace Whisper

#endif // WHISPER__PARSER__CODE_SOURCE_HPP
