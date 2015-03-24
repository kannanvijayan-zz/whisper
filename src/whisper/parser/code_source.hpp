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
    CodeSource() {}

  public:
    virtual ~CodeSource() {}

    virtual const char *name() const = 0;
    virtual uint32_t size() const = 0;

    virtual bool read(const uint8_t **dataOut, uint32_t *dataSizeOut) = 0;

    virtual bool hasError() const = 0;
    virtual const char *error() const = 0;
};

//
// FileCodeSource
//
// Code source that reads from a file.
//
class FileCodeSource : public CodeSource
{
  private:
    static constexpr unsigned ErrorMaxLength = 128;

  private:
    const char *filename_;
    int fd_;
    uint32_t size_;
    void *data_;
    bool hasError_;
    char error_[ErrorMaxLength];

  public:
    FileCodeSource(const char *filename)
      : CodeSource(),
        filename_(filename),
        fd_(-1),
        size_(0),
        data_(nullptr),
        hasError_(false)
    {
        if (!initialize()) {
            WH_ASSERT(hasError_);
            finalize();
        }
    }

    virtual ~FileCodeSource() {
        finalize();
    }

    virtual const char *name() const override {
        WH_ASSERT(!hasError_);
        return filename_;
    }
    virtual uint32_t size() const override {
        WH_ASSERT(!hasError_);
        return size_;
    }

    virtual bool read(const uint8_t **dataOut,
                      uint32_t *dataSizeOut)
        override
    {
        WH_ASSERT(!hasError_);
        *dataOut = reinterpret_cast<const uint8_t *>(data_);
        *dataSizeOut = size_;
        return true;
    }

    virtual bool hasError() const override {
        return hasError_;
    }
    virtual const char *error() const override {
        WH_ASSERT(hasError_);
        return error_;
    }

  private:
    bool initialize();
    void finalize();

    void setError(const char *msg);
    void setError(const char *msg, const char *data);
};


//
// SourceReader
//
// A reader API built on top of a CodeSource to allow byte-by-byte
// access.
//
class SourceReader
{
  private:
    CodeSource &source_;
    uint32_t size_;
    const uint8_t *start_;
    const uint8_t *end_;
    const uint8_t *cursor_;
    bool atEndOfInput_;

    const char *error_;

  public:
    SourceReader(CodeSource &source);

    CodeSource &source() const {
        WH_ASSERT(!error_);
        return source_;
    }

    uint32_t bufferSize() const {
        return end_ - start_;
    }

    const uint8_t *cursor() const {
        WH_ASSERT(!error_);
        return cursor_;
    }
    const uint8_t *dataAt(uint32_t posn) const {
        WH_ASSERT(posn <= bufferSize());
        return start_ + posn;
    }

    uint32_t positionOf(const uint8_t *ptr) const {
        WH_ASSERT(!error_);
        WH_ASSERT(ptr >= start_ && ptr <= end_);
        return ptr - start_;
    }
    uint32_t position() const {
        WH_ASSERT(!error_);
        return positionOf(cursor_);
    }

    bool atEnd() const {
        WH_ASSERT(cursor_ <= end_);
        return cursor_ == end_;
    }

    uint8_t readByte() {
        WH_ASSERT(!atEnd());
        return *cursor_++;
    }

    void rewindTo(uint32_t pos) {
        WH_ASSERT(pos <= position());
        cursor_ = start_ + pos;
    }

    void advanceTo(uint32_t pos) {
        WH_ASSERT(pos >= position());
        WH_ASSERT(pos <= positionOf(end_));
        cursor_ = start_ + pos;
    }

    void rewindBy(uint32_t count) {
        WH_ASSERT(count <= position());
        cursor_ -= count;
    }

  private:
    void installBuffer(const uint8_t *start, uint32_t size, uint32_t posn) {
        WH_ASSERT(posn <= size);
        start_ = start;
        end_ = start_ + size;
        cursor_ = start_ + posn;
    }
};


} // namespace Whisper

#endif // WHISPER__PARSER__CODE_SOURCE_HPP
