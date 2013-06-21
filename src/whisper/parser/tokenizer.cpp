
#include <unictype.h>
#include "parser/tokenizer.hpp"


namespace Whisper {


const char *
Token::TypeString(Type type)
{
    switch (type) {
#define DEF_CASE_(tok) \
      case tok: \
        return #tok;
      WHISPER_DEFN_TOKENS(DEF_CASE_)
#undef DEF_CASE_
      case INVALID:
        return "INVALID";
      case LIMIT:
        return "INVALID_LIMIT";
    }
    return "INVALID_UNKNOWN";
}

inline const Token &
Tokenizer::readInputElement(InputElementKind iek)
{
    if (pushedBack_) {
        pushedBack_ = false;
        return tok_;
    }

    try {
        return readInputElementImpl(iek);
    } catch(TokenizerError err) {
        return emitToken(Token::Error);
    }
}

const Token &
Tokenizer::readInputElementImpl(InputElementKind iek)
{
    WH_ASSERT(!hasError());

    // Start the next token.
    startToken();

    unic_t ch = readChar();

    // Whitespace, simple identifiers, numbers, and strings will be very
    // common.  Check for them first.
    if (IsWhitespace(ch))
        return readWhitespace();

    if (IsSimpleIdentifierStart(ch))
        return readIdentifierName();

    if (IsDigit(ch))
        return readNumericLiteral(ch == '0');

    if (ch == '\'' || ch == '"')
        return readStringLiteral(ch);

    // Next, check for punctuators, ordered from an intuitive sense
    // of most common to least common.
    if (ch == '(')
        return emitToken(Token::OpenParen);

    if (ch == ')')
        return emitToken(Token::CloseParen);

    if (ch == '.') {
        // Check for decimal literal.
        unic_t ch = readChar();
        if (IsDigit(ch))
            return readNumericLiteralFraction();
        unreadChar(ch);
        return emitToken(Token::Dot);
    }

    if (ch == ',')
        return emitToken(Token::Comma);

    if (ch == '=') {
        unic_t ch2 = readChar();
        if (ch2 == '=') {
            unic_t ch3 = readChar();
            if (ch3 == '=')
                return emitToken(Token::StrictEqual);

            unreadChar(ch3);
            return emitToken(Token::Equal);
        }

        unreadChar(ch2);
        return emitToken(Token::Assign);
    }

    if (ch == '!') {
        unic_t ch2 = readChar();
        if (ch2 == '=') {
            unic_t ch3 = readChar();
            if (ch3 == '=')
                return emitToken(Token::StrictNotEqual);

            unreadChar(ch3);
            return emitToken(Token::NotEqual);
        }

        unreadChar(ch2);
        return emitToken(Token::LogicalNot);
    }

    if (ch == '<') {
        unic_t ch2 = readChar();
        if (ch2 == '=') {
            return emitToken(Token::LessEqual);
        } else if (ch2 == '<') {
            unic_t ch3 = readChar();
            if (ch3 == '=')
                return emitToken(Token::ShiftLeftAssign);

            unreadChar(ch3);
            return emitToken(Token::ShiftLeft);
        }

        unreadChar(ch2);
        return emitToken(Token::LessThan);
    }

    if (ch == '>') {
        unic_t ch2 = readChar();
        if (ch2 == '=') {
            return emitToken(Token::GreaterEqual);
        } else if (ch2 == '>') {
            unic_t ch3 = readChar();
            if (ch3 == '=') {
                return emitToken(Token::ShiftRightAssign);
            } else if (ch3 == '>') {
                unic_t ch4 = readChar();
                if (ch4 == '=')
                    return emitToken(Token::ShiftUnsignedRightAssign);

                unreadChar(ch4);
                return emitToken(Token::ShiftUnsignedRight);
            }

            unreadChar(ch3);
            return emitToken(Token::ShiftRight);
        }

        unreadChar(ch2);
        return emitToken(Token::GreaterThan);
    }

    if (ch == '+') {
        unic_t ch2 = readChar();
        if (ch2 == '+')
            return emitToken(Token::PlusPlus);
        else if (ch2 == '=')
            return emitToken(Token::PlusAssign);

        unreadChar(ch2);
        return emitToken(Token::Plus);
    }

    if (ch == '-') {
        unic_t ch2 = readChar();
        if (ch2 == '-')
            return emitToken(Token::MinusMinus);
        else if (ch2 == '=')
            return emitToken(Token::MinusAssign);

        unreadChar(ch2);
        return emitToken(Token::Minus);
    }

    if (ch == '&') {
        unic_t ch2 = readChar();
        if (ch2 == '&')
            return emitToken(Token::LogicalAnd);
        else if (ch2 == '=')
            return emitToken(Token::BitAndAssign);

        unreadChar(ch2);
        return emitToken(Token::BitAnd);
    }

    if (ch == '|') {
        unic_t ch2 = readChar();
        if (ch2 == '|')
            return emitToken(Token::LogicalOr);
        else if (ch2 == '=')
            return emitToken(Token::BitOrAssign);

        unreadChar(ch2);
        return emitToken(Token::BitOr);
    }

    if (ch == '*') {
        unic_t ch2 = readChar();
        if (ch2 == '=')
            return emitToken(Token::StarAssign);

        unreadChar(ch2);
        return emitToken(Token::Star);
    }

    // '/' can be a comment, divide operator, or regex
    if (ch == '/') {
        unic_t ch2 = readChar();

        // Check for comment.
        if (ch2 == '*')
            return readMultiLineComment();
        else if (ch2 == '/')
            return readSingleLineComment();

        WH_ASSERT(iek == InputElement_Div || iek == InputElement_RegExp);
        if (iek == InputElement_Div)
            return readDivPunctuator(ch2);

        if (ch2 == End)
            emitError("Premature end of input in RegExp body.");
        return readRegularExpressionLiteral(ch2);
    }

    if (ch == '{')
        return emitToken(Token::OpenBrace);

    if (ch == '}')
        return emitToken(Token::CloseBrace);

    if (ch == '[')
        return emitToken(Token::OpenBracket);

    if (ch == ']')
        return emitToken(Token::CloseBracket);

    if (ch == ';')
        return emitToken(Token::Semicolon);

    if (ch == '^') {
        unic_t ch2 = readChar();
        if (ch2 == '=')
            return emitToken(Token::BitXorAssign);

        unreadChar(ch2);
        return emitToken(Token::BitXor);
    }

    if (ch == '%') {
        unic_t ch2 = readChar();
        if (ch2 == '=')
            return emitToken(Token::PercentAssign);

        unreadChar(ch2);
        return emitToken(Token::Percent);
    }

    // Line terminators are probably more common the the following
    // three punctuators.
    if (IsLineTerminator(ch))
        return readLineTerminatorSequence(ch);

    if (ch == '~')
        return emitToken(Token::Tilde);

    if (ch == '?')
        return emitToken(Token::Question);

    if (ch == ':')
        return emitToken(Token::Colon);

    // Handle unicode escapes and complex identifiers last.
    if (ch == '\\') {
        consumeUnicodeEscapeSequence();
        return readIdentifierName();
    }

    if (IsComplexIdentifierStart(ch))
        return readIdentifierName();

    // End of stream is least common.
    if (ch == End)
        return emitToken(Token::End);

    return emitError("Unrecognized character.");
}

const Token &
Tokenizer::readWhitespace()
{
    for (;;) {
        unic_t ch = readChar();
        if (IsWhitespace(ch))
            continue;

        unreadChar(ch);
        break;
    }

    return emitToken(Token::Whitespace);
}

const Token &
Tokenizer::readLineTerminatorSequence(unic_t ch)
{
    finishLineTerminator(ch);
    startNewLine();
    return emitToken(Token::LineTerminatorSequence);
}

const Token &
Tokenizer::readMultiLineComment()
{
    bool sawStar = false;
    for (;;) {
        unic_t ch = readNonEndChar();

        if (sawStar && (ch == '/'))
            break;

        if (IsLineTerminator(ch)) {
            finishLineTerminator(ch);
            startNewLine();
        }

        sawStar = (ch == '*');
    }
    return emitToken(Token::MultiLineComment);
}

const Token &
Tokenizer::readSingleLineComment()
{
    for (;;) {
        unic_t ch = readChar();

        if (IsLineTerminator(ch) || ch == End) {
            unreadChar(ch);
            break;
        }
    }
    return emitToken(Token::SingleLineComment);
}

const Token &
Tokenizer::readDivPunctuator(unic_t ch)
{
    if (ch == '=')
        return emitToken(Token::DivideAssign);

    unreadChar(ch);
    return emitToken(Token::Divide);
}

const Token &
Tokenizer::readRegularExpressionLiteral(unic_t ch)
{
    WH_ASSERT(ch != End);

    // First char of regexp body comes in as argument.
    if (IsLineTerminator(ch))
        return emitError("Line terminator in RegExp body.");
    else if (ch == '\\')
        consumeRegularExpressionBackslashSequence();
    else if (ch == '[')
        consumeRegularExpressionCharacterClass();

    // Read remaining chars.
    for (;;) {
        ch = readNonEndChar();
        if (ch == '\\')
            consumeRegularExpressionBackslashSequence();
        else if (ch == '[')
            consumeRegularExpressionCharacterClass();
        else if (IsLineTerminator(ch))
            emitError("Line terminator in RegExp body.");
        else if (ch == '/')
            break;
    }
    return emitToken(Token::RegularExpressionLiteral);
}

inline void
Tokenizer::consumeRegularExpressionBackslashSequence()
{
    unic_t ch = readNonEndChar();
    if (IsLineTerminator(ch))
        emitError("Line terminator in RegExp backslash sequence.");
}

void
Tokenizer::consumeRegularExpressionCharacterClass()
{
    for (;;) {
        unic_t ch = readNonEndChar();
        if (ch == '\\')
            consumeRegularExpressionBackslashSequence();
        else if (IsLineTerminator(ch))
            emitError("Line terminator in RegExp character class.");
        else if (ch == ']')
            break;
    }
}

const Token &
Tokenizer::readIdentifierName()
{
    for (;;) {
        unic_t ch = readChar();

        // Common case: simple identifier continuation.
        if (IsSimpleIdentifierContinue(ch))
            continue;

        if (ch == '\\') {
            consumeUnicodeEscapeSequence();
            continue;
        }

        // Any other ASCII char means end of identifier.
        // Check this first because it's more likely than
        // a complex identifier continue char.
        if (IsAscii(ch)) {
            unreadChar(ch);
            break;
        }

        // Check for complex identifier continue char.
        if (IsComplexIdentifierContinue(ch))
            continue;

        // Any other char means end of identifier
        unreadChar(ch);
        break;
    }
    return emitToken(Token::IdentifierName);
}

void
Tokenizer::consumeUnicodeEscapeSequence()
{
    // Read 4 hex characters.
    for (int i = 0; i < 4; i++) {
        unic_t ch = readNonEndChar();
        if (!IsHexDigit(ch))
            emitError("Invalid unicode escape sequence.");
    }
}

const Token &
Tokenizer::readNumericLiteral(bool startsWithZero)
{
    unic_t ch = readChar();

    // Check for hex vs. decimal literal.
    if (startsWithZero && CharIn<'x','X'>(ch))
        return readHexIntegerLiteral();

    // Check for fraction.
    if (ch == '.')
        return readNumericLiteralFraction();

    // Check for exponent.
    if (CharIn<'e','E'>(ch))
        return readNumericLiteralExponent();

    // Otherwise check for non-digit char.
    if (!IsDigit(ch)) {
        unreadChar(ch);
        return emitToken(Token::NumericLiteral);
    }

    // Zero followed by another digit is a not valid.
    if (startsWithZero) {
        WH_ASSERT(IsDigit(ch));
        return emitError("Digit following 0 in decimal literal.");
    }

    // Otherwise, keep reading chars.
    for (;;) {
        ch = readChar();

        if (IsDigit(ch))
            continue;

        // Check for fraction.
        if (ch == '.')
            return readNumericLiteralFraction();

        // Check for exponent.
        if (CharIn<'e','E'>(ch))
            return readNumericLiteralExponent();

        unreadChar(ch);
        break;
    }
    return emitToken(Token::NumericLiteral);
}

const Token &
Tokenizer::readNumericLiteralFraction()
{
    for (;;) {
        unic_t ch = readChar();

        if (IsDigit(ch))
            continue;

        // Check for exponent.
        if (CharIn<'e','E'>(ch))
            return readNumericLiteralExponent();

        unreadChar(ch);
        break;
    }
    return emitToken(Token::NumericLiteral);
}

const Token &
Tokenizer::readNumericLiteralExponent()
{
    unic_t ch = readChar();

    // Skip any sign char
    if (CharIn<'-','+'>(ch))
        ch = readChar();

    // First char must be a digit.
    if (!IsDigit(ch))
        return emitError("Numeric literal exponent not followed by digit.");

    for (;;) {
        ch = readChar();

        if (IsDigit(ch))
            continue;

        unreadChar(ch);
        break;
    }
    return emitToken(Token::NumericLiteral);
}

const Token &
Tokenizer::readHexIntegerLiteral()
{
    for (;;) {
        unic_t ch = readChar();

        if (IsHexDigit(ch))
            continue;

        unreadChar(ch);
        break;
    }
    return emitToken(Token::NumericLiteral);
}

const Token &
Tokenizer::readStringLiteral(unic_t quoteChar)
{
    for (;;) {
        unic_t ch = readNonEndChar();

        if (ch == quoteChar);
            break;

        if (ch == '\\')
            consumeStringEscapeSequence();

        if (IsLineTerminator(ch))
            emitError("Unescaped line terminator in string.");
    }

    return emitToken(Token::StringLiteral);
}

void
Tokenizer::consumeStringEscapeSequence()
{
    unic_t ch = readNonEndChar();
    if (CharIn<'n', 'r', 't', '\'', '"', '\\', 'v', 'b', 'f'>(ch))
        return;

    if (ch == '0') {
        unic_t ch2 = readNonEndChar();
        if (IsDigit(ch2))
            emitError("Digit following backslash-zero in string.");
        unreadChar(ch2);
        return;
    }

    if (ch == 'x') {
        for (int i = 0; i < 2; i++) {
            unic_t ch2 = readNonEndChar();
            if (!IsHexDigit(ch2))
                emitError("Invalid string hex escape sequence.");
        }
        return;
    }

    if (ch == 'u') {
        for (int i = 0; i < 4; i++) {
            unic_t ch2 = readNonEndChar();
            if (!IsHexDigit(ch2))
                emitError("Invalid string unicode escape sequence.");
        }
        return;
    }

    if (IsLineTerminator(ch)) {
        if (ch == '\r') {
            unic_t ch2 = readNonEndChar();
            if (ch2 != '\n')
                unreadChar(ch2);
        }
        return;
    }
}

inline const Token &
Tokenizer::emitToken(Token::Type type)
{
    WH_ASSERT(!pushedBack_);
    WH_ASSERT(Token::IsValidType(type));

    tok_ = Token(type,
                 stream_.positionOf(tokStart_),
                 stream_.cursor() - tokStart_,
                 tokStartLine_, tokStartLineOffset_,
                 line_, lineOffset());
    return tok_;
}

const Token &
Tokenizer::emitError(const char *msg)
{
    WH_ASSERT(!error_);
    error_ = msg;
    throw TokenizerError();
}

void
Tokenizer::rewindToToken(const Token &tok)
{
    // Find the stream position to rewind to.
    stream_.rewindTo(tok.offset());
    line_ = tok.startLine();
    lineStart_ = stream_.cursor() - tok.startLineOffset();
}

void
Tokenizer::pushbackImplicitSemicolon()
{
    WH_ASSERT(!pushedBack_);

    tok_ = Token(Token::Semicolon,
                 stream_.position(), 0,
                 line_, lineOffset(),
                 line_, lineOffset());
    pushedBack_ = true;
}

unic_t
Tokenizer::readCharSlow(unic_t firstByte)
{
    WH_ASSERT(firstByte > 0x7F);
    WH_ASSERT(MaxUnicode == 0x10FFFFu);

    // First byte with bits 10XX-XXXX is not a valid unicode char.
    if (firstByte <= 0xBF)
        emitError("Invalid unicode character: 0x7F < firstByte <= 0xBF.");

    // firstByte >= 1100-0000
    // Check for 2-byte UTF-8 char: firstByte <= 1101-1111
    if (firstByte <= 0xDF) {
        // Resulting bytes: 110A-AAAA 10BB-BBBB
        // Char bits: 5 + 6 = 11.
        // Max value: 0x7ff
        unic_t secondByte = readCharNextByte();

        return ((firstByte & 0x1F)  << 6) |
               ((secondByte & 0x3F) << 0);
    }

    // Check for 3-byte UTF-8 char: firstByte <= 1110-1111
    // firstByte >= 1110-0000
    if (firstByte <= 0xEF) {
        // Resulting bytes: 1110-AAAA 10BB-BBBB 10CC-CCCC
        // Char bits: 4 + 6 + 6 = 16.
        // Max value: 0xffff
        unic_t secondByte = readCharNextByte();
        unic_t thirdByte = readCharNextByte();

        return ((firstByte & 0x1F)  << 12) |
               ((secondByte & 0x3F) << 6) |
               ((thirdByte & 0x3F)  << 0);
    }

    // firstByte >= 1111-0000
    // Check for 4-byte UTF-8 char: firstByte <= 1111-0111
    if (firstByte <= 0xF7) {
        // Resulting bytes: 1111-0AAA 10BB-BBBB 10CC-CCCC 10DD-DDDD
        // Char bits: 3 + 6 + 6 + 6 = 21.
        // Max value: 0x1fffff
        unic_t secondByte = readCharNextByte();
        unic_t thirdByte = readCharNextByte();
        unic_t fourthByte = readCharNextByte();

        return ((firstByte & 0x1F)  << 18) |
               ((secondByte & 0x3F) << 12) |
               ((thirdByte & 0x3F)  << 6) |
               ((fourthByte & 0x3F) << 0);
    }

    // firstByte >= 1111-1100
    emitError("Invalid unicode character: firstByte > 0xFB.");
    return Error;
}

uint8_t
Tokenizer::readCharNextByte()
{
    if (stream_.atEnd())
        emitError("Incomplete unicode character.");

    // Non-first unicode characters must be in range 1000-0000 to 1011-1111
    uint8_t byte = stream_.readByte();
    if (byte < 0x80 || byte > 0xBF)
        emitError("Invalid unicode character: <0x80 | >0xBF.");

    return byte;
}

void
Tokenizer::slowUnreadChar(unic_t ch)
{
    // Unreading an error is a no-op.
    WH_ASSERT(ch > 0x7F);
    WH_ASSERT(ch == End || ch <= MaxUnicode);

    // Up to 5 + 6 bits = 11 bits, 2 byte char
    // 3 + 8 bits
    if (ch <= 0x7ff) {
        stream_.rewindBy(2);
        return;
    }

    // Up to 4 + 6 + 6 bits = 16 bits, 3 byte char
    // 8 + 8 bits
    if (ch <= 0xffff) {
        stream_.rewindBy(3);
        return;
    }

    // Up to 3 + 6 + 6 + 6 bits = 21 bits, 4 byte char
    // 5 + 8 + 8 bits
    if (ch <= 0x1fffff) {
        stream_.rewindBy(4);
        return;
    }

    // Check for unreading End, which is a no-op.
    if (ch == End)
        return;

    WH_UNREACHABLE("Invalid character pushed back.");
}

/* static */ bool
Tokenizer::IsWhitespaceSlow(unic_t ch)
{
    static constexpr unic_t FF   = 0x000C;
    static constexpr unic_t VT   = 0x000B;
    static constexpr unic_t NBSP = 0x00A0;
    static constexpr unic_t BOM  = 0xFEFF;
    WH_ASSERT(!(CharIn<' ', '\t'>(ch)));
    return CharIn<FF, VT, NBSP, BOM>(ch) ||
           uc_is_general_category(ch, UC_SPACE_SEPARATOR);
}

/* static */ bool
Tokenizer::IsLineTerminatorSlow(unic_t ch)
{
    static constexpr unic_t LS = 0x2028;
    static constexpr unic_t PS = 0x2029;
    WH_ASSERT(!(CharIn<'\r', '\n'>(ch)));
    return CharIn<LS, PS>(ch);
}

/* static */ bool
Tokenizer::IsComplexIdentifierStart(unic_t ch)
{
    WH_ASSERT(!IsAsciiLetter(ch) && !(CharIn<'$', '_'>(ch)) && (ch != '\\'));

    return uc_is_property(ch, UC_PROPERTY_ID_START);
}

/* static */ bool
Tokenizer::IsComplexIdentifierContinue(unic_t ch)
{
    WH_ASSERT(!IsAsciiLetter(ch) && !IsDigit(ch) && !(CharIn<'$', '_'>(ch)) &&
              (ch != '\\'));

    constexpr unic_t ZWNJ = 0x200C;
    constexpr unic_t ZWJ  = 0x200D;
    if (CharIn<ZWNJ, ZWJ>(ch))
        return true;

    return uc_is_property(ch, UC_PROPERTY_ID_CONTINUE);
}


} // namespace Whisper
