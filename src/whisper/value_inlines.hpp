#ifndef WHISPER__VALUE_INLINES_HPP
#define WHISPER__VALUE_INLINES_HPP

#include "value.hpp"
#include <type_traits>

namespace Whisper {


template <typename CharT>
inline uint32_t
Value::readImmString8(CharT *buf) const
{
    WH_ASSERT(this->isImmString8());
    unsigned len = immString8Length();
    uint64_t data = tagged_ >> ImmString8DataShift;
    for (unsigned i = 0; i < len; i++) {
        buf[i] = data & 0xFFu;
        data >>= 8;
    }
    return len;
}

template <typename CharT, bool Trunc>
inline uint32_t
Value::readImmString16(CharT *buf) const
{
    static_assert(Trunc || sizeof(CharT) >= sizeof(uint16_t),
                  "Character type too small for non-truncating read.");
    WH_ASSERT(this->isImmString16());
    unsigned len = immString16Length();
    uint64_t data = tagged_ >> ImmString16DataShift;
    for (unsigned i = 0; i < len; i++) {
        buf[i] = data & 0xFFFFu;
        data >>= 16;
    }
    return len;
}

template <typename CharT, bool Trunc>
inline uint32_t
Value::readImmString(CharT *buf) const
{
    static_assert(Trunc || sizeof(CharT) >= sizeof(uint16_t),
                  "Character type too small for non-truncating read.");
    WH_ASSERT(isImmString8() || isImmString16());

    if (this->isImmString8())
        return this->readImmString8<CharT>(buf);

    return this->readImmString16<CharT, Trunc>(buf);
}



} // namespace Whisper

#endif // WHISPER__VALUE_INLINES_HPP
