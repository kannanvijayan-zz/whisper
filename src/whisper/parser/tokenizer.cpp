
#include <unictype.h>
#include <string.h>
#include <algorithm>

#include "spew.hpp"
#include "parser/tokenizer.hpp"


namespace Whisper {


//
// KeywordTable
//
// Keeps an ordered table of keywords which can be used to do quick
// lookups of identifiers which may be keywords.
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
// lengths.  Max keyword length is 10.  There are 12 entries in the
// array to allow for a "zero" entry for length 0, and
// "final" entry that points to the end of the keyword list.
static constexpr unsigned KEYWORD_MAX_LENGTH = 10;
static unsigned KEYWORD_TABLE_SECTIONS[KEYWORD_MAX_LENGTH+2];

static uint32_t
ComputeLastBytesPacked(const char *str, uint8_t length)
{
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

static void
InitializeKeywordTable()
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
    KEYWORD_TABLE_SECTIONS[0] = 0;
    unsigned offset = 0;
    for (unsigned i = 1; i <= KEYWORD_MAX_LENGTH; i++)
        offset = InitializeKeywordSection(i, offset);

    KEYWORD_TABLE_SECTIONS[KEYWORD_MAX_LENGTH+1] = KEYWORD_TABLE_SIZE;
    KEYWORD_TABLE_INITIALIZED = true;
}

Token::Type
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

