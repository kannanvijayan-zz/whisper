
#include <iostream>
#include <stdlib.h>
#include "debug.hpp"

namespace Whisper {


#if defined(ENABLE_DEBUG)

void
Assert(const char *file, int line, const char *condstr, bool cond)
{
    if (cond)
        return;
    std::cerr << "!ASSERT! @ " << file << ":" << line
              << " [" << condstr << "]" << std::endl;
    abort();
}

void
AssertUnreachable(const char *file, int line, const char *msg)
{
    std::cerr << "!UNREACHABLE! @ " << file << ":" << line
              << " [" << msg << "]" << std::endl;
    abort();
}

#endif // defined(ENABLE_DEBUG)


} // namespace Whisper
