
#include <unictype.h>
#include <string.h>
#include <algorithm>
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

//
// KeywordTable implementation.
// 
struct KeywordTableEntry
{
    Token::Type token;
    const char *keywordString;
    uint32_t lastBytesPacked;
    uint8_t priority;
    uint8_t length;
    bool futureReserved;
    bool strictFutureReserved;
};
#define KW_TABSZ_(tokId,name,prio) +1
static constexpr unsigned KEYWORD_TABLE_SIZE = 0
        WHISPER_DEFN_KEYWORDS(KW_TABSZ_);
#undef KW_TABSZ_

static KeywordTableEntry KEYWORD_TABLE[KEYWORD_TABLE_SIZE];
static bool KEYWORD_TABLE_INITIALIZED = false;

// Array storing ofsets into keyword table for keywords of particular
// lengths.  Max keyword length is 10.  There are 12 entries in the
// array to allow for a "zero" entry for length 0, and
// "final" entry that points to the end of the keyword list.
static constexpr unsigned KEYWORD_MAX_LENGTH = 10;
static unsigned KEYWORD_TABLE_SECTIONS[KEYWORD_MAX_LENGTH+2];

static uint32_t
ComputeLastBytesPacked(const char *str, uint8_t length)
{
    if (length == 2) {
        return (static_cast<uint32_t>(str[0]) << 8)
             | (static_cast<uint32_t>(str[1]) << 0);
    }

    if (length == 3) {
        return (static_cast<uint32_t>(str[0]) << 16)
             | (static_cast<uint32_t>(str[1]) << 8)
             | (static_cast<uint32_t>(str[2]) << 0);
    }


    str += length - 4;
    return (static_cast<uint32_t>(str[0]) << 24)
         | (static_cast<uint32_t>(str[1]) << 16)
         | (static_cast<uint32_t>(str[2]) << 8)
         | (static_cast<uint32_t>(str[3]) << 0);
}

unsigned InitializeKeywordSection(unsigned len, unsigned start)
{
    for (unsigned i = start; i < KEYWORD_TABLE_SIZE; i++) {
        if (KEYWORD_TABLE[i].length >= len) {
            KEYWORD_TABLE_SECTIONS[len] = i;
            return i;
        }
    }
    WH_UNREACHABLE("Unreachable.");
    return 0;
}

void InitializeKeywordTable()
{
    WH_ASSERT(!KEYWORD_TABLE_INITIALIZED);

    // Write out the keyword table entries in order.
    // Initially, use lastBytesPacked to store priority.
    // Later overwrite it with the actual packed last bytes of the keyword.
    unsigned idx = 0;
#define KW_ADDENT_(tokId,name,prio) \
    KEYWORD_TABLE[idx].token = Token::tokId; \
    KEYWORD_TABLE[idx].keywordString = name; \
    KEYWORD_TABLE[idx].priority = prio; \
    KEYWORD_TABLE[idx].length = strlen(name); \
    KEYWORD_TABLE[idx].lastBytesPacked = \
        ComputeLastBytesPacked(name, KEYWORD_TABLE[idx].length); \
    KEYWORD_TABLE[idx].futureReserved = \
        (prio == WHISPER_KEYWORD_FUTURE_RESERVED_PRIO); \
    KEYWORD_TABLE[idx].strictFutureReserved = \
        (prio == WHISPER_KEYWORD_STRICT_FUTURE_RESERVED_PRIO); \
    idx += 1;
    WHISPER_DEFN_KEYWORDS(KW_ADDENT_);
#undef KW_ADDENT_

    // Sort the table for easy lookup.
    // Keywords are sorted by (length, priority)
    struct Less {
        inline Less() {}
        inline bool operator ()(const KeywordTableEntry &a,
                                const KeywordTableEntry &b)
        {
            if (a.length < b.length)
                return true;
            if (a.length > b.length)
                return false;
            return a.priority < b.priority;
        }
    };
    std::sort(&KEYWORD_TABLE[0], &KEYWORD_TABLE[KEYWORD_TABLE_SIZE], Less());

    // Scan and store the section offests.
    unsigned offset = 0;
    for (unsigned i = 0; i < KEYWORD_MAX_LENGTH; i++)
        offset = InitializeKeywordSection(i, offset);

    KEYWORD_TABLE_SECTIONS[KEYWORD_MAX_LENGTH+1] = KEYWORD_TABLE_SIZE;
    KEYWORD_TABLE_INITIALIZED = true;
}


//
// Tokenizer implementation.
//

TokenizerMark
Tokenizer::mark() const
{
    return TokenizerMark(stream_.position(),
                         line_,
                         stream_.cursor() - lineStart_,
                         strict_,
                         pushedBackToken_,
                         tok_);
}

void
Tokenizer::gotoMark(const TokenizerMark &mark)
{
    stream_.rewindTo(mark.position());
    line_ = mark.line();
    lineStart_ = stream_.cursor() - mark.lineOffset();
    WH_ASSERT(strict_ == mark.strict());
    WH_ASSERT(mark.pushedBackToken() == mark.token().debug_isPushedBack());
    pushedBackToken_ = mark.pushedBackToken();
    tok_ = mark.token();
}

Token
Tokenizer::getAutomaticSemicolon() const
{
    uint32_t lineOffset = stream_.cursor() - lineStart_;
    return Token(Token::Semicolon, stream_.position(), 0,
                 line_, lineOffset, line_, lineOffset);
}

void
Tokenizer::pushBackLastToken()
{
    WH_ASSERT(!pushedBackToken_);
    WH_ASSERT(!tok_.debug_isPushedBack());
    WH_ASSERT(!tok_.isError());
    pushedBackToken_ = true;
    rewindToToken(tok_);
    tok_.debug_markPushedBack();
}

