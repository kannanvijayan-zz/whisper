#ifndef WHISPER__PARSER__PARSER_INLINES_HPP
#define WHISPER__PARSER__PARSER_INLINES_HPP

#include <new>
#include "parser/parser.hpp"

//
// The tokenizer parses a code source into a series of tokens.
//

namespace Whisper {

using namespace AST;

//
// Parser
//

template <Token::Type TYPE>
/*static*/ inline bool
Parser::TestTokenType(Token::Type type)
{
    return (type == TYPE);
}

template <Token::Type T1, Token::Type T2, Token::Type... TS>
/*static*/ inline bool
Parser::TestTokenType(Token::Type type)
{
    return (type == T1) || TestTokenType<T2, TS...>(type);
}

template <Token::Type TYPE>
/*static*/ inline bool
Parser::ContainsKeywordType()
{
    return Token::IsKeywordType(TYPE);
}

template <Token::Type T1, Token::Type T2, Token::Type... TS>
/*static*/ inline bool
Parser::ContainsKeywordType()
{
    return Token::IsKeywordType(T1) || ContainsKeywordType<T2, TS...>();
}

template <Token::Type... TYPES>
inline const Token *
Parser::checkGetNextToken(Tokenizer::InputElementKind kind, bool checkKw)
{
    WH_ASSERT_IF(!checkKw, !ContainsKeywordType<TYPES...>());

    const Token &tok = nextToken(kind, checkKw);
    if (TestTokenType<TYPES...>(tok.type()))
        return &tok;
    tokenizer_.pushBackLastToken();
    return nullptr;
}

template <Token::Type... TYPES>
inline const Token *
Parser::checkGetNextToken(bool checkKw)
{
    return checkGetNextToken<TYPES...>(Tokenizer::InputElement_Div,
                                       checkKw);
}

template <Token::Type... TYPES>
inline const Token *
Parser::checkGetNextToken()
{
    return checkGetNextToken<TYPES...>(Tokenizer::InputElement_Div, false);
}

template <Token::Type... TYPES>
inline const Token *
Parser::checkGetNextKeywordToken()
{
    return checkGetNextToken<TYPES...>(Tokenizer::InputElement_Div, true);
}

template <Token::Type... TYPES>
inline bool
Parser::checkNextToken(Tokenizer::InputElementKind kind, bool checkKw)
{
    const Token *tok = checkGetNextToken<TYPES...>(kind, checkKw);
    if (tok)
        tok->debug_markUsed();
    return tok;
}

template <Token::Type... TYPES>
inline bool
Parser::checkNextToken(bool checkKw)
{
    return checkNextToken<TYPES...>(Tokenizer::InputElement_Div, checkKw);
}

template <Token::Type... TYPES>
inline bool
Parser::checkNextToken()
{
    return checkNextToken<TYPES...>(Tokenizer::InputElement_Div, false);
}

template <Token::Type... TYPES>
inline bool
Parser::checkNextKeywordToken()
{
    return checkNextToken<TYPES...>(Tokenizer::InputElement_Div, true);
}

template <typename T>
inline STLBumpAllocator<T>
Parser::allocatorFor() const
{
    return STLBumpAllocator<T>(tokenizer_.allocator());
}

template <typename T, typename... ARGS>
inline T *
Parser::make(ARGS... args)
{
    return new (allocatorFor<T>().allocate(1)) T(
        std::forward<ARGS>(args)...);
}


} // namespace Whisper

#endif // WHISPER__PARSER__PARSER_INLINES_HPP
