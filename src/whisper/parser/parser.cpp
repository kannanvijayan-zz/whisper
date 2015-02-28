
#include "spew.hpp"
#include "parser/parser.hpp"

#include <vector>

//
// The tokenizer parses a code source into a series of tokens.
//

namespace Whisper {


//
// Parser
//

Parser::Parser(const STLBumpAllocator<uint8_t> &allocator,
               Tokenizer &tokenizer)
  : allocator_(allocator),
    tokenizer_(tokenizer)
{}

Parser::~Parser() {}

FileNode *
Parser::parseFile()
{
    try {
        // Parse statements.
        StatementList stmts(allocatorFor<Statement *>());
        tryParseStatementList(stmts);

        // Should have reached end of file.
        if (!checkNextToken<Token::Type::End>())
            emitError("Expected end-of-input after statements.");

        // Construct and return a FileNode.
        return make<FileNode>(std::move(stmts));

    } catch (ParserError err) {
        return nullptr;

    } catch (BumpAllocatorError err) {
        error_ = "Memory allocation failed during parse.";
        return nullptr;
    }
}

void
Parser::tryParseStatementList(StatementList &stmts)
{
    for (;;) {
        Statement *stmt = tryParseStatement();
        if (stmt) {
            stmts.push_back(stmt);
            continue;
        }

        break;
    }
}

Statement *
Parser::tryParseStatement()
{
    const Token &tok = nextToken();

    Expression *expr = tryParseExpression(tok, Prec_Statement);
    if (expr) {
        // Consume semicolon at end of statement.
        if (!checkNextToken<Token::Type::Semicolon>())
            emitError("Expected semicolon at end of expression.");

        return make<ExprStmtNode>(expr);
    } 

    if (tok.isSemicolon()) {
        tok.debug_markUsed();
        return make<EmptyStmtNode>();
    }

    pushBackLastToken();
    return nullptr;
}

Expression *
Parser::parseExpression(const Token &startToken, Precedence prec)
{
    Expression *expr = tryParseExpression(startToken, prec);
    if (!expr)
        emitError("Expected expression.");

    return expr;
}

Expression *
Parser::tryParseExpression(const Token &startToken, Precedence prec)
{
    Expression *expr = nullptr;
    if (startToken.isIdentifier()) {
        expr = make<NameExprNode>(IdentifierToken(startToken));

    } else if (startToken.isIntegerLiteral()) {
        expr = make<IntegerExprNode>(IntegerLiteralToken(startToken));

    } else if (startToken.isMinus()) {
        startToken.debug_markUsed();
        Expression *subexpr = parseExpression(Prec_Unary);
        expr = make<NegExprNode>(subexpr);

    } else if (startToken.isPlus()) {
        startToken.debug_markUsed();
        Expression *subexpr = parseExpression(Prec_Unary);
        expr = make<PosExprNode>(subexpr);

    } else {
        return nullptr;

    }

    return parseExpressionRest(expr, prec);
}

Expression *
Parser::parseExpressionRest(Expression *seedExpr, Precedence prec)
{
    WH_ASSERT(prec > Prec_Highest && prec <= Prec_Lowest);

    Expression *curExpr = seedExpr;

    for (;;) {
        // Read first token.
        const Token &optok = nextToken();
        optok.debug_markUsed();

        // Check from highest to lowest precedence.

        if (optok.isDot()) {
            const Token *maybeName =
                checkGetNextToken<Token::Type::Identifier>();
            if (!maybeName)
                emitError("Expected identifier after '.'");

            IdentifierToken name(*maybeName);

            DotExprNode *dotExpr = make<DotExprNode>(curExpr, name);
            curExpr = parseCallTrailer(dotExpr);
            continue;
        }

        if (optok.isArrow()) {
            const Token *maybeName =
                checkGetNextToken<Token::Type::Identifier>();
            if (!maybeName)
                emitError("Expected identifier after '->'");

            IdentifierToken name(*maybeName);

            ArrowExprNode *arrowExpr = make<ArrowExprNode>(curExpr, name);
            curExpr = parseCallTrailer(arrowExpr);
            continue;
        }

        if (optok.isStar()) {
            if (prec <= Prec_Product) {
                pushBackLastToken();
                break;
            }

            Expression *rhsExpr = parseExpression(Prec_Product);
            curExpr = make<MulExprNode>(curExpr, rhsExpr);
            continue;
        }

        if (optok.isSlash()) {
            if (prec <= Prec_Product) {
                pushBackLastToken();
                break;
            }

            Expression *rhsExpr = parseExpression(Prec_Product);
            curExpr = make<DivExprNode>(curExpr, rhsExpr);
            continue;
        }

        if (optok.isPlus()) {
            if (prec <= Prec_Sum) {
                pushBackLastToken();
                break;
            }

            Expression *rhsExpr = parseExpression(Prec_Sum);
            curExpr = make<AddExprNode>(curExpr, rhsExpr);
            continue;
        }

        if (optok.isMinus()) {
            if (prec <= Prec_Sum) {
                pushBackLastToken();
                break;
            }

            Expression *rhsExpr = parseExpression(Prec_Sum);
            curExpr = make<SubExprNode>(curExpr, rhsExpr);
            continue;
        }

        // Got an unrecognized token.  Let caller handle it.
        pushBackLastToken();
        break;
    }

    WH_ASSERT(curExpr);
    return curExpr;
}

Expression *
Parser::parseCallTrailer(PropertyExpression *propExpr)
{
    // Check for open paren.
    const Token *tok = checkGetNextToken<Token::Type::OpenParen>();
    if (!tok)
        return propExpr;
    tok->debug_markUsed();

    // Got open paren, parse call.
    ExpressionList expressions(allocatorFor<Expression *>());
    for (;;) {
        const Token &tok = nextToken();

        Expression *expr = tryParseExpression(tok, Prec_Comma);
        if (expr) {
            const Token *nextTok =
                checkGetNextToken<Token::Type::Comma,
                                  Token::Type::CloseParen>();

            if (!nextTok)
                emitError("Expected ',' or ')' at end of expression.");

            expressions.push_back(expr);

            if (nextTok->isComma()) {
                nextTok->debug_markUsed();
                continue;
            }

            WH_ASSERT(nextTok->isCloseParen());
            nextTok->debug_markUsed();
            break;
        }

        if (tok.isCloseParen()) {
            tok.debug_markUsed();
            break;
        }

        pushBackLastToken();
        emitError("Expected ')' at end of call expression.");
    }

    return make<CallExprNode>(propExpr, std::move(expressions));
}

const Token &
Parser::nextToken()
{
    // Read next valid token.
    for (;;) {
        const Token &tok = tokenizer_.readToken();
        tok.debug_markUsed();
        if (tok.isWhitespace())
            continue;

        if (tok.isLineTerminatorSequence())
            continue;

        if (tok.isMultiLineComment() || tok.isSingleLineComment())
            continue;

        if (tok.isError()) {
            error_ = tokenizer_.error();
            throw ParserError();
        }

        tok.debug_clearUsed();
        return tok;
    }
}

Parser::MorphError
Parser::emitError(const char *msg)
{
    error_ = msg;
    SpewParserError("%s", msg);
    throw ParserError();
}


} // namespace Whisper
