
#include <unictype.h>
#include <string.h>
#include <algorithm>

#include "spew.hpp"
#include "parser/tokenizer.hpp"


namespace Whisper {


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
};
#define KW_TABSZ_(tokId,name,prio) +1
static constexpr unsigned KEYWORD_TABLE_SIZE = 0
        WHISPER_DEFN_KEYWORDS(KW_TABSZ_);
#undef KW_TABSZ_

static KeywordTableEntry KEYWORD_TABLE[KEYWORD_TABLE_SIZE];
static bool KEYWORD_TABLE_INITIALIZED = false;

// Array storing ofsets into keyword table for keywords of particular
// lengths.  One entry for every length up to the max keyword length,
// plus an additional two entries, one for dummy "zero-length" keywords,
// and another to point to the end of the keyword table.
static constexpr unsigned KEYWORD_MAX_LENGTH = 8;
static unsigned KEYWORD_TABLE_SECTIONS[KEYWORD_MAX_LENGTH+2];

static uint32_t
ComputeLastBytesPacked(const char *str, uint8_t length)
{
    WH_ASSERT(length >= 2);
    if (length == 2) {
        return (ToUInt32(str[0]) << 8)
             | (ToUInt32(str[1]) << 0);
    }

    if (length == 3) {
        return (ToUInt32(str[0]) << 16)
             | (ToUInt32(str[1]) << 8)
             | (ToUInt32(str[2]) << 0);
    }

    str += length - 4;
    return (ToUInt32(str[0]) << 24)
         | (ToUInt32(str[1]) << 16)
         | (ToUInt32(str[2]) << 8)
         | (ToUInt32(str[3]) << 0);
}