const Token &
Tokenizer::readToken(InputElementKind iek, bool checkKeywords)
{
    if (pushedBackToken_) {
        WH_ASSERT(tok_.debug_isPushedBack());
        pushedBackToken_ = false;
        tok_.debug_clearPushedBack();
        tok_.debug_clearUsed();
        advancePastToken(tok_);
        return tok_;
    }

    try {
        return readTokenImpl(iek, checkKeywords);
    } catch (TokenizerError exc) {
        return emitToken(Token::Error);
    }
}

const Token &
Tokenizer::readTokenImpl(InputElementKind iek, bool checkKeywords)
{
    WH_ASSERT(!hasError());

    // Start the next token.
    startToken();

    unic_t ch = readChar();

    // Whitespace, simple identifiers, numbers, and strings will be very
    // common.  Check for them first.
    if (IsWhitespace(ch))
        return readWhitespace();

    if (IsKeywordChar(ch)) {
        if (checkKeywords)
            return readIdentifier(ch);
        return readIdentifierName();
    }

    if (IsNonKeywordSimpleIdentifierStart(ch))
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

void
Tokenizer::rewindToToken(const Token &tok)
{
    // Find the stream position to rewind to.
    stream_.rewindTo(tok.offset());
    line_ = tok.startLine();
    lineStart_ = stream_.cursor() - tok.startLineOffset();
}

void
Tokenizer::advancePastToken(const Token &tok)
{
    // Find the stream position to advance to.
    stream_.advanceTo(tok.endOffset());
    line_ = tok.endLine();
    lineStart_ = stream_.cursor() - tok.endLineOffset();
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

const Token &
Tokenizer::readIdentifier(unic_t firstChar)
{
    WH_ASSERT(IsKeywordChar(firstChar));
    uint32_t lastBytesPacked = firstChar;

    for (;;) {
        unic_t ch = readChar();

        if (IsKeywordChar(ch)) {
            lastBytesPacked <<= 8;
            lastBytesPacked |= ch;
            continue;
        }

        // Common case: simple identifier continuation.
        if (IsNonKeywordSimpleIdentifierContinue(ch))
            return readIdentifierName();

        // Unicode escape in identifier means its not a keyword.
        if (ch == '\\') {
            consumeUnicodeEscapeSequence();
            return readIdentifierName();
        }

        // Any other ASCII char means end of identifier.
        // Check this first because it's more likely than
        // a complex identifier continue char.
        if (IsAscii(ch)) {
            WH_ASSERT(!IsComplexIdentifierContinue(ch));
            unreadChar(ch);
            break;
        }

        // Check for complex identifier continue char.
        // If found, it cannot be a keyword.
        if (IsComplexIdentifierContinue(ch))
            return readIdentifierName();

        // Any other char means end of identifier
        unreadChar(ch);
        break;
    }

    // Check for keyword.  Use the token length to search.
    WH_ASSERT(KEYWORD_TABLE_INITIALIZED);

    unsigned tokenLength = stream_.cursor() - tokStart_;
    WH_ASSERT(tokenLength >= 1);

    // Single letter identifiers are common, and can't be keywords.
    // Very long identifiers also can't be keywords
    if (tokenLength < 2 || tokenLength > KEYWORD_MAX_LENGTH)
        return emitToken(Token::IdentifierName);

    unsigned startIdx = KEYWORD_TABLE_SECTIONS[tokenLength];
    unsigned endIdx = KEYWORD_TABLE_SECTIONS[tokenLength+1];

    // Check for keyword by scanning keyword table by length.
    for (unsigned i = startIdx; i < endIdx; i++) {
        KeywordTableEntry &ent = KEYWORD_TABLE[i];
        WH_ASSERT_IF(ent.length <= 7, ent.length == tokenLength);

        if (ent.lastBytesPacked != lastBytesPacked)
            continue;

        if (ent.length <= 4)
            return emitToken(ent.token);

        if (ent.length <= 7 || ent.length == tokenLength) {
            if (memcmp(ent.keywordString, tokStart_, tokenLength-4) == 0)
                return emitToken(ent.token);
        }
    }

    // No keywords matched.
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
    WH_ASSERT(Token::IsValidType(type));

    // Previously emitted token must have been used.
    WH_ASSERT(tok_.debug_isUsed());

    tok_ = Token(type,
                 stream_.positionOf(tokStart_),
                 stream_.cursor() - tokStart_,
                 tokStartLine_, tokStartLineOffset_,
                 line_, stream_.cursor() - lineStart_);
    return tok_;
}

const Token &
Tokenizer::emitError(const char *msg)
{
    WH_ASSERT(!error_);
    error_ = msg;
    throw TokenizerError();
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
    WH_ASSERT(!IsSimpleIdentifierStart(ch) && (ch != '\\'));

    return uc_is_property(ch, UC_PROPERTY_ID_START);
}

/* static */ bool
Tokenizer::IsComplexIdentifierContinue(unic_t ch)
{
    WH_ASSERT(!IsSimpleIdentifierContinue(ch) && (ch != '\\'));

    constexpr unic_t ZWNJ = 0x200C;
    constexpr unic_t ZWJ  = 0x200D;
    if (CharIn<ZWNJ, ZWJ>(ch))
        return true;

    return uc_is_property(ch, UC_PROPERTY_ID_CONTINUE);
}


bool
TokenHasAsciiText(const CodeSource &source, const Token &tok,
                  const char *str, uint32_t length)
{
    if (tok.length() != length)
        return false;

    const uint8_t *text = tok.text(source);
    for (uint32_t i = 0; i < length; i++) {
        if (text[i] != str[i])
            return false;
    }
    return true;
}


} // namespace Whisper
