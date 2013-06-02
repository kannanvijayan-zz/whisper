
#include <iostream>
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
}

#endif // defined(ENABLE_DEBUG)


} // namespace Whisper