unsigned InitializeKeywordSection(unsigned len, unsigned start)
{
    WH_ASSERT(start <= KEYWORD_TABLE_SIZE);

    unsigned i;
    for (i = start; i < KEYWORD_TABLE_SIZE; i++) {
        if (KEYWORD_TABLE[i].length >= len)
            break;
    }

    KEYWORD_TABLE_SECTIONS[len] = i;
    return i;
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
    idx += 1;
    WHISPER_DEFN_KEYWORDS(KW_ADDENT_);
#undef KW_ADDENT_

    // Sort the table for easy lookup.
    // Keywords are sorted by (length, priority)
    struct Less {
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
    KEYWORD_TABLE_SECTIONS[0] = 0;
    unsigned offset = 0;
    for (unsigned i = 1; i <= KEYWORD_MAX_LENGTH; i++)
        offset = InitializeKeywordSection(i, offset);

    KEYWORD_TABLE_SECTIONS[KEYWORD_MAX_LENGTH+1] = KEYWORD_TABLE_SIZE;
    KEYWORD_TABLE_INITIALIZED = true;
}

static Token::Type
CheckKeywordTable(const uint8_t *text, unsigned length,
                  uint32_t lastBytesPacked)
{
    WH_ASSERT(KEYWORD_TABLE_INITIALIZED);
    WH_ASSERT(length >= 1);

    // Single letter identifiers are common, and can't be keywords.
    // Very long identifiers also can't be keywords
    if (length < 2 || length > KEYWORD_MAX_LENGTH)
        return Token::INVALID;

    unsigned startIdx = KEYWORD_TABLE_SECTIONS[length];
    unsigned endIdx = KEYWORD_TABLE_SECTIONS[length+1];

    WH_ASSERT(startIdx < KEYWORD_TABLE_SIZE);
    WH_ASSERT(endIdx <= KEYWORD_TABLE_SIZE);
    WH_ASSERT(startIdx <= endIdx);

    // Check for keyword by scanning first for a matching
    // lastBytesPacked.  For keywords of length 4 or less,
    // a match of lastBytesPacked means the keyword matches.
    // For longer keywords, compare all characters in the
    // string except the last 4 chars (which have already
    // been matched by lastBytesPacked).
    for (unsigned i = startIdx; i < endIdx; i++) {
        KeywordTableEntry &ent = KEYWORD_TABLE[i];
        WH_ASSERT(ent.length == length);

        if (ent.lastBytesPacked == lastBytesPacked) {
            if (length <= 4)
                return ent.token;

            if (memcmp(ent.keywordString, text, length-4) == 0)
                return ent.token;
        }
    }
    return Token::INVALID;
}


//
// QuickTokenTable implementation.
//
struct QuickTokenEntry
{
    Token::Type type;
};


static constexpr unsigned QUICK_TOKEN_TABLE_SIZE = 128;
static QuickTokenEntry QUICK_TOKEN_TABLE[QUICK_TOKEN_TABLE_SIZE];
static bool QUICK_TOKEN_TABLE_INITIALIZED = false;

static inline void
SetQuickTokenTableEntry(char ch, Token::Type tokType)
{
    uint8_t idx = ch;
    WH_ASSERT(idx < 128);
    QUICK_TOKEN_TABLE[idx].type = tokType;
}

void
InitializeQuickTokenTable()
{
    WH_ASSERT(!QUICK_TOKEN_TABLE_INITIALIZED);

    for (unsigned i = 0; i < QUICK_TOKEN_TABLE_SIZE; i++)
        QUICK_TOKEN_TABLE[i].type = Token::INVALID;

#define QUICKTOK_INIT_(tok, chr) SetQuickTokenTableEntry(chr, Token::tok);
    WHISPER_DEFN_QUICK_TOKENS(QUICKTOK_INIT_)
#undef QUICKTOK_INIT_

    QUICK_TOKEN_TABLE_INITIALIZED = true;
}

static inline Token::Type
LookupQuickToken(unic_t ch)
{
    WH_ASSERT(QUICK_TOKEN_TABLE_INITIALIZED);
    WH_ASSERT(ch >= 0 && ch <= static_cast<unic_t>(QUICK_TOKEN_TABLE_SIZE));
    return QUICK_TOKEN_TABLE[ch].type;
}


//
// Token implementation.
//

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
// Tokenizer implementation.
//

TokenizerMark
Tokenizer::mark() const
{
    return TokenizerMark(stream_.position(),
                         line_,
                         stream_.cursor() - lineStart_,
                         pushedBackToken_,
                         tok_);
}

void
Tokenizer::gotoMark(const TokenizerMark &mark)
{
    stream_.rewindTo(mark.position());
    line_ = mark.line();
    lineStart_ = stream_.cursor() - mark.lineOffset();
    WH_ASSERT(mark.pushedBackToken() == mark.token().debug_isPushedBack());
    pushedBackToken_ = mark.pushedBackToken();
    tok_ = mark.token();
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
Tokenizer::readToken()
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
        return readTokenImpl();
    } catch (TokenizerError exc) {
        return emitToken(Token::Error);
    }
}