    // Check for keyword by scanning keyword table by length.
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
// QuickTokenTable
//
// Maps ascii characters to immediately-returnable tokens.
//
struct QuickTokenEntry
{
    Token::Type type;
};


static constexpr unsigned QUICK_TOKEN_TABLE_SIZE = 128;
static QuickTokenEntry QUICK_TOKEN_TABLE[QUICK_TOKEN_TABLE_SIZE];

static inline void
SetQuickTokenTableEntry(char ch, Token::Type tokType)
{
    uint8_t idx = ch;
    WH_ASSERT(idx < 128);
    QUICK_TOKEN_TABLE[idx].type = tokType;
}

static void
InitializeQuickTokenTable()
{
    for (unsigned i = 0; i < QUICK_TOKEN_TABLE_SIZE; i++)
        QUICK_TOKEN_TABLE[i].type = Token::INVALID;

#define QUICKTOK_INIT_(tok, chr) SetQuickTokenTableEntry(chr, Token::tok);
    WHISPER_DEFN_QUICK_TOKENS(QUICKTOK_INIT_)
#undef QUICKTOK_INIT_
}

static inline Token::Type
LookupQuickToken(unic_t ch)
{
    if (ch >= 0 && ch <= static_cast<unic_t>(QUICK_TOKEN_TABLE_SIZE))
        return QUICK_TOKEN_TABLE[ch].type;
    return Token::INVALID;
}


//
// InitializeTokenizer
//
void
InitializeTokenizer()
{
    InitializeKeywordTable();
    InitializeQuickTokenTable();
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
    return TokenizerMark(reader_.position(),
                         line_,
                         reader_.cursor() - lineStart_,
                         pushedBackToken_,
                         tok_);
}

void
Tokenizer::gotoMark(const TokenizerMark &mark)
{
    reader_.rewindTo(mark.position());
    line_ = mark.line();
    lineStart_ = reader_.cursor() - mark.lineOffset();
    WH_ASSERT(mark.pushedBackToken() == mark.token().debug_isPushedBack());
    pushedBackToken_ = mark.pushedBackToken();
    tok_ = mark.token();
}

Token
Tokenizer::getAutomaticSemicolon() const
{
    uint32_t lineOffset = reader_.cursor() - lineStart_;
    return Token(Token::Semicolon, reader_.position(), 0,
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

    unic_t ch = readAsciiChar();
    Token::Type quickTokenType = LookupQuickToken(ch);
    if (quickTokenType != Token::INVALID)
        return emitToken(quickTokenType);

    // Whitespace, simple identifiers, numbers, and strings will be very
    // common.  Check for them first.
    if (IsWhitespace(ch))
        return readWhitespace();

    if (IsKeywordChar(ch))
        return readIdentifier(ch);

    if (IsNonKeywordSimpleIdentifierStart(ch))
        return readIdentifierName();

    if (IsDecDigit(ch))
        return readNumericLiteral(ch == '0');

    if (ch == '.')
        return emitToken(Token::Dot);

    if (ch == '/') {
        // Check for '/', '//' or '/*'
        unic_t ch2 = readAsciiChar();
        if (ch2 == '/')
            return readSingleLineComment();

        if (ch2 == '*')
            return readMultiLineComment();

        unreadAsciiChar(ch2);
        return emitToken(Token::Slash);
    }

    if (ch == '-') {
        // Check for '-' or '->'
        unic_t ch2 = readAsciiChar();
        if (ch2 == '>')
            return emitToken(Token::Arrow);

        unreadAsciiChar(ch2);
        return emitToken(Token::Minus);
    }

    if (ch == '+')
        return emitToken(Token::Plus);

    if (ch == '*')
        return emitToken(Token::Star);

    if (ch == '=')
        return emitToken(Token::Equal);

    // Line terminators are probably more common the the following
    // three punctuators.
    if (IsAsciiLineTerminator(ch))
        return readLineTerminatorSequence(ch);

    if (ch == '\\') {
        consumeUnicodeEscapeSequence();
        return readIdentifierName();
    }

    WH_ASSERT(!(CharIn<'(', ')', '{', '}', ',', ';', '.'>(ch)));
    WH_ASSERT(!(CharIn<'+', '-', '/', '*', '='>(ch)));

    // Handle unicode escapes and complex identifiers last.
    ch = maybeRereadNonAsciiToFull(ch);

    if (IsNonAsciiLineTerminator(ch)) {
        startNewLine();
        return emitToken(Token::LineTerminatorSequence);
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
    reader_.rewindTo(tok.offset());
    line_ = tok.startLine();
    lineStart_ = reader_.cursor() - tok.startLineOffset();

    // Rewinding to a token discards any pushed-back tokens.
    pushedBackToken_ = false;
}

void
Tokenizer::advancePastToken(const Token &tok)
{
    // Find the stream position to advance to.
    reader_.advanceTo(tok.endOffset());
    line_ = tok.endLine();
    lineStart_ = reader_.cursor() - tok.endLineOffset();

    // Advancing past a token discards any pushed-back tokens.
    pushedBackToken_ = false;
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
Tokenizer::readIdentifierName()
{
    for (;;) {
        unic_t ch = readAsciiChar();

        // Common case: simple identifier continuation.
        if (IsSimpleIdentifierContinue(ch))
            continue;

        if (ch == '\\') {
            consumeUnicodeEscapeSequence();
            continue;
        }

        if (IsDecDigit(ch))
            emitError("Digit immediately follows identifier.");

        // Any other ASCII char means end of identifier.
        // Check this first because it's more likely than
        // a complex identifier continue char.
        if (IsAscii(ch)) {
            unreadChar(ch);
            break;
        }

        // May be End or NonAscii here.  If NonAcii, read a full char
        // before continuing.
        ch = maybeRereadNonAsciiToFull(ch);

        // Check for complex identifier continue char.
        if (IsComplexIdentifierContinue(ch))
            continue;

        // Any other char means end of identifier
        unreadChar(ch);
        break;
    }
    return emitToken(Token::Identifier);
}

const Token &
Tokenizer::readIdentifier(unic_t firstChar)
{
    WH_ASSERT(IsKeywordChar(firstChar));
    uint32_t lastBytesPacked = firstChar;

    for (;;) {
        unic_t ch = readAsciiChar();

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

        // Digit immediately following identifier is not allowed.
        if (IsDecDigit(ch))
            emitError("Digit immediately follows identifier.");

        // Any other ASCII char means end of identifier.
        // Check this first because it's more likely than
        // a complex identifier continue char.
        if (IsAscii(ch)) {
            WH_ASSERT(!IsComplexIdentifierContinue(ch));
            unreadChar(ch);
            break;
        }

        // May be End or NonAscii here.  If NonAcii, read a full char
        // before continuing.
        ch = maybeRereadNonAsciiToFull(ch);

        // Check for complex identifier continue char.
        // If found, it cannot be a keyword.
        if (IsComplexIdentifierContinue(ch))
            return readIdentifierName();

        // Any other char means end of identifier
        unreadChar(ch);
        break;
    }

    unsigned tokenLength = reader_.cursor() - tokStart_;
    Token::Type kwType = CheckKeywordTable(tokStart_, tokenLength,
                                           lastBytesPacked);
    if (kwType == Token::INVALID)
        return emitToken(Token::Identifier);

    return emitToken(kwType);
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
    unic_t ch = readAsciiChar();

    // Check for hex vs. decimal literal.
    if (startsWithZero) {
        if (CharIn<'x'>(ch))
            return readHexIntegerLiteral();
        if (CharIn<'b'>(ch))
            return readBinIntegerLiteral();
        if (CharIn<'o'>(ch))
            return readOctIntegerLiteral();
        if (CharIn<'d'>(ch))
            return readOctIntegerLiteral();
    }

    // Check for fraction.
    if (ch == '.')
        return emitError("Cannot parse floats yet.");

    // Check for exponent.
    if (CharIn<'e','E'>(ch))
        return emitError("Cannot parse floats yet.");

    // If digit, check constraints.
    if (IsDecDigit(ch)) {
        // Zero followed by another digit is a not valid.
        if (startsWithZero)
            return emitError("Digit following 0 in decimal literal.");
    } else if (ch != '_') {
        // Otherwise check for non-digit char.
        if (IsSimpleIdentifierContinue(ch)) {
            return emitError("Identifier starts immediately after "
                             "numeric literal.");
        }

        // Check for non-ascii identifier continues.
        ch = maybeRereadNonAsciiToFull(ch);
        if (IsComplexIdentifierContinue(ch)) {
            return emitError("Identifier starts immediately after "
                             "numeric literal.");
        }
        unreadChar(ch);
        return emitToken(Token::IntegerLiteral);
    }

    bool allowUnderscore = (ch != '_');

    // Otherwise, keep reading chars.
    for (;;) {
        ch = readAsciiChar();

        if (IsDecDigit(ch)) {
            allowUnderscore = true;
            continue;
        }

        if (ch == '_') {
            if (!allowUnderscore) {
                return emitError("Multiple sequential undescores not allowed "
                                 "in integer literals.");
            }
            allowUnderscore = false;
            continue;
        }

        // Check for fraction.
        if (ch == '.')
            return emitError("Cannot parse floats yet.");

        // Check for exponent.
        if (CharIn<'e','E'>(ch))
            return emitError("Cannot parse floats yet.");

        if (IsSimpleIdentifierContinue(ch)) {
            return emitError("Identifier starts immediately after "
                             "numeric literal.");
        }

        // Any other ascii character means end of integer literal.
        if (IsAscii(ch)) {
            unreadChar(ch);
            return emitToken(Token::IntegerLiteral);
        }

        ch = maybeRereadNonAsciiToFull(ch);

        if (IsComplexIdentifierContinue(ch)) {
            return emitError("Identifier starts immediately after "
                             "numeric literal.");
        }

        unreadChar(ch);
        break;
    }

    // If allowUnderscore is false, it means integer ended in an
    // underscore.
    if (!allowUnderscore)
        return emitError("Integer literal not allowed to end in underscore.");

    return emitToken(Token::IntegerLiteral);
}

const Token &
Tokenizer::readHexIntegerLiteral()
{
    // Must read at least one valid digit char.
    unic_t ch = readAsciiChar();
    if (!IsHexDigit(ch))
        return emitError("Starting hex digit not present in hex literal.");

    bool allowUnderscore = true;
    for (;;) {
        unic_t ch = readAsciiChar();

        if (IsHexDigit(ch))
            continue;

        if (ch == '_') {
            if (!allowUnderscore) {
                return emitError("Multiple sequential undescores not allowed "
                                 "in integer literals.");
            }
            allowUnderscore = false;
            continue;
        }

        if (IsSimpleIdentifierContinue(ch)) {
            return emitError("Identifier starts immediately after "
                             "numeric literal.");
        }

        // Any other ascii character means end of integer literal.
        if (IsAscii(ch)) {
            unreadChar(ch);
            return emitToken(Token::IntegerLiteral, Token::Int_HexPrefix);
        }

        ch = maybeRereadNonAsciiToFull(ch);

        if (IsComplexIdentifierContinue(ch)) {
            return emitError("Identifier starts immediately after "
                             "numeric literal.");
        }

        unreadChar(ch);
        break;
    }
    return emitToken(Token::IntegerLiteral, Token::Int_HexPrefix);
}

const Token &
Tokenizer::readDecIntegerLiteral()
{
    // Must read at least one valid digit char.
    unic_t ch = readAsciiChar();
    if (!IsDecDigit(ch))
        return emitError("Starting dec digit not present in dec literal.");

    bool allowUnderscore = true;
    for (;;) {
        unic_t ch = readAsciiChar();

        if (IsDecDigit(ch))
            continue;

        if (ch == '_') {
            if (!allowUnderscore) {
                return emitError("Multiple sequential undescores not allowed "
                                 "in integer literals.");
            }
            allowUnderscore = false;
            continue;
        }

        if (IsSimpleIdentifierContinue(ch)) {
            return emitError("Identifier starts immediately after "
                             "numeric literal.");
        }

        // Any other ascii character means end of integer literal.
        if (IsAscii(ch)) {
            unreadChar(ch);
            return emitToken(Token::IntegerLiteral, Token::Int_DecPrefix);
        }

        ch = maybeRereadNonAsciiToFull(ch);

        if (IsComplexIdentifierContinue(ch)) {
            return emitError("Identifier starts immediately after "
                             "numeric literal.");
        }

        unreadChar(ch);
        break;
    }
    return emitToken(Token::IntegerLiteral, Token::Int_DecPrefix);
}

const Token &
Tokenizer::readOctIntegerLiteral()
{
    // Must read at least one valid digit char.
    unic_t ch = readAsciiChar();
    if (!IsOctDigit(ch))
        return emitError("Starting oct digit not present in oct literal.");

    bool allowUnderscore = true;
    for (;;) {
        unic_t ch = readAsciiChar();

        if (IsOctDigit(ch))
            continue;

        if (ch == '_') {
            if (!allowUnderscore) {
                return emitError("Multiple sequential undescores not allowed "
                                 "in integer literals.");
            }
            allowUnderscore = false;
            continue;
        }

        if (IsDecDigit(ch))
            return emitError("Oct literal followed by decimal digit.");

        if (IsSimpleIdentifierContinue(ch)) {
            return emitError("Identifier starts immediately after "
                             "numeric literal.");
        }

        // Any other ascii character means end of integer literal.
        if (IsAscii(ch)) {
            unreadChar(ch);
            return emitToken(Token::IntegerLiteral, Token::Int_OctPrefix);
        }

        ch = maybeRereadNonAsciiToFull(ch);

        if (IsComplexIdentifierContinue(ch)) {
            return emitError("Identifier starts immediately after "
                             "numeric literal.");
        }

        unreadChar(ch);
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
        return emitError("Starting bin digit not present in bin literal.");

    bool allowUnderscore = true;
    for (;;) {
        unic_t ch = readAsciiChar();

        if (IsBinDigit(ch))
            continue;

        if (ch == '_') {
            if (!allowUnderscore) {
                return emitError("Multiple sequential undescores not allowed "
                                 "in integer literals.");
            }
            allowUnderscore = false;
            continue;
        }

        if (IsDecDigit(ch))
            return emitError("Bin literal followed by decimal digit.");

        if (IsSimpleIdentifierContinue(ch)) {
            return emitError("Identifier starts immediately after "
                             "numeric literal.");
        }

        // Any other ascii character means end of integer literal.
        if (IsAscii(ch)) {
            unreadChar(ch);
            return emitToken(Token::IntegerLiteral, Token::Int_BinPrefix);
        }

        ch = maybeRereadNonAsciiToFull(ch);

        if (IsComplexIdentifierContinue(ch)) {
            return emitError("Identifier starts immediately after "
                             "numeric literal.");
        }

        unreadChar(ch);
        break;
    }
    return emitToken(Token::IntegerLiteral, Token::Int_BinPrefix);
}

const Token &
Tokenizer::emitToken(Token::Type type, Token::Flags flags)
{
    WH_ASSERT(Token::IsValidType(type));

    // Previously emitted token must have been used.
    WH_ASSERT(tok_.debug_isUsed());

    tok_ = Token(type, flags,
                 reader_.positionOf(tokStart_),
                 reader_.cursor() - tokStart_,
                 tokStartLine_, tokStartLineOffset_,
                 line_, reader_.cursor() - lineStart_);
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
    WH_UNREACHABLE("Should have thrown before this.");
    return NonAscii;
}

uint8_t
Tokenizer::readCharNextByte()
{
    if (reader_.atEnd())
        emitError("Incomplete unicode character.");

    // Non-first unicode characters must be in range 1000-0000 to 1011-1111
    uint8_t byte = reader_.readByte();
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
        reader_.rewindBy(2);
        return;
    }

    // Up to 4 + 6 + 6 bits = 16 bits, 3 byte char
    // 8 + 8 bits
    if (ch <= 0xffff) {
        reader_.rewindBy(3);
        return;
    }

    // Up to 3 + 6 + 6 + 6 bits = 21 bits, 4 byte char
    // 5 + 8 + 8 bits
    if (ch <= 0x1fffff) {
        reader_.rewindBy(4);
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


} // namespace Whisper
