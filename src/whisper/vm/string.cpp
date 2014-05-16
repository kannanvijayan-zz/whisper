

#include "common.hpp"
#include "debug.hpp"
#include "vm/string.hpp"

#include <algorithm>

namespace Whisper {
namespace VM {

//
// UTF-8 Encoding:
//
// ASCII            - 0XXX-XXXX
// Two-Byte         - 110X-XXXX 10XX-XXXX
// Three-Byte       - 1110-XXXX 10XX-XXXX 10XX-XXXX
// Four-Byte        - 1111-0XXX 10XX-XXXX 10XX-XXXX 10XX-XXXX
//
// As UTF-8 is restricted to be under 0x10FFFF, five and six byte
// encodings are not supported.
//

template <typename Handler>
static inline void
StringReadUTF8Codepoint(const uint8_t *utf8Bytes, Handler &handler)
{
    uint8_t ch0 = utf8Bytes[0];

    // Check for 0XXX-XXXX values.
    if (ch0 < 0x80u) {
        handler(1, ch0);
        return;
    }

    // Check for 110X-XXXX 10XX-XXXX values.
    if (ch0 < 0xE0u) {
        WH_ASSERT(ch0 >= 0xC0u);
        uint8_t ch1 = utf8Bytes[1];
        handler(2, (uint32_t(ch0 & 0x1Fu) << 6) | uint32_t(ch1 & 0x3Fu));
        return;
    }

    // Check for 1110-XXXX 10XX-XXXX 10XX-XXXX values.
    if (ch0 < 0xF0u) {
        WH_ASSERT(ch0 >= 0xE0u);
        uint8_t ch1 = utf8Bytes[1];
        uint8_t ch2 = utf8Bytes[2];
        handler(3, (uint32_t(ch0 & 0x0Fu) << 12) |
                   (uint32_t(ch1 & 0x3Fu) << 6) |
                   uint32_t(ch2 & 0x3Fu));
        return;
    }

    // Check for 1111-0XXX 10XX-XXXX 10XX-XXXX 10XX-XXXX values.
    WH_ASSERT(ch0 < 0xF8u);
    WH_ASSERT(ch0 >= 0xF0u);
    uint8_t ch1 = utf8Bytes[1];
    uint8_t ch2 = utf8Bytes[2];
    uint8_t ch3 = utf8Bytes[3];
    uint32_t result = (uint32_t(ch0 & 0x0Fu) << 18) |
                      (uint32_t(ch1 & 0x3Fu) << 12) |
                      (uint32_t(ch2 & 0x3Fu) << 6)  |
                      uint32_t(ch3 & 0x3Fu);
    WH_ASSERT(result <= 0x10FFFF);
    handler(4, result);
}

String::String(uint32_t byteLength, const uint8_t *data)
  : length_(0)
{
    std::copy(data, data + byteLength, data_);

    // Count length of string.
    for (Cursor curs = begin(); curs < end(); advance(curs))
        length_++;
}

String::String(uint32_t byteLength, const char *data)
  : length_(byteLength)
{
    std::copy(data, data + byteLength, data_);
}

String::String(const char *data)
  : length_(strlen(data))
{
    std::copy(data, data + length_, data_);
}

String::String(const String &other)
  : length_(other.length_)
{
    std::copy(other.data_, other.data_ + other.byteLength(), data_);
}

void
String::advance(Cursor &cursor) const
{
    WH_ASSERT(cursor.offset() < byteLength());

    struct Handler {
        const String *str;
        Cursor &curs;
        Handler(const String *s, Cursor &c) : str(s), curs(c) {}
        void operator ()(unsigned bytes, uint32_t codepoint) {
            WH_ASSERT(bytes > 0);
            WH_ASSERT(curs.offset() <= str->byteLength() - bytes);
            curs.incrementOffset(bytes);
        }
    };

    Handler handler(this, cursor);
    StringReadUTF8Codepoint<Handler>(&data_[cursor.offset()], handler);
}

unic_t
String::read(const Cursor &cursor) const
{
    WH_ASSERT(cursor.offset() < byteLength());

    struct Handler {
        const String *str;
        const Cursor &curs;
        unic_t codepointOut;
        Handler(const String *s, const Cursor &c)
          : str(s), curs(c), codepointOut(InvalidUnicode)
        {}

        void operator ()(unsigned bytes, uint32_t codepoint) {
            WH_ASSERT(bytes > 0);
            WH_ASSERT(curs.offset() <= str->byteLength() - bytes);
            codepointOut = codepoint;
        }
    };

    Handler handler(this, cursor);
    StringReadUTF8Codepoint<Handler>(&data_[cursor.offset()], handler);

    WH_ASSERT(handler.codepointOut <= MaxUnicode);
    return handler.codepointOut;
}

unic_t
String::readAdvance(Cursor &cursor) const
{
    WH_ASSERT(cursor.offset() < byteLength());

    struct Handler {
        const String *str;
        Cursor &curs;
        unic_t codepointOut;
        Handler(const String *s, Cursor &c)
          : str(s), curs(c), codepointOut(InvalidUnicode)
        {}

        void operator ()(unsigned bytes, uint32_t codepoint) {
            WH_ASSERT(bytes > 0);
            WH_ASSERT(curs.offset() <= str->byteLength() - bytes);
            curs.incrementOffset(bytes);
            codepointOut = codepoint;
        }
    };

    Handler handler(this, cursor);
    StringReadUTF8Codepoint<Handler>(&data_[cursor.offset()], handler);

    WH_ASSERT(handler.codepointOut <= MaxUnicode);
    return handler.codepointOut;
}


} // namespace VM
} // namespace Whisper
