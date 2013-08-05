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
    Parser(Tokenizer &tokenizer);

    ~Parser();

    ProgramNode *parseProgram();

    bool hasError() const;
    const char *error() const;
        

  private:
    SourceElementNode *tryParseSourceElement();
    StatementNode *tryParseStatement(bool *isNamedFunction = nullptr);

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

    ExpressionNode *tryParseExpressionStatement(bool *sawLabel,
                                                bool *isNamedFunction);
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
        Prec_Call,
        Prec_Member
    };

    ExpressionNode *tryParseExpression(bool forbidIn,
                                       Precedence prec,
                                       bool expectSemicolon=false);

    ExpressionNode *tryParseExpression(bool forbidIn,
                                       bool expectSemicolon=false);

    ExpressionNode *tryParseExpression(Precedence prec,
                                        bool expectSemicolon=false);
    ExpressionNode *tryParseExpression();

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

    const Token &nextToken(bool checkKeywords);
    const Token &nextToken();

    // Check to see if upcoming token matches expected type,
    // but also return the token that was checked for.

    template <Token::Type TYPE>
    inline static bool TestTokenType(Token::Type type);

    template <Token::Type T1, Token::Type T2, Token::Type... TS>
    inline static bool TestTokenType(Token::Type type);

    template <Token::Type TYPE>
    inline static bool ContainsKeywordType();

    template <Token::Type T1, Token::Type T2, Token::Type... TS>
    inline static bool ContainsKeywordType();

    template <Token::Type... TYPES>
    inline const Token *checkGetNextToken(Tokenizer::InputElementKind kind,
                                          bool checkKw);

    template <Token::Type... TYPES>
    inline const Token *checkGetNextToken(bool checkKw);

    template <Token::Type... TYPES>
    inline const Token *checkGetNextToken();

    template <Token::Type... TYPES>
    inline const Token *checkGetNextKeywordToken();

    // Check to see if upcoming token matches expected type.

    template <Token::Type... TYPES>
    inline bool checkNextToken(Tokenizer::InputElementKind kind, bool checkKw);

    template <Token::Type... TYPES>
    inline bool checkNextToken(bool checkKw);

    template <Token::Type... TYPES>
    inline bool checkNextToken();

    template <Token::Type... TYPES>
    inline bool checkNextKeywordToken();

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
    inline STLBumpAllocator<T> allocatorFor() const;

    template <typename T, typename... ARGS>
    inline T *make(ARGS... args);
};


} // namespace Whisper

#endif // WHISPER__PARSER__PARSER_HPP
