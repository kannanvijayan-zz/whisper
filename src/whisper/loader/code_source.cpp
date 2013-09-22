
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <fstream>
#include <algorithm>

#include "loader/code_source.hpp"

namespace Whisper {


//
// CodeSource
//

CodeSource::CodeSource(const char *name)
  : name_(name)
{}

CodeSource::~CodeSource()
{}

const char *
CodeSource::name() const
{
    return name_;
}

const uint8_t *
CodeSource::data() const
{
    return data_;
}

uint32_t
CodeSource::dataSize() const
{
    return dataSize_;
}

const uint8_t *
CodeSource::dataEnd() const
{
    return dataEnd_;
}

//
// FileCodeSource
//


FileCodeSource::FileCodeSource(const char *filename)
  : CodeSource(filename)
{}

FileCodeSource::~FileCodeSource()
{
    finalize();
}

void
FileCodeSource::finalize()
{
    if (fd_ == -1)
        return;

    // unmap the file if needed.
    if (data_ != nullptr)
        munmap(const_cast<uint8_t *>(data_), dataSize_);

    close(fd_);
#if defined(ENABLE_DEBUG)
    fd_ = -1;
#endif
}

bool
FileCodeSource::initialize()
{
    WH_ASSERT(fd_ == -1);

    // try to open the file.
    fd_ = open(name_, O_RDONLY);
    if (fd_ == -1) {
        finalize();
        error_ = "Could not open.";
        return false;
    }

    // find the size of the file
    struct stat st;
    if (fstat(fd_, &st) == -1) {
        finalize();
        error_ = "Could not stat.";
        return false;
    }

    // size too large.
    if (st.st_size > UINT32_MAX) {
        finalize();
        error_ = "File too large.";
        return false;
    }
    dataSize_ = st.st_size;

    // For zero-length file, skip mmap
    if (dataSize_ == 0) {
        data_ = dataEnd_ = nullptr;
        return true;
    }

    // mmap the file.
    void *data = mmap(NULL, dataSize_, PROT_READ, MAP_PRIVATE, fd_, 0);
    if (data == MAP_FAILED) {
        finalize();
        error_ = "Could not mmap.";
        return false;
    }
    data_ = reinterpret_cast<uint8_t *>(data);
    dataEnd_ = data_ + dataSize_;

    return true;
}

bool
FileCodeSource::hasError() const
{
    return error_;
}

const char *
FileCodeSource::error() const
{
    WH_ASSERT(hasError());
    return error_;
}

//
// SourceStream
//

SourceStream::SourceStream(CodeSource &source)
    : source_(source),
      cursor_(source_.data())
{}

CodeSource &
SourceStream::source() const
{
    return source_;
}

const uint8_t *
SourceStream::cursor() const
{
    return cursor_;
}

uint32_t
SourceStream::positionOf(const uint8_t *ptr) const
{
    WH_ASSERT(ptr >= source_.data() && ptr <= source_.dataEnd());
    return ptr - source_.data();
}

uint32_t
SourceStream::position() const
{
    return positionOf(cursor_);
}

bool
SourceStream::atEnd() const
{
    return cursor_ == source_.dataEnd();
}

uint8_t
SourceStream::readByte()
{
    // readByte() should never be called on an EOLed stream.
    WH_ASSERT(!atEnd());
    return *cursor_++;
}

void
SourceStream::rewindTo(uint32_t pos)
{
    WH_ASSERT(pos <= position());
    cursor_ = source_.data() + pos;
}

void
SourceStream::advanceTo(uint32_t pos)
{
    WH_ASSERT(pos >= position());
    cursor_ = source_.data() + pos;
}

void
SourceStream::rewindBy(uint32_t count) {
    WH_ASSERT(count <= position());
    cursor_ -= count;
}


} // namespace Whisper
