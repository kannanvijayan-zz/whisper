#ifndef WHISPER__PARSER__PARSER_HPP
#define WHISPER__PARSER__PARSER_HPP

#include <new>
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
    Tokenizer &tokenizer_;

    // Error message.
    const char *error_ = nullptr;

    // Strict mode.
    bool strict_ = false;

    // Marks the existence of a pushed-back implicit semicolon.
    Token automaticSemicolon_;
    bool hasAutomaticSemicolon_ = false;
    bool justReadAutomaticSemicolon_ = false;

  public:
    Parser(Tokenizer &tokenizer)
      : tokenizer_(tokenizer),
        automaticSemicolon_()
    {}

    inline ~Parser() {}

    ProgramNode *parseProgram();

    inline bool hasError() const {
        return error_;
    }

    inline const char *error() const {
        WH_ASSERT(error_);
        return error_;
    }
        

  private:
    SourceElementNode *tryParseSourceElement();
    StatementNode *tryParseStatement();

    VariableStatementNode *parseVariableStatement();
    VariableDeclaration parseVariableDeclaration();

    IfStatementNode *parseIfStatement();
    IterationStatementNode *parseForStatement();
    BlockNode *tryParseBlock();
    IterationStatementNode *parseWhileStatement();
    IterationStatementNode *parseDoWhileStatement();
    ReturnStatementNode *parseReturnStatement(const Token &retToken);
    BreakStatementNode *parseBreakStatement(const Token &breakToken);
    ContinueStatementNode *parseContinueStatement(const Token &continueToken);
    SwitchStatementNode *parseSwitchStatement();
    TryStatementNode *parseTryStatement();
    ThrowStatementNode *parseThrowStatement();
    DebuggerStatementNode *parseDebuggerStatement();
    WithStatementNode *parseWithStatement();

    ExpressionNode *tryParseExpressionStatement(bool &sawLabel);
    LabelledStatementNode *tryParseLabelledStatement(
                            const IdentifierNameToken &label);

    // Precedences, from low to high
    enum Precedence
    {
        Prec_Comma = 0,
        Prec_Assignment,    // =, +=, *=, ...
        Prec_Conditional,   // ?:
        Prec_LogicalOr,     // ||
        Prec_LogicalAnd,    // &&
        Prec_BitwiseOr,     // |
        Prec_BitwiseXor,    // ^
        Prec_BitwiseAnd,    // &
        Prec_Equality,      // ==, ===, !=, !==
        Prec_Relational,    // <, >, >=, <=, instanceof, in
        Prec_Shift,         // <<, >>, >>>
        Prec_Additive,      // +, -
        Prec_Multiplicative,// *, /, %
        Prec_Unary,         // delete, void, typeof, ++, --, +, -, ~, !
        Prec_Postfix,       // ++, --
        Prec_Member
    };

    ExpressionNode *tryParseExpression(bool forbidIn,
                                       Precedence prec,
                                       bool expectSemicolon=false);

    inline ExpressionNode *tryParseExpression(bool forbidIn,
                                              bool expectSemicolon=false)
    {
        return tryParseExpression(forbidIn, Prec_Comma, expectSemicolon);
    }
    inline ExpressionNode *tryParseExpression(Precedence prec,
                                              bool expectSemicolon=false)
    {
        return tryParseExpression(/*forbidIn=*/false, prec, expectSemicolon);
    }
    inline ExpressionNode *tryParseExpression() {
        return tryParseExpression(/*forbidIn=*/false);
    }

    ArrayLiteralNode *tryParseArrayLiteral();
    ObjectLiteralNode *tryParseObjectLiteral();
    FunctionExpressionNode *tryParseFunction();
    void parseFunctionBody(SourceElementList &body);

    void parseArguments(ExpressionList &list);

    // Push back token.

    void pushBackAutomaticSemicolon();
    void pushBackLastToken();

    // Read next token.

    const Token &nextToken(Tokenizer::InputElementKind kind,
                           bool checkKeywords);

    inline const Token &nextToken(bool checkKeywords) {
        return nextToken(Tokenizer::InputElement_Div, checkKeywords);
    }

    inline const Token &nextToken() {
        return nextToken(Tokenizer::InputElement_Div, false);
    }

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

    template <Token::Type TYPE>
    inline static bool ContainsKeywordType() {
        return Token::IsKeywordType(TYPE);
    }
    template <Token::Type T1, Token::Type T2, Token::Type... TS>
    inline static bool ContainsKeywordType() {
        return Token::IsKeywordType(T1) ||
               ContainsKeywordType<T2, TS...>();
    }

    template <Token::Type... TYPES>
    inline const Token *checkGetNextToken(Tokenizer::InputElementKind kind,
                                          bool checkKw)
    {
        WH_ASSERT_IF(!checkKw, !ContainsKeywordType<TYPES...>());

        const Token &tok = nextToken(kind, checkKw);
        if (TestTokenType<TYPES...>(tok.type()))
            return &tok;
        tokenizer_.pushBackLastToken();
        return nullptr;
    }

    template <Token::Type... TYPES>
    inline const Token *checkGetNextToken(bool checkKw) {
        return checkGetNextToken<TYPES...>(Tokenizer::InputElement_Div,
                                           checkKw);
    }

    template <Token::Type... TYPES>
    inline const Token *checkGetNextToken() {
        return checkGetNextToken<TYPES...>(Tokenizer::InputElement_Div, false);
    }

    template <Token::Type... TYPES>
    inline const Token *checkGetNextKeywordToken() {
        return checkGetNextToken<TYPES...>(Tokenizer::InputElement_Div, true);
    }

    // Check to see if upcoming token matches expected type.

    template <Token::Type... TYPES>
    inline bool checkNextToken(Tokenizer::InputElementKind kind, bool checkKw) {
        const Token *tok = checkGetNextToken<TYPES...>(kind, checkKw);
        if (tok)
            tok->debug_markUsed();
        return tok;
    }

    template <Token::Type... TYPES>
    inline bool checkNextToken(bool checkKw) {
        return checkNextToken<TYPES...>(Tokenizer::InputElement_Div, checkKw);
    }

    template <Token::Type... TYPES>
    inline bool checkNextToken() {
        return checkNextToken<TYPES...>(Tokenizer::InputElement_Div, false);
    }

    template <Token::Type... TYPES>
    inline bool checkNextKeywordToken() {
        return checkNextToken<TYPES...>(Tokenizer::InputElement_Div, true);
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
    MorphError emitError(const char *msg);

    template <typename T>
    inline STLBumpAllocator<T> allocatorFor() const {
        return STLBumpAllocator<T>(tokenizer_.allocator());
    }

    template <typename T, typename... ARGS>
    inline T *make(ARGS... args) {
        return new (allocatorFor<T>().allocate(1)) T(
            std::forward<ARGS>(args)...);
    }
};


} // namespace Whisper

#endif // WHISPER__PARSER__PARSER_HPP
