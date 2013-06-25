#ifndef WHISPER__PARSER__PARSER_HPP
#define WHISPER__PARSER__PARSER_HPP

#include <list>
#include "common.hpp"
#include "debug.hpp"
#include "parser/code_source.hpp"
#include "parser/token_defn.hpp"
#include "allocators.hpp"

//
// The tokenizer parses a code source into a series of tokens.
//

namespace Whisper {


//
// Token
//
// Represents a token.
//
class Token
{
  public:
    enum Type : uint8_t
    {
        INVALID = 0,
#define DEF_ENUM_(tok)   tok,
        WHISPER_DEFN_TOKENS(DEF_ENUM_)
#undef DEF_ENUM_
        LIMIT
    };

    inline static bool IsValidType(Type type) {
        return (type > INVALID) && (type < LIMIT);
    }

    static const char *TypeString(Type type);

  private:
    Type type_ = INVALID;
    uint32_t offset_ = 0;
    uint32_t length_ = 0;
    uint32_t startLine_ = 0;
    uint32_t startLineOffset_ = 0;
    uint32_t endLine_ = 0;
    uint32_t endLineOffset_ = 0;

  public:
    Token() {}
    Token(Type type, uint32_t offset, uint32_t length,
          uint32_t startLine, uint32_t startLineOffset,
          uint32_t endLine, uint32_t endLineOffset)
      : type_(type), offset_(offset), length_(length),
        startLine_(startLine), startLineOffset_(startLineOffset),
        endLine_(endLine), endLineOffset_(endLineOffset)
    {}

    inline Type type() const {
        return type_;
    }
    inline const char *typeString() const {
        return TypeString(type_);
    }

    inline uint32_t offset() const {
        return offset_;
    }
    inline uint32_t length() const {
        return length_;
    }

    inline uint32_t startLine() const {
        return startLine_;
    }
    inline uint32_t startLineOffset() const {
        return startLineOffset_;
    }

    inline uint32_t endLine() const {
        return endLine_;
    }
    inline uint32_t endLineOffset() const {
        return endLineOffset_;
    }

    // Define type check methods
#define DEF_CHECKER_(tok) \
    inline bool is##tok() const { \
        return type_ == tok; \
    }
    WHISPER_DEFN_TOKENS(DEF_CHECKER_)
#undef DEF_CHECKER_
};


//
// KeywordTable
//
// Keeps an ordered table of keywords which can be used to do quick
// lookups of identifiers which may be keywords.
//
void InitializeKeywordTable();

//
// Tokenizer
//
// Parses tokens from a source stream.
//
class TokenizerError {
  friend class Tokenizer;
  private:
    inline TokenizerError() {}
};

class Tokenizer
{
  private:
    STLBumpAllocator<uint8_t> allocator_;
    CodeSource &source_;
    SourceStream stream_;
    Token tok_;

    // Parsing state.
    uint32_t line_ = 0;
    const uint8_t *lineStart_ = 0;

    // Current token state.
    const uint8_t *tokStart_ = nullptr;
    uint32_t tokStartLine_ = 0;
    uint32_t tokStartLineOffset_ = 0;

    // Error message.
    const char *error_ = nullptr;

    // Flag indicating strict parsing mode.
    bool strict_ = false;

    // Flag indicating that a token has been pushed back.
    bool pushedBack_ = false;

  public:
    Tokenizer(const STLBumpAllocator<uint8_t> &allocator, CodeSource &source)
      : allocator_(allocator),
        source_(source),
        stream_(source_),
        tok_()
    {}

    inline ~Tokenizer() {}

    inline CodeSource &source() const {
        return source_;
    }

    inline uint32_t position() const {
        return stream_.position();
    }

    inline uint32_t line() const {
        return line_;
    }
    inline uint32_t lineOffset() const {
        return stream_.cursor() - lineStart_;
    }

    inline bool hasError() const {
        return error_ != nullptr;
    }
    inline const char *error() const {
        WH_ASSERT(hasError());
        return error_;
    }

    enum InputElementKind {
        InputElement_Div,
        InputElement_RegExp
    };
    const Token &readInputElementImpl(InputElementKind iek,
                                      bool checkKeywords);
    inline const Token &readInputElement(InputElementKind iek,
                                         bool checkKeywords);

    inline bool isStrict() const {
        return strict_;
    }
    inline void setStrict(bool strict) {
        strict_ = strict;
    }

  private:
    const Token &readWhitespace();
    const Token &readLineTerminatorSequence(unic_t ch);
    const Token &readMultiLineComment();
    const Token &readSingleLineComment();
    const Token &readDivPunctuator(unic_t ch);

    const Token &readRegularExpressionLiteral(unic_t ch);
    inline void consumeRegularExpressionBackslashSequence();
    void consumeRegularExpressionCharacterClass();

