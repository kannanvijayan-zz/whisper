
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

Parser::Parser(STLBumpAllocator<uint8_t> const& allocator,
               Tokenizer& tokenizer)
  : allocator_(allocator),
    tokenizer_(tokenizer)
{}

Parser::~Parser() {}

FileNode*
Parser::parseFile()
{
    try {
        // Parse statements.
        StatementList stmts(allocatorFor<Statement*>());
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
Parser::tryParseStatementList(StatementList& stmts)
{
    for (;;) {
        Statement* stmt = tryParseStatement();
        if (stmt) {
            stmts.push_back(stmt);
            continue;
        }

        break;
    }
}

Statement*
Parser::tryParseStatement()
{
    Token tok = nextToken();

    Expression* expr = tryParseExpression(tok, Prec_Statement);
    if (expr) {
        // Consume semicolon at end of statement.
        if (!checkNextToken<Token::Type::Semicolon>())
            emitError("Expected semicolon at end of expression.");

        return make<ExprStmtNode>(expr);
    }

    if (tok.isVarKeyword())
        return parseVarStatement();

    if (tok.isConstKeyword())
        return parseConstStatement();

    if (tok.isDefKeyword())
        return parseDefStatement();

    if (tok.isReturnKeyword())
        return parseReturnStatement();

    if (tok.isIfKeyword())
        return parseIfStatement();

    if (tok.isLoopKeyword())
        return parseLoopStatement();

    if (tok.isSemicolon())
        return make<EmptyStmtNode>();

    rewindToToken(tok);
    return nullptr;
}

VarStmtNode*
Parser::parseVarStatement()
{
    BindingStatement::BindingList bindings(
        allocatorFor<BindingStatement::Binding>());

    for (;;) {
        // Get name.
        Token const* nameTok = checkGetNextToken<Token::Type::Identifier>();
        if (!nameTok)
            emitError("Expected variable name in 'var' statement.");

        IdentifierToken name(*nameTok);

        // Check for '=' assign, or ',' or ';'
        Token::Type nextType = checkTypeNextToken<Token::Type::Equal,
                                                  Token::Type::Comma,
                                                  Token::Type::Semicolon>();
        if (nextType == Token::Type::INVALID)
            emitError("Unexpected token after name in 'var' statement.");

        if (nextType == Token::Type::Equal) {
            // Parse initializer expression.
            Expression* expr = parseExpression(Prec_Comma);
            bindings.push_back(BindingStatement::Binding(name, expr));

            // Expect a ',' or ';' after it.
            nextType = checkTypeNextToken<Token::Type::Comma,
                                          Token::Type::Semicolon>();
            if (nextType == Token::Type::INVALID)
                emitError("Expected ',' or ';' after var initializer.");

            if (nextType == Token::Type::Comma)
                continue;

            WH_ASSERT(nextType == Token::Type::Semicolon);
            break;
        }

        // Push back empty binding for name.
        bindings.push_back(BindingStatement::Binding(name, nullptr));

        if (nextType == Token::Type::Comma)
            continue;

        WH_ASSERT(nextType == Token::Type::Semicolon);
        break;
    }

    return make<VarStmtNode>(std::move(bindings));
}

ConstStmtNode*
Parser::parseConstStatement()
{
    BindingStatement::BindingList bindings(
        allocatorFor<BindingStatement::Binding>());

    for (;;) {
        // Get name.
        Token const* nameTok = checkGetNextToken<Token::Type::Identifier>();
        if (!nameTok)
            emitError("Expected variable name in 'const' statement.");

        IdentifierToken name(*nameTok);

        // Check for '=' or ';'
        if (!checkNextToken<Token::Type::Equal>())
            emitError("Expected '=' after 'const' name.");

        // Parse initializer expression.
        Expression* expr = parseExpression(Prec_Comma);
        bindings.push_back(BindingStatement::Binding(name, expr));

        // Expect a ',' or ';' after it.
        Token::Type nextType = checkTypeNextToken<Token::Type::Comma,
                                                  Token::Type::Semicolon>();
        if (nextType == Token::Type::INVALID)
            emitError("Expected ',' or ';' after 'const' initializer.");

        if (nextType == Token::Type::Comma)
            continue;

        WH_ASSERT(nextType == Token::Type::Semicolon);
        break;
    }

    return make<ConstStmtNode>(std::move(bindings));
}

DefStmtNode*
Parser::parseDefStatement()
{
    // Must be followed by name.
    Token const* nameTok = checkGetNextToken<Token::Type::Identifier>();
    if (!nameTok)
        emitError("Expected name after 'def'.");

    IdentifierToken name(*nameTok);

    // Must be followed by '('
    if (!checkNextToken<Token::Type::OpenParen>())
        emitError("Expected '(' in conditional pair.");

    // Parse parameter list.
    IdentifierList paramNames(allocatorFor<IdentifierToken>());
    for (;;) {
        Token const* paramTok = checkGetNextToken<Token::Type::Identifier,
                                                  Token::Type::CloseParen>();
        if (!paramTok)
            emitError("Unexpected token in def parameter list.");

        if (paramTok->isIdentifier()) {
            paramNames.push_back(IdentifierToken(*paramTok));
            Token::Type nextType =
                checkTypeNextToken<Token::Type::Comma,
                                   Token::Type::CloseParen>();

            if (nextType == Token::Type::INVALID)
                emitError("Expected ',' or ')' in def params.");

            if (nextType == Token::Type::Comma)
                continue;

            WH_ASSERT(nextType == Token::Type::CloseParen);
            break;
        }

        if (paramTok->isCloseParen())
            break;
    }

    // Expect open-brace afterward.
    if (!checkNextToken<Token::Type::OpenBrace>())
        emitError("Expected '{' after def params.");

    Block* block = parseBlock();

    return make<DefStmtNode>(name, std::move(paramNames), block);
}

ReturnStmtNode*
Parser::parseReturnStatement()
{
    Token nextTok = nextToken();

    // Parse return statement.
    Expression* expr = tryParseExpression(nextTok, Prec_Statement);
    if (expr) {
        if (!checkNextToken<Token::Type::Semicolon>())
            emitError("Expected semicolon after return statement.");

        return make<ReturnStmtNode>(expr);
    }

    if (!nextTok.isSemicolon())
        emitError("Expected semicolon after return statement.");

    return make<ReturnStmtNode>(nullptr);
}

IfStmtNode*
Parser::parseIfStatement()
{
    IfStmtNode::CondPair ifPair = parseIfCondPair();

    // List of elsif cond pairs.
    IfStmtNode::CondPairList elsifPairs(allocatorFor<IfStmtNode::CondPair>());

    Block* elseBlock = nullptr;

    // Check for following 'elsif' or 'else'
    Token::Type type = checkTypeNextToken<Token::Type::ElseKeyword,
                                          Token::Type::ElsifKeyword>();
    while (type == Token::Type::ElsifKeyword) {

        elsifPairs.push_back(parseIfCondPair());

        type = checkTypeNextToken<Token::Type::ElseKeyword,
                                  Token::Type::ElsifKeyword>();
    }

    if (type == Token::Type::ElseKeyword) {
        // Expect open-brace afterward.
        if (!checkNextToken<Token::Type::OpenBrace>())
            emitError("Expected '{' after 'else' keyword.");

        elseBlock = parseBlock();
    }

    return make<IfStmtNode>(ifPair, std::move(elsifPairs), elseBlock);
}

IfStmtNode::CondPair
Parser::parseIfCondPair()
{
    // Must be followed by '('
    if (!checkNextToken<Token::Type::OpenParen>())
        emitError("Expected '(' in conditional pair.");

    // Parse a conditional expression.
    Expression* expr = parseExpression(Prec_Lowest);

    // Expect close paren afterward.
    if (!checkNextToken<Token::Type::CloseParen>())
        emitError("Expected ')' in conditional pair.");

    // Expect open-brace afterward.
    if (!checkNextToken<Token::Type::OpenBrace>())
        emitError("Expected '{' in conditional pair.");

    Block* block = parseBlock();

    return IfStmtNode::CondPair(expr, block);
}

LoopStmtNode*
Parser::parseLoopStatement()
{
    // Expect open-brace afterward.
    if (!checkNextToken<Token::Type::OpenBrace>())
        emitError("Expected '{' after 'loop' keyword.");

    Block* block = parseBlock();
    return make<LoopStmtNode>(block);
}

Block*
Parser::parseBlock()
{
    // Parse statements.
    StatementList stmts(allocatorFor<Statement*>());
    tryParseStatementList(stmts);

    // Should have reached end-of-block.
    if (!checkNextToken<Token::Type::CloseBrace>())
        emitError("Expected '}' at end of block.");

    // Construct and return a FileNode.
    return make<Block>(std::move(stmts));
}

Expression*
Parser::parseExpression(Token const& startToken, Precedence prec)
{
    Expression* expr = tryParseExpression(startToken, prec);
    if (!expr)
        emitError("Expected expression.");

    return expr;
}

Expression*
Parser::tryParseExpression(Token const& startToken, Precedence prec)
{
    Expression* expr = nullptr;
    if (startToken.isIdentifier()) {
        NameExprNode* nameExpr = make<NameExprNode>(
                                    IdentifierToken(startToken));

        expr = parseCallTrailer(nameExpr);

    } else if (startToken.isIntegerLiteral()) {
        expr = make<IntegerExprNode>(IntegerLiteralToken(startToken));

    } else if (startToken.isMinus()) {
        startToken.debug_markUsed();
        Expression* subexpr = parseExpression(Prec_Unary);
        expr = make<NegExprNode>(subexpr);

    } else if (startToken.isPlus()) {
        startToken.debug_markUsed();
        Expression* subexpr = parseExpression(Prec_Unary);
        expr = make<PosExprNode>(subexpr);

    } else if (startToken.isOpenParen()) {
        startToken.debug_markUsed();
        Expression *subexpr = parseExpression(Prec_Parenthesis);
        if (!checkNextToken<Token::Type::CloseParen>())
            emitError("Expected close-paren after parenthetic expression.");
        expr = make<ParenExprNode>(subexpr);

    } else {
        return nullptr;

    }

    return parseExpressionRest(expr, prec);
}

Expression*
Parser::parseExpressionRest(Expression* seedExpr, Precedence prec)
{
    WH_ASSERT(prec > Prec_Highest && prec <= Prec_Lowest);

    Expression* curExpr = seedExpr;

    for (;;) {
        // Read first token.
        Token const& optok = nextToken();
        optok.debug_markUsed();

        // Check from highest to lowest precedence.

        if (optok.isDot()) {
            Token const* maybeName =
                checkGetNextToken<Token::Type::Identifier>();
            if (!maybeName)
                emitError("Expected identifier after '.'");

            IdentifierToken name(*maybeName);

            DotExprNode* dotExpr = make<DotExprNode>(curExpr, name);
            curExpr = parseCallTrailer(dotExpr);
            continue;
        }

        if (optok.isArrow()) {
            Token const* maybeName =
                checkGetNextToken<Token::Type::Identifier>();
            if (!maybeName)
                emitError("Expected identifier after '->'");

            IdentifierToken name(*maybeName);

            ArrowExprNode* arrowExpr = make<ArrowExprNode>(curExpr, name);
            curExpr = parseCallTrailer(arrowExpr);
            continue;
        }

        if (optok.isStar()) {
            if (prec <= Prec_Product) {
                pushBackLastToken();
                break;
            }

            Expression* rhsExpr = parseExpression(Prec_Product);
            curExpr = make<MulExprNode>(curExpr, rhsExpr);
            continue;
        }

        if (optok.isSlash()) {
            if (prec <= Prec_Product) {
                pushBackLastToken();
                break;
            }

            Expression* rhsExpr = parseExpression(Prec_Product);
            curExpr = make<DivExprNode>(curExpr, rhsExpr);
            continue;
        }

        if (optok.isPlus()) {
            if (prec <= Prec_Sum) {
                pushBackLastToken();
                break;
            }

            Expression* rhsExpr = parseExpression(Prec_Sum);
            curExpr = make<AddExprNode>(curExpr, rhsExpr);
            continue;
        }

        if (optok.isMinus()) {
            if (prec <= Prec_Sum) {
                pushBackLastToken();
                break;
            }

            Expression* rhsExpr = parseExpression(Prec_Sum);
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

Expression*
Parser::parseCallTrailer(PropertyExpression* propExpr)
{
    // Check for open paren.
    if (!checkNextToken<Token::Type::OpenParen>())
        return propExpr;

    // Got open paren, parse call.
    ExpressionList expressions(allocatorFor<Expression*>());
    for (;;) {
        Token tok = nextToken();

        Expression* expr = tryParseExpression(tok, Prec_Comma);
        if (expr) {
            Token const* nextTok =
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

        if (!tok.isCloseParen()) {
            rewindToToken(tok);
            emitError("Expected ')' at end of call expression.");
        }

        break;
    }

    return make<CallExprNode>(propExpr, std::move(expressions));
}

Token const&
Parser::nextToken()
{
    // Read next valid token.
    for (;;) {
        Token const& tok = tokenizer_.readToken();
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
Parser::emitError(char const* msg)
{
    error_ = msg;
    SpewParserError("%s", msg);
    throw ParserError();
}


} // namespace Whisper
