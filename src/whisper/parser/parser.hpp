#ifndef WHISPER__PARSER__PARSER_HPP
#define WHISPER__PARSER__PARSER_HPP

#include "allocators.hpp"
#include "parser/tokenizer.hpp"
#include "parser/syntax_tree.hpp"

//
// The tokenizer parses a code source into a series of tokens.
//

namespace Whisper {

using namespace AST;

//
// Parser
//
class ParserError {
  friend class Parser;
  private:
    inline ParserError() {}
};

class Parser
{
  private:
    STLBumpAllocator<uint8_t> allocator_;
    Tokenizer& tokenizer_;

    // Error message.
    char const* error_ = nullptr;

  public:
    Parser(STLBumpAllocator<uint8_t> const& allocator,
           Tokenizer& tokenizer);
    ~Parser();

    FileNode* parseFile();

    inline bool hasError() const {
        return error_;
    }

    inline char const* error() const {
        WH_ASSERT(hasError());
        return error_;
    }

  private:
    void tryParseStatementList(StatementList& stmts);

    Statement* tryParseStatement();

    VarStmtNode* parseVarStatement();
    ConstStmtNode* parseConstStatement();
    DefStmtNode* parseDefStatement();
    ReturnStmtNode* parseReturnStatement();
    IfStmtNode* parseIfStatement();
    LoopStmtNode* parseLoopStatement();
    IfStmtNode::CondPair parseIfCondPair();

    Block* parseBlock();

    // Enum for expression precedence, from highest precedence to
    // lowest.
    enum Precedence {
        Prec_Highest        = 0,
        Prec_Trailer,   /* Postfix '->', '.', '(...)', '[...]', etc. */
        Prec_Unary,     /* Unary '+', '-' */
        Prec_Product,   /* Binary '*', '/' */
        Prec_Sum,       /* Binary '+', '-' */
        Prec_Comma,
        Prec_Parenthesis,
        Prec_Statement,
        Prec_Lowest
    };

    Expression* parseExpression(Token const& startToken, Precedence prec);

    Expression* parseExpression(Precedence prec) {
        return parseExpression(nextToken(), prec);
    }

    Expression* tryParseExpression(Token const& startToken,
                                   Precedence prec);

    Expression* parseExpressionRest(Expression* seedExpr,
                                    Precedence prec);

    Expression* parseCallTrailer(PropertyExpression* propExpr);

    // Push back token.
    inline void pushBackLastToken() {
        tokenizer_.pushBackLastToken();
    }
    inline void rewindToToken(Token const& tok) {
        tokenizer_.rewindToToken(tok);
    }

    // Read next token.
    Token const& nextToken();

    // Check to see if upcoming token matches expected type,
    // but also return the token that was checked for.

    template <Token::Type TYPE>
    inline static bool TestTokenType(Token::Type type) {
        return (type == TYPE);
    }

    template <Token::Type T1, Token::Type T2, Token::Type... TS>
    inline static bool TestTokenType(Token::Type type) {
        return (type == T1) || TestTokenType<T2, TS...>(type);
    }

    template <Token::Type T1, Token::Type T2, Token::Type... TS>
    inline static bool ContainsKeywordType() {
        return Token::IsKeywordType(T1) || ContainsKeywordType<T2, TS...>();
    }

    template <Token::Type... TYPES>
    inline Token const* checkGetNextToken()
    {
        Token const& tok = nextToken();
        if (TestTokenType<TYPES...>(tok.type()))
            return &tok;
        tokenizer_.pushBackLastToken();
        return nullptr;
    }

    template <Token::Type... TYPES>
    inline Token::Type checkTypeNextToken()
    {
        Token const& tok = nextToken();
        if (TestTokenType<TYPES...>(tok.type())) {
            tok.debug_markUsed();
            return tok.type();
        }
        tokenizer_.pushBackLastToken();
        return Token::Type::INVALID;
    }

    // Check to see if upcoming token matches expected type.
    template <Token::Type... TYPES>
    inline bool checkNextToken() {
        Token const* tok = checkGetNextToken<TYPES...>();
        if (tok)
            tok->debug_markUsed();
        return tok;
    }

    // Emit a parser error.

    struct MorphError {
        // MorphError can convert to any type.
        template <typename T> inline operator T() {
            // This should NEVER be reached.
            WH_UNREACHABLE("MorphError");
            throw false;
        }
    };
    MorphError emitError(char const* msg);

    // Allocation helpers.

    template <typename T>
    inline STLBumpAllocator<T> allocatorFor() const {
        return STLBumpAllocator<T>(allocator_);
    }

    template <typename T, typename... ARGS>
    inline T* make(ARGS... args) {
        return new (allocatorFor<T>().allocate(1)) T(
            std::forward<ARGS>(args)...);
    }
};


} // namespace Whisper

#endif // WHISPER__PARSER__PARSER_HPP