    const Token &readIdentifierName();
    const Token &readIdentifier(unic_t firstChar);
    void consumeUnicodeEscapeSequence();

    const Token &readNumericLiteral(bool startsWithZero);
    const Token &readNumericLiteralFraction();
    const Token &readNumericLiteralExponent();

    const Token &readHexIntegerLiteral();

    const Token &readStringLiteral(unic_t quoteChar);
    void consumeStringEscapeSequence();

    inline const Token &emitToken(Token::Type type);
    const Token &emitError(const char *msg);

    void rewindToToken(const Token &tok);
    void pushbackImplicitSemicolon();

    static constexpr unic_t End = INT32_MAX;
    static constexpr unic_t Error = -1;

    inline void startToken() {
        tokStart_ = stream_.cursor();
        tokStartLine_ = line_;
        tokStartLineOffset_ = tokStart_ - lineStart_;
    }

    inline void startNewLine() {
        line_++;
        lineStart_ = stream_.cursor();
    }

    inline unic_t readChar() {
        if (stream_.atEnd())
            return End;

        uint8_t b = stream_.readByte();
        if (b <= 0x7Fu)
            return b;

        return readCharSlow(b);
    }
    inline unic_t readNonEndChar() {
        unic_t ch = readChar();
        if (ch == End)
            emitError("Unexpected end of input.");
        return ch;
    }
    unic_t readCharSlow(unic_t firstByte);
    uint8_t readCharNextByte();

    inline void unreadChar(unic_t ch) {
        if (ch <= 0x7f)
            stream_.rewindBy(1);
        else
            slowUnreadChar(ch);
    }
    void slowUnreadChar(unic_t ch);

    inline void finishLineTerminator(unic_t ch) {
        if (ch == '\r') {
            unic_t ch2 = readChar();
            if (ch2 != '\n')
                unreadChar(ch2);
        }
    }


    template <unic_t Char0>
    inline static bool CharIn(unic_t ch) {
        return ch == Char0;
    }

    template <unic_t Char0, unic_t Char1, unic_t... Rest>
    inline static bool CharIn(unic_t ch) {
        return ch == Char0 || CharIn<Char1, Rest...>(ch);
    }

    template <unic_t From, unic_t To>
    inline static bool CharInRange(unic_t ch) {
        return (ch >= From) && (ch <= To);
    }

    inline static bool IsWhitespace(unic_t ch) {
        return CharIn<' ','\t'>(ch) || IsWhitespaceSlow(ch);
    }
    static bool IsWhitespaceSlow(unic_t ch);

    inline static bool IsLineTerminator(unic_t ch) {
        return CharIn<'\r','\n'>(ch) || IsLineTerminatorSlow(ch);
    }
    static bool IsLineTerminatorSlow(unic_t ch);

    inline static bool IsAscii(unic_t ch) {
        return (ch <= 0x7f);
    }
    inline static bool IsAsciiLetter(unic_t ch) {
        return CharInRange<'a','z'>(ch) || CharInRange<'A','Z'>(ch);
    }

    inline static bool IsKeywordChar(unic_t ch) {
        return CharIn<'a','z'>(ch);
    }
    inline static bool IsNonKeywordSimpleIdentifierStart(unic_t ch) {
        WH_ASSERT(!IsKeywordChar(ch));
        return CharIn<'A','Z'>(ch) || CharIn<'$','_'>(ch);
    }
    inline static bool IsSimpleIdentifierStart(unic_t ch) {
        return IsKeywordChar(ch) || IsNonKeywordSimpleIdentifierStart(ch);
    }
    static bool IsComplexIdentifierStart(unic_t ch);

    inline static bool IsNonKeywordSimpleIdentifierContinue(unic_t ch) {
        WH_ASSERT(!IsKeywordChar(ch));
        return CharIn<'A','Z'>(ch) || IsDigit(ch) || CharIn<'$','_'>(ch);
    }
    inline static bool IsSimpleIdentifierContinue(unic_t ch) {
        return IsKeywordChar(ch) || IsNonKeywordSimpleIdentifierContinue(ch);
    }
    inline static bool IsComplexIdentifierContinue(unic_t ch);


    inline static bool IsHexDigit(unic_t ch) {
        return CharInRange<'0', '9'>(ch) ||
               CharInRange<'A', 'F'>(ch) ||
               CharInRange<'a', 'f'>(ch);
    }

    inline static bool IsDigit(unic_t ch) {
        return CharInRange<'0', '9'>(ch);
    }

    inline static bool IsBinaryDigit(unic_t ch) {
        return CharIn<'0', '1'>(ch);
    }
};


} // namespace Whisper

#endif // WHISPER__PARSER__PARSER_HPP
