#ifndef WHISPER__VM__STRING_INLINES_HPP
#define WHISPER__VM__STRING_INLINES_HPP


#include "vm/string.hpp"

namespace Whisper {
namespace VM {


inline uint32_t
String::length() const
{
    if (isFlatString())
        return toFlatString()->length();
    WH_UNREACHABLE("Invalid string kind.");
    return UINT32_MAX;
}

inline uint32_t
String::charAt(uint32_t idx) const
{
    WH_ASSERT(idx < length());
    if (isFlatString())
        return toFlatString()->charAt(idx);
    WH_UNREACHABLE("Invalid string kind.");
}


} // namespace VM
} // namespace Whisper


#endif // WHISPER__VM__STRING_INLINES_HPP
