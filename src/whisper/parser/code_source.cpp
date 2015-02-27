
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <fstream>
#include <algorithm>

#include "code_source.hpp"

namespace Whisper {


//
// FileCodeSource
//

bool
FileCodeSource::initialize()
{
    WH_ASSERT(fd_ == -1);

    // try to open the file.
    fd_ = open(filename_, O_RDONLY);
    if (fd_ == -1) {
        error_ = "Could not open.";
        return false;
    }

    // find the size of the file
    struct stat st;
    if (fstat(fd_, &st) == -1) {
        error_ = "Could not stat.";
        return false;
    }

    // size too large.
    if (st.st_size > UINT32_MAX) {
        error_ = "File too large.";
        return false;
    }
    size_ = st.st_size;

    // For zero-length file, skip mmap
    if (size_ == 0) {
        data_ = nullptr;
        return true;
    }

    // mmap the file.
    void *data = mmap(NULL, size_, PROT_READ, MAP_PRIVATE, fd_, 0);
    if (data == MAP_FAILED) {
        error_ = "Could not mmap.";
        return false;
    }
    data_ = reinterpret_cast<uint8_t *>(data);

    return true;
}

void
FileCodeSource::finalize()
{
    if (fd_ == -1)
        return;

    // unmap the file if needed.
    if (data_ != nullptr)
        munmap(data_, size_);

    close(fd_);
#if defined(ENABLE_DEBUG)
    fd_ = -1;
    data_ = nullptr;
#endif
}

//
// SourceReader
//

SourceReader::SourceReader(CodeSource &source)
  : source_(source),
    size_(0),
    start_(nullptr),
    end_(nullptr),
    cursor_(nullptr),
    atEndOfInput_(false),
    error_(nullptr)
{
    WH_ASSERT(!source_.hasError());

    size_ = source.size();

    const uint8_t *buf = nullptr;
    uint32_t bufSize = 0;
    if (!source_.read(&buf, &bufSize)) {
        WH_ASSERT(source_.hasError());
        error_ = source_.error();
        return;
    }

    if (bufSize != size_) {
        error_ = "Incomplete read of code source.";
        return;
    }

    installBuffer(buf, bufSize, 0);
}


} // namespace Whisper
