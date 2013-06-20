
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <fstream>
#include <algorithm>

#include "code_source.hpp"

namespace Whisper {


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
        return false;
    }

    // find the size of the file
    struct stat st;
    if (fstat(fd_, &st) == -1) {
        finalize();
        return false;
    }

    // size too large.
    if (st.st_size > UINT32_MAX) {
        finalize();
        return false;
    }
    dataSize_ = st.st_size;

    // mmap the file.
    void *data = mmap(NULL, dataSize_, PROT_READ, MAP_PRIVATE, fd_, 0);
    if (data == MAP_FAILED) {
        finalize();
        return false;
    }
    data_ = reinterpret_cast<uint8_t *>(data);
    dataEnd_ = data_ + dataSize_;

    return true;
}


} // namespace Whisper
