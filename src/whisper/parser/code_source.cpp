
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <algorithm>

#include "code_source.hpp"

namespace Whisper {


FileCodeSource::FileCodeSource(const char *filename)
  : filename_(filename)
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
        munmap(const_cast<uint8_t *>(data_), size_);

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
    fd_ = open(filename_, O_RDONLY);
    if (fd_ == -1) {
        finalize();
        return false;
    }

    // find the size of the file
    struct stat st;
    if (fstat(fd_, &st) == -1) {
        finalize();
        return false;
    }

    // size too large.
    if (st.st_size > SIZE_MAX) {
        finalize();
        return false;
    }
    size_ = st.st_size;

    // mmap the file.
    void *data = mmap(NULL, size_, PROT_READ, MAP_PRIVATE, fd_, 0);
    if (data == MAP_FAILED) {
        finalize();
        return false;
    }
    data_ = reinterpret_cast<uint8_t *>(data);

    return true;
}

const uint8_t *
FileCodeSource::name()
{
    return reinterpret_cast<const uint8_t *>(filename_);
}

size_t
FileCodeSource::position()
{
    return pos_;
}

void
FileCodeSource::mark()
{
#if defined(ENABLE_DEBUG)
    mark_ = pos_;
    marked_ = true;
#endif
}

void
FileCodeSource::erase()
{
#if defined(ENABLE_DEBUG)
    WH_ASSERT(marked_);
    mark_ = 0;
    marked_ = false;
#endif
}

bool
FileCodeSource::read(uint8_t *buf, size_t size, size_t *bytesRead)
{
    WH_ASSERT(buf);
    WH_ASSERT(bytesRead);
    size_t left = size_ - pos_;
    size_t toRead = (size <= left) ? size : left;
    const uint8_t *readFrom = data_ + pos_;
    std::copy(readFrom, readFrom + toRead, buf);
    pos_ += toRead;
    *bytesRead = toRead;
    return true;
}

bool
FileCodeSource::rewindTo(size_t toPos)
{
#if defined(ENABLE_DEBUG)
    WH_ASSERT(marked_);
    WH_ASSERT(toPos >= marked_);
    WH_ASSERT(toPos <= size_);
#endif
    pos_ = toPos;
    return true;
}



bool
SourceStream::initialize()
{
    WH_ASSERT(!bufferEnd_);
    WH_ASSERT(!bufferCur_);
    WH_ASSERT(bufferStartPos_ == 0);
    WH_ASSERT(markDepth_ == 0);
    WH_ASSERT(!atEnd_);

    return advance();
}

int
SourceStream::readByteSlow()
{
    WH_ASSERT(bufferCur_ == bufferEnd_);

    bufferStartPos_ += (bufferEnd_ - &buffer_[0]);
    if (!advance())
        return Error;

    if (atEnd_)
        return End;

    WH_ASSERT(bufferCur_ < bufferEnd_);
    return *bufferCur_++;
}

bool
SourceStream::rewindSlow(size_t pos)
{
    if (!source_.rewindTo(pos))
        return false;
    bufferStartPos_ = pos;
    if (!advance())
        return false;

    return true;
}

bool
SourceStream::advance()
{
    WH_ASSERT(!atEnd_);

    size_t nread;
    if (!source_.read(buffer_, BufferSize, &nread))
        return false;
    bufferEnd_ = &buffer_[0] + nread;
    bufferCur_ = &buffer_[0];
    if (nread == 0)
        atEnd_ = true;
    return true;
}


} // namespace Whisper
