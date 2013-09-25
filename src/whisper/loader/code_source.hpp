#ifndef WHISPER__LOADER__CODE_SOURCE_HPP
#define WHISPER__LOADER__CODE_SOURCE_HPP

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
// Represents some abstract source of loaded code.
//
class CodeSource
{
  protected:
    const char *name_;

    const uint8_t *data_;
    const uint8_t *dataEnd_;
    uint32_t dataSize_;

    CodeSource(const char *name);
    ~CodeSource();

  public:
    const char *name() const;

    const uint8_t *data() const;

    uint32_t dataSize() const;

    const uint8_t *dataEnd() const;
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
    const char *error_ = nullptr;

  public:
    FileCodeSource(const char *filename);
    ~FileCodeSource();

    void finalize();

  public:
    bool initialize();

    bool hasError() const;
    const char *error() const;
};

//
// SourceStream
//
// A stream API built on top of a CodeStream to allow byte-by-byte
// access.
//
class SourceStream
{
  private:
    CodeSource &source_;
    const uint8_t *cursor_;

  public:
    SourceStream(CodeSource &source);

    CodeSource &source() const;

    const uint8_t *cursor() const;

    uint32_t positionOf(const uint8_t *ptr) const;
    uint32_t position() const;

    bool atEnd() const;

    uint8_t readByte();

    void rewindTo(uint32_t pos);

    void advanceTo(uint32_t pos);

    void rewindBy(uint32_t count);
};


} // namespace Whisper

#endif // WHISPER__LOADER__CODE_SOURCE_HPP
