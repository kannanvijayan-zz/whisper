#ifndef WHISPER__PARSER__CODE_SOURCE_HPP
#define WHISPER__PARSER__CODE_SOURCE_HPP

#include <fstream>
#include <algorithm>
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
// Abstract base class representing the code source interface.
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
    inline CodeSource() {}
    inline ~CodeSource() {}

  public:
    // The name of the source.
    virtual const uint8_t *name() = 0;

    // The current position within the source.
    virtual size_t position() = 0;

    // Set and erase marks.
    virtual void mark() = 0;
    virtual void erase() = 0;

    // Read some data from the source.
    virtual bool read(uint8_t *buf, size_t size, size_t *bytesRead) = 0;

    // Rewind to an earlier position.
    virtual bool rewind(size_t toPos) = 0;
};

//
// FileCodeSource
//
// Code source that reads from a file.
//
class FileCodeSource : public CodeSource
{
  private:
    const char *filename_;
    int fd_ = -1;
    const uint8_t *data_ = nullptr;
    size_t size_ = 0;
    size_t pos_ = 0;
#if defined(ENABLE_DEBUG)
    size_t mark_ = 0;
    bool marked_ = false;
#endif

  public:
    FileCodeSource(const char *filename);
    ~FileCodeSource();

    void finalize();

  public:
    bool initialize();

    const uint8_t *name() override;

    size_t position() override;

    void mark() override;
    void erase() override;

    bool read(uint8_t *buf, size_t size, size_t *bytesRead) override;

    bool rewind(size_t toPos) override;
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
  public:
    static constexpr int End = std::numeric_limits<int>::max();
    static constexpr int Error = -1;

  private:
    CodeSource &source_;

    static constexpr unsigned BufferSize = 1024;
    uint8_t buffer_[BufferSize];

    uint8_t *bufferEnd_ = nullptr;
    uint8_t *bufferCur_ = nullptr;

    size_t bufferStartPos_ = 0;

    unsigned markDepth_ = 0;

    bool atEnd_ = false;

  public:
    inline SourceStream(CodeSource &source) : source_(source) {}

    bool initialize();

    inline size_t position() const {
        return bufferStartPos_ + (bufferCur_ - &buffer_[0]);
    }

    inline bool atEnd() const {
        return atEnd_;
    }

    inline void mark() {
        if (markDepth_ == 0)
            source_.mark();
        markDepth_++;
    }

    inline void erase() {
        markDepth_--;
        if (markDepth_ == 0)
            source_.erase();
    }

    inline int readByte() {
        // readByte() should never be called on an EOLed stream.
        WH_ASSERT(!atEnd_);
        WH_ASSERT(bufferCur_ <= bufferEnd_);

        if (bufferCur_ < bufferEnd_)
            return *bufferCur_++;

        return readByteSlow();
    }

    inline bool rewind(size_t pos) {
        WH_ASSERT(!atEnd_);
        WH_ASSERT(bufferCur_ <= bufferEnd_);
        WH_ASSERT(pos <= position());

        if (pos >= bufferStartPos_) {
            bufferCur_ = buffer_ + (pos - bufferStartPos_);
            return true;
        }

        return rewindSlow(pos);
    }

  private:
    int readByteSlow();
    bool rewindSlow(size_t pos);
    bool advance();
};


} // namespace Whisper

#endif // WHISPER__PARSER__CODE_SOURCE_HPP