const Token &
Tokenizer::readTokenImpl()
{
    WH_ASSERT(!hasError());

    // Start the next token.
    startToken();

    //
    // First: check for ascii start-of-token chars.
    //

    unic_t ch = readAsciiChar();

    // Whitespace, simple identifiers, numbers, and strings will be very
    // common.  Check for them first.
    if (IsWhitespace(ch))
        return readWhitespace();

    if (IsAsciiIdentifierStart(ch))
        return readKeywordOrAsciiIdentifierOrStringLiteral(ch);

    if (IsDecDigit(ch))
        return readNumericLiteral(ch == '0');

    if (CharOneOf<'\'', '"'>(ch))
        return readStringLiteral(Token::NoFlags, ch);

    if (ch >= 0) {
        Token::Type quickTokenType = LookupQuickToken(ch);
        if (quickTokenType != Token::INVALID)
            return emitToken(quickTokenType);
    }

    // Bare dot and float literals are pretty common.
    if (ch == '.') {
        unic_t ch2 = readAsciiChar();
        if (IsDecDigit(ch2))
            return readFloatAfterDotRest(Token::NoFlags);

        unreadAsciiChar(ch2);
        return emitToken(Token::Dot);
    }

    // Next, check for punctuators, ordered from an intuitive sense
    // of most common to least common.
    if (ch == '+') {
        unic_t ch2 = readAsciiChar();

        if (ch2 == '=')
            return emitToken(Token::PlusEqual);

        unreadAsciiChar(ch2);
        return emitToken(Token::Plus);
    }

    if (ch == '-') {
        unic_t ch2 = readAsciiChar();

        if (ch2 == '=')
            return emitToken(Token::MinusEqual);

        if (ch2 == '>')
            return emitToken(Token::RightArrow);

        unreadAsciiChar(ch2);
        return emitToken(Token::Minus);
    }

    if (ch == '*') {
        unic_t ch2 = readAsciiChar();

        if (ch2 == '=')
            return emitToken(Token::StarEqual);

        if (ch2 == '*') {
            unic_t ch3 = readAsciiChar();

            if (ch3 == '=')
                return emitToken(Token::StarStarEqual);

            unreadAsciiChar(ch3);
            return emitToken(Token::StarStar);
        }

        unreadAsciiChar(ch2);
        return emitToken(Token::Star);
    }

    if (ch == '/') {
        unic_t ch2 = readAsciiChar();

        if (ch2 == '=')
            return emitToken(Token::SlashEqual);

        if (ch2 == '/') {
            unic_t ch3 = readAsciiChar();

            if (ch3 == '=')
                return emitToken(Token::SlashSlashEqual);

            unreadAsciiChar(ch3);
            return emitToken(Token::SlashSlash);
        }

        unreadAsciiChar(ch2);
        return emitToken(Token::Slash);
    }

    if (ch == '%') {
        unic_t ch2 = readAsciiChar();

        if (ch2 == '=')
            return emitToken(Token::PercentEqual);

        unreadAsciiChar(ch2);
        return emitToken(Token::Percent);
    }

    if (ch == '<') {
        unic_t ch2 = readAsciiChar();

        if (ch2 == '=')
            return emitToken(Token::LessEqual);

        if (ch2 == '<') {
            unic_t ch3 = readAsciiChar();

            if (ch3 == '=')
                return emitToken(Token::ShiftLeftEqual);

            unreadChar(ch3);
            return emitToken(Token::ShiftLeft);
        }

        unreadAsciiChar(ch2);
        return emitToken(Token::Less);
    }

    if (ch == '>') {
        unic_t ch2 = readAsciiChar();

        if (ch2 == '=')
            return emitToken(Token::GreaterEqual);

        if (ch2 == '>') {
            unic_t ch3 = readAsciiChar();

            if (ch3 == '=')
                return emitToken(Token::ShiftRightEqual);

            unreadChar(ch3);
            return emitToken(Token::ShiftRight);
        }

        unreadAsciiChar(ch2);
        return emitToken(Token::Greater);
    }

    if (ch == '=') {
        unic_t ch2 = readAsciiChar();

        if (ch2 == '=')
            return emitToken(Token::EqualEqual);

        unreadAsciiChar(ch2);
        return emitToken(Token::Equal);
    }

    if (ch == '!') {
        unic_t ch2 = readAsciiChar();

        if (ch2 == '=')
            return emitToken(Token::BangEqual);

        unreadAsciiChar(ch2);
        emitError("Unrecognized character after '!'.");
    }

    if (ch == '&') {
        unic_t ch2 = readAsciiChar();

        if (ch2 == '=')
            return emitToken(Token::AmpersandEqual);

        unreadAsciiChar(ch2);
        return emitToken(Token::Ampersand);
    }

    if (ch == '|') {
        unic_t ch2 = readAsciiChar();

        if (ch2 == '=')
            return emitToken(Token::PipeEqual);

        unreadAsciiChar(ch2);
        return emitToken(Token::Pipe);
    }

    if (ch == '^') {
        unic_t ch2 = readAsciiChar();

        if (ch2 == '=')
            return emitToken(Token::CaratEqual);

        unreadAsciiChar(ch2);
        return emitToken(Token::Carat);
    }

    if (ch == '~')
        return emitToken(Token::Tilde);

    if (ch == '#')
        return readComment();

    // All brackets should have been handled by the quick token table.
    WH_ASSERT((!CharOneOf<'(',')','[',']','{','}',',',':',';','@'>(ch)));

    // Line terminators.
    if (IsNewline(ch)) {
        finishNewline(ch);
        return emitToken(Token::Newline);
    }

    if (ch == '\\')
        return emitToken(Token::Backslash);

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
Tokenizer::readComment()
{
    for (;;) {
        unic_t ch = readChar();

        if (IsNewline(ch)) {
            unreadChar(ch);
            break;
        }

        if (ch == End)
            break;
    }
    return emitToken(Token::Comment);
}

const Token &
Tokenizer::readKeywordOrAsciiIdentifierOrStringLiteral(unic_t firstChar)
{
    // Check for string literal prefixes.
    if (CharOneOf<'r','R'>(firstChar)) {
        unic_t ch2 = readAsciiChar();

        if (IsStringQuote(ch2))
            return readStringLiteral(Token::Str_Raw, ch2);

        if (CharOneOf<'b','B'>(ch2)) {
            unic_t ch3 = readAsciiChar();

            if (IsStringQuote(ch3))
                return readStringLiteral(Token::Str_RawBytes, ch3);

            unreadAsciiChar(ch3);
        }

        unreadAsciiChar(ch2);
    } else if (CharOneOf<'u', 'U'>(firstChar)) {
        unic_t ch2 = readAsciiChar();

        if (IsStringQuote(ch2))
            return readStringLiteral(Token::Str_Unicode, ch2);

        unreadChar(ch2);
    } else if (CharOneOf<'b','B'>(firstChar)) {
        unic_t ch2 = readAsciiChar();

        if (IsStringQuote(ch2))
            return readStringLiteral(Token::Str_Bytes, ch2);

        if (CharOneOf<'r','R'>(ch2)) {
            unic_t ch3 = readAsciiChar();

            if (IsStringQuote(ch3))
                return readStringLiteral(Token::Str_RawBytes, ch3);

            unreadAsciiChar(ch3);
        }

        unreadAsciiChar(ch2);
    }

    // Short-circuit to faster loop on known non-keyword chars.
    if (!IsKeywordStart(firstChar))
        return readAsciiIdentifierRest();

    uint32_t lastBytesPacked = firstChar;
    for (;;) {
        unic_t ch = readAsciiChar();

        if (IsKeywordContinue(ch)) {
            lastBytesPacked <<= 8;
            lastBytesPacked |= ch;
            continue;
        }

        // Short-circuit to faster loop on known non-keyword chars.
        if (IsAsciiIdentifierContinue(ch))
            return readAsciiIdentifierRest();

        // In any case, we will be un-reading the ascii char.
        unreadAsciiChar(ch);

        // Any other ascii char (and End) signals end-of-identifier.
        if (ch != NonAscii)
            break;

        // Got NonAscii, re-read a full char and check for non-ascii
        // identifier chars.
        ch = readChar();
        if (IsNonAsciiIdentifierContinue(ch))
            return readNonAsciiIdentifierRest();

        // No matching chars.  Identifier ended.
        unreadChar(ch);
        break;
    }

    Token::Type kwType = CheckKeywordTable(tokStart_, stream_.cursor() - tokStart_,
                                           lastBytesPacked);
    if (kwType != Token::INVALID)
        return emitToken(kwType);

    return emitToken(Token::Identifier);
}

const Token &
Tokenizer::readAsciiIdentifierRest()
{
    for (;;) {
        unic_t ch = readAsciiChar();

        if (IsAsciiIdentifierContinue(ch))
            continue;

        unreadAsciiChar(ch);

        if (ch == NonAscii)
            return readNonAsciiIdentifierRest();

        break;
    }

    return emitToken(Token::Identifier);
}

const Token &
Tokenizer::readNonAsciiIdentifierRest()
{
    for (;;) {
        unic_t ch = readChar();
        if (IsAsciiIdentifierContinue(ch) || IsNonAsciiIdentifierContinue(ch))
            continue;

        unreadChar(ch);
        break;
    }

    return emitToken(Token::Identifier);
}

const Token &
Tokenizer::readNumericLiteral(bool startsWithZero)
{
    unic_t ch = readAsciiChar();

    // Check for hex vs. decimal literal.
    if (startsWithZero) {
        if (CharOneOf<'x','X'>(ch))
            return readHexIntegerLiteral();
        if (CharOneOf<'b','B'>(ch))
            return readBinIntegerLiteral();
        if (CharOneOf<'o','O'>(ch))
            return readOctIntegerLiteral();

        // Skip any more leading '0's
        while (ch == '0')
            ch = readAsciiChar();

        if (IsDecDigit(ch))
            emitError("Digit following 0 in decimal literal.");
    }

    if (IsDecDigit(ch))
        return readDecIntegerLiteralRest();

    // Check for fraction.
    if (ch == '.')
        return readFloatAfterDot();

    // Check for exponent.
    if (CharOneOf<'e','E'>(ch))
        return readFloatExponent(Token::NoFlags);

    if (CharOneOf<'j','J'>(ch)) {
        if (!checkFinishNumericLiteralAscii())
            emitError("Identifier continue after imaginary literal.");

        return emitToken(Token::ImaginaryIntegerLiteral);
    }

    unreadAsciiChar(ch);
    if (ch == NonAscii) {
        if (!checkFinishNumericLiteralNonAscii())
            emitError("Identifier continue after integer literal.");
    }

    return emitToken(Token::IntegerLiteral);
}

const Token &
Tokenizer::readDecIntegerLiteralRest()
{
    // Otherwise, keep reading chars.
    for (;;) {
        unic_t ch = readAsciiChar();

        if (IsDecDigit(ch))
            continue;

        // Check for fraction.
        if (ch == '.')
            return readFloatAfterDot();

        // Check for exponent.
        if (CharOneOf<'e','E'>(ch))
            return readFloatExponent(Token::NoFlags);

        // Check for imaginary suffix.
        if (CharOneOf<'j','J'>(ch))  {
            if (!checkFinishNumericLiteralAscii())
                emitError("Identifier continue after imaginary literal.");

            return emitToken(Token::ImaginaryIntegerLiteral);
        }

        // Ensure literal isn't followed by identifier continue chars.
        unreadAsciiChar(ch);
        if (ch == NonAscii) {
            if (!checkFinishNumericLiteralNonAscii())
                emitError("Identifier continue after integer literal.");
        }

        break;
    }

    return emitToken(Token::IntegerLiteral);
}

const Token &
Tokenizer::readHexIntegerLiteral()
{
    // Must read at least one valid digit char.
    unic_t ch = readAsciiChar();
    if (!IsHexDigit(ch))
        emitError("Starting hex digit not present in hex literal.");

    for (;;) {
        ch = readAsciiChar();

        if (IsHexDigit(ch))
            continue;

        // Ensure literal isn't followed by identifier continue chars.
        unreadAsciiChar(ch);
        if (ch == NonAscii) {
            if (!checkFinishNumericLiteralNonAscii())
                emitError("Identifier continue after integer literal.");
        }

        break;
    }

    return emitToken(Token::IntegerLiteral, Token::Int_HexPrefix);
}

const Token &
Tokenizer::readOctIntegerLiteral()
{
    // Must read at least one valid digit char.
    unic_t ch = readAsciiChar();
    if (!IsOctDigit(ch))
        emitError("Starting oct digit not present in oct literal.");

    for (;;) {
        unic_t ch = readAsciiChar();

        if (IsOctDigit(ch))
            continue;

        if (IsDecDigit(ch))
            emitError("Oct literal followed by decimal digit.");

        // Ensure literal isn't followed by identifier continue chars.
        unreadAsciiChar(ch);
        if (ch == NonAscii) {
            if (!checkFinishNumericLiteralNonAscii())
                emitError("Identifier continue after integer literal.");
        }

        break;
    }

    return emitToken(Token::IntegerLiteral, Token::Int_OctPrefix);
}

const Token &
Tokenizer::readBinIntegerLiteral()
{
    // Must read at least one valid digit char.
    unic_t ch = readAsciiChar();
    if (!IsBinDigit(ch))
        emitError("Starting bin digit not present in bin literal.");

    for (;;) {
        unic_t ch = readAsciiChar();

        if (IsBinDigit(ch))
            continue;

        if (IsDecDigit(ch))
            emitError("Bin literal followed by decimal digit.");

        unreadAsciiChar(ch);
        if (ch == NonAscii) {
            if (!checkFinishNumericLiteralNonAscii())
                emitError("Identifier continue after integer literal.");
        }

        break;
    }

    return emitToken(Token::IntegerLiteral, Token::Int_BinPrefix);
}

const Token &
Tokenizer::readFloatAfterDot()
{
    unic_t ch = readAsciiChar();

    const Token::Flags flags = Token::Flags(Token::Float_HasLeft | Token::Float_HasDot);

    if (CharOneOf<'e','E'>(ch))
        return readFloatExponent(flags);

    if (CharOneOf<'j','J'>(ch)) {
        // Bare float imaginary in "123.j" style.
        if (!checkFinishNumericLiteralAscii())
            emitError("Identifier continue after imaginary literal.");

        return emitToken(Token::ImaginaryFloatLiteral, flags);
    }

    if (IsDecDigit(ch))
        return readFloatAfterDotRest(flags);

    unreadAsciiChar(ch);
    if (ch == NonAscii) {
        if (!checkFinishNumericLiteralNonAscii())
            emitError("Identifier continue after integer literal.");
    }

    // Bare float in "123." style.
    return emitToken(Token::FloatLiteral, flags);
}

const Token &
Tokenizer::readFloatAfterDotRest(Token::Flags flags)
{
    flags = Token::Flags(flags | Token::Float_HasRight);

    for (;;) {
        unic_t ch = readAsciiChar();

        if (IsDecDigit(ch))
            continue;

        if (CharOneOf<'e','E'>(ch))
            return readFloatExponent(flags);

        if (CharOneOf<'j','J'>(ch)) {
            // Bare float imaginary in "123.j" style.
            if (!checkFinishNumericLiteralAscii())
                emitError("Identifier continue after imaginary literal.");

            return emitToken(Token::ImaginaryFloatLiteral, flags);
        }

        unreadAsciiChar(ch);
        if (ch == NonAscii) {
            if (!checkFinishNumericLiteralNonAscii())
                emitError("Identifier continue after integer literal.");
        }

        break;
    }

    // Simple float in "123.456" style.
    return emitToken(Token::FloatLiteral, flags);
}

const Token &
Tokenizer::readFloatExponent(Token::Flags flags)
{
    // Check for sign.
    unic_t ch = readAsciiNonEndChar();

    flags = Token::Flags(flags | Token::Float_HasExp);

    if (IsDecDigit(ch))
        return readFloatExponentRest(flags);

    if (CharOneOf<'+','-'>(ch)) {
        unic_t ch2 = readAsciiNonEndChar();
        if (!IsDecDigit(ch2))
            emitError("Invalid character in float exponent.");

        flags = Token::Flags(flags | Token::Float_HasExpSign);
        return readFloatExponentRest(flags);
    }

    // No other accepted chars after 'e' in exponent.
    return emitError("Invalid character in float exponent.");
}

const Token &
Tokenizer::readFloatExponentRest(Token::Flags flags)
{
    for (;;) {
        unic_t ch = readAsciiChar();

        if (IsDecDigit(ch))
            continue;

        if (CharOneOf<'j','J'>(ch)) {
            // Exponential float imaginary e.g. "2.5E-10j".
            if (!checkFinishNumericLiteralAscii())
                emitError("Identifier continue after imaginary literal.");

            return emitToken(Token::ImaginaryFloatLiteral, flags);
        }

        unreadAsciiChar(ch);
        if (ch == NonAscii) {
            if (!checkFinishNumericLiteralNonAscii())
                emitError("Identifier continue after float literal.");
        }

        break;
    }

    return emitToken(Token::FloatLiteral, flags);
}

bool
Tokenizer::checkFinishNumericLiteralAscii()
{
    unic_t ch = readAsciiChar();

    if (IsAsciiIdentifierContinue(ch))
        return false;

    unreadAsciiChar(ch);
    if (ch != NonAscii)
        return true;

    return checkFinishNumericLiteralNonAscii();
}

bool
Tokenizer::checkFinishNumericLiteralNonAscii()
{
    unic_t ch = readChar();
    WH_ASSERT(ch > 0x7f);

    if (IsNonAsciiIdentifierContinue(ch))
        return false;

    unreadChar(ch);
    return true;
}

const Token &
Tokenizer::readStringLiteral(Token::Flags flags, unic_t quoteChar)
{
    unic_t ch = readAsciiNonEndChar();
    if (ch == quoteChar) {
        unic_t ch2 = readAsciiChar();

        // Check for triple-quoted string.
        if (ch2 == quoteChar)
            return readMultiStringLiteral(flags, quoteChar);

        // Nope, just two bare quotes.  Emit empty string literal.
        unreadAsciiChar(ch2);
        if (quoteChar == '"')
            flags = Token::Flags(flags | Token::Str_DoubleQuote);

        return emitToken(Token::StringLiteral, flags);
    }

    return readShortStringLiteralRest(flags, quoteChar, ch);
}

const Token &
Tokenizer::readMultiStringLiteral(Token::Flags flags, unic_t quoteChar)
{
    flags = Token::Flags(flags | Token::Str_Multi);

    for (;;) {
        unic_t ch = readAsciiNonEndChar();

        if (ch == quoteChar) {
            unic_t ch2 = readAsciiChar();
            if (ch2 == quoteChar) {
                unic_t ch3 = readAsciiChar();
                if (ch3 == quoteChar)
                    return emitToken(Token::StringLiteral, flags);

                unreadAsciiChar(ch3);
                continue;
            }

            unreadAsciiChar(ch2);
            continue;
        }

        if (ch == '\\') {
            if (flags & Token::Str_Raw)
                consumeStringLiteralEscapeRaw();
            if (flags & Token::Str_Bytes)
                consumeStringLiteralEscapeBytes();
            else
                consumeStringLiteralEscape();
        }

        if (IsNewline(ch)) {
            finishNewline(ch);
            continue;
        }

        if (ch != NonAscii)
            continue;

        unreadAsciiChar(ch);
        ch = readChar();
    }
}

const Token &
Tokenizer::readShortStringLiteralRest(Token::Flags flags,
                                      unic_t quoteChar,
                                      unic_t ch)
{
    WH_ASSERT(IsAscii(ch) || ch == NonAscii);
    do {
        if (IsNewline(ch))
            emitError("Newline in short string.");

        if (ch == '\\') {
            if (flags & Token::Str_Raw)
                consumeStringLiteralEscapeRaw();
            if (flags & Token::Str_Bytes)
                consumeStringLiteralEscapeBytes();
            else
                consumeStringLiteralEscape();
        }

        if (ch == quoteChar)
            break;

        if (ch == NonAscii) {
            unreadAsciiChar(ch);
            ch = readChar();
            continue;
        }

        if (ch == End)
            emitError("End-of-input in string literal.");

        ch = readAsciiChar();

    } while(true);

    return emitToken(Token::StringLiteral, flags);
}

void
Tokenizer::consumeStringLiteralEscapeRaw()
{
    // consume next character unconditionally.
    unic_t ch = readChar();

    if (IsNewline(ch)) {
        finishNewline(ch);
        return;
    }

    if (ch == End)
        emitError("End-of-input in string literal.");

    return;
}

void
Tokenizer::consumeStringLiteralEscapeBytes()
{
    // consume next character unconditionally.
    unic_t ch = readAsciiChar();

    if (CharOneOf<'n','t','\'','"','r','a','b','v','f'>(ch))
        return;

    if (ch == 'x') {
        consumeStringLiteralHexEscape();
        return;
    }

    if (IsNewline(ch)) {
        finishNewline(ch);
        return;
    }

    if (IsOctDigit(ch)) {
        consumeStringLiteralOctEscape();
        return;
    }

    if (ch == End)
        emitError("End-of-input in string literal.");

    if (ch == NonAscii) {
        unreadAsciiChar(ch);
        ch = readChar();
    }

    return;
}

void
Tokenizer::consumeStringLiteralEscape()
{
    // consume next character unconditionally.
    unic_t ch = readAsciiChar();

    if (CharOneOf<'n','t','\'','"','r','a','b','v','f'>(ch))
        return;

    if (ch == 'u') {
        for (unsigned i = 0; i < 4; i++) {
            unic_t chDigit = readAsciiChar();
            if (!IsHexDigit(chDigit))
                emitError("Expected hex digit in unicode16 escape.");
        }
        return;
    }

    if (ch == 'U') {
        for (unsigned i = 0; i < 8; i++) {
            unic_t chDigit = readAsciiChar();
            if (!IsHexDigit(chDigit))
                emitError("Expected hex digit in unicode32 escape.");
        }
        return;
    }

    if (ch == 'x') {
        consumeStringLiteralHexEscape();
        return;
    }

    if (IsNewline(ch)) {
        finishNewline(ch);
        return;
    }

    if (IsOctDigit(ch)) {
        consumeStringLiteralOctEscape();
        return;
    }

    if (ch == 'N') {
        unic_t chBrace = readAsciiChar();
        if (chBrace != '{')
            emitError("Expected '{' after unicode-name escape.");

        for (;;) {
            unic_t chName = readChar();
            if (chName == '}')
                break;

            if (IsNewline(chName) || chName == End)
                emitError("Incomplete unicode-name escape.");
        }

        return;
    }

    if (ch == End)
        emitError("End-of-input in string literal.");

    if (ch == NonAscii) {
        unreadAsciiChar(ch);
        ch = readChar();
    }

    return;

}

void
Tokenizer::consumeStringLiteralHexEscape()
{
    for (unsigned i = 0; i < 2; i++) {
        unic_t chDigit = readAsciiChar();
        if (!IsHexDigit(chDigit))
            emitError("Expected hex digit in hex8 escape.");
    }

    return;
}

void
Tokenizer::consumeStringLiteralOctEscape()
{
    for (unsigned i = 0; i < 2; i++) {
        unic_t chDigit = readAsciiChar();
        if (!IsOctDigit(chDigit)) {
            unreadAsciiChar(chDigit);
            break;
        }
    }
    return;
}

const Token &
Tokenizer::emitToken(Token::Type type, Token::Flags flags)
{
    WH_ASSERT(Token::IsValidType(type));

    // Previously emitted token must have been used.
    WH_ASSERT(tok_.debug_isUsed());

    tok_ = Token(type, flags,
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
    SpewParserError("%s", msg);
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

        return ((firstByte & 0x0F)  << 12) |
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

        return ((firstByte & 0x07)  << 18) |
               ((secondByte & 0x3F) << 12) |
               ((thirdByte & 0x3F)  << 6) |
               ((fourthByte & 0x3F) << 0);
    }

    // firstByte >= 1111-1100
    emitError("Invalid unicode character: firstByte > 0xFB.");
    WH_UNREACHABLE("Should have thrown before this.");
    return NonAscii;
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
Tokenizer::IsNonAsciiIdentifierStart(unic_t ch)
{
    WH_ASSERT(!IsAsciiIdentifierStart(ch));
    return uc_is_property_xid_start(ch);
}

/* static */ bool
Tokenizer::IsNonAsciiIdentifierContinue(unic_t ch)
{
    WH_ASSERT(!IsAsciiIdentifierContinue(ch));
    return uc_is_property_xid_continue(ch);
}


} // namespace Whisper
