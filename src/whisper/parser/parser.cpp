
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
        StatementList stmts(allocatorFor<StatementNode *>());
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
        const Token &tok = nextToken();

        // Check for expression-starting tokens.
        if (tok.isIdentifier() || tok.isIntegerLiteral() ||
            tok.isPlus() || tok.isMinus())
        {
            pushBackLastToken();
            ExpressionNode *expr = parseExpression();

            // Consume semicolon at end of statement.
            if (!checkNextToken<Token::Type::Semicolon>())
                emitError("Expected semicolon at end of expression.");

            stmts.push_back(make<ExprStmtNode>(expr));
            continue;
        } 

        if (tok.isSemicolon()) {
            stmts.push_back(make<EmptyStmtNode>());
            continue;
        }

        pushBackLastToken();

        break;
    }
}

ExpressionNode *
Parser::parseExpression(const Token &startToken, Precedence prec)
{
    ExpressionNode *expr = nullptr;
    if (startToken.isIdentifier()) {
        expr = make<IdentifierExprNode>(IdentifierToken(startToken));

    } else if (startToken.isIntegerLiteral()) {
        expr = make<IntegerLiteralExprNode>(IntegerLiteralToken(startToken));

    } else if (startToken.isMinus()) {
        ExpressionNode *subexpr = parseExpression(Prec_Unary);
        expr = make<NegExprNode>(subexpr);

    } else if (startToken.isPlus()) {
        ExpressionNode *subexpr = parseExpression(Prec_Unary);
        expr = make<PosExprNode>(subexpr);

    } else {
        emitError("Unrecognized expression token.");
    }

    return parseExpressionRest(expr, prec);
}

ExpressionNode *
Parser::parseExpressionRest(ExpressionNode *seedExpr, Precedence prec)
{
    WH_ASSERT(prec > Prec_Highest && prec <= Prec_Lowest);

    ExpressionNode *curExpr = seedExpr;
    Precedence curPrec = prec;

    for (;;) {
        // Read first token.
        const Token &optok = nextToken();
        optok.debug_markUsed();

        // Check from highest to lowest precedence.
        if (optok.isStar()) {
            if (prec <= Prec_Product) {
                pushBackLastToken();
                break;
            }

            ExpressionNode *rhsExpr = parseExpression(Prec_Product);
            curExpr = make<MulExprNode>(curExpr, rhsExpr);
            continue;
        }

        if (optok.isSlash()) {
            if (prec <= Prec_Product) {
                pushBackLastToken();
                break;
            }

            ExpressionNode *rhsExpr = parseExpression(Prec_Product);
            curExpr = make<DivExprNode>(curExpr, rhsExpr);
            continue;
        }

        if (optok.isPlus()) {
            if (prec <= Prec_Sum) {
                pushBackLastToken();
                break;
            }

            ExpressionNode *rhsExpr = parseExpression(Prec_Sum);
            curExpr = make<AddExprNode>(curExpr, rhsExpr);
            continue;
        }

        if (optok.isMinus()) {
            if (prec <= Prec_Sum) {
                pushBackLastToken();
                break;
            }

            ExpressionNode *rhsExpr = parseExpression(Prec_Sum);
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
