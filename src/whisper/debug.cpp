
#include <iostream>
#include <stdlib.h>
#include "debug.hpp"

namespace Whisper {


#if defined(ENABLE_DEBUG)

void
Assert(char const* file, int line, char const* condstr, bool cond)
{
    if (cond)
        return;
    std::cerr << "!ASSERT! @ " << file << ":" << line
              << " [" << condstr << "]" << std::endl;
    abort();
}

void
AssertUnreachable(char const* file, int line, char const* msg)
{
    std::cerr << "!UNREACHABLE! @ " << file << ":" << line
              << " [" << msg << "]" << std::endl;
    abort();
}

#endif // defined(ENABLE_DEBUG)


} // namespace Whisper
