
#include "parser/parser.hpp"

//
// The tokenizer parses a code source into a series of tokens.
//

namespace Whisper {


ProgramNode *
Parser::parseProgram()
{
    try {
        SourceElementList sourceElements(allocatorFor<SourceElementNode *>());

        // Read zero or more source elements.
        for (;;) {
            SourceElementNode *sourceElem = tryParseSourceElement();
            if (!sourceElem)
                break;
            sourceElements.push_back(sourceElem);
        }

        // Next token must be end of input.
        if (!checkNextToken<Token::End>())
            emitError("Invalid source element.");

        return make<ProgramNode>(std::move(sourceElements));
    } catch (ParserError err) {
        return nullptr;
    }
}

SourceElementNode *
Parser::tryParseSourceElement()
{
    // Try to parse a statement.
    StatementNode *stmt = tryParseStatement();
    if (!stmt) {
        if (checkNextToken<Token::CloseBrace, Token::End>()) {
            pushBackLastToken();
            return nullptr;
        }
        return emitError("Could not parse source element.");
    }

    // If the statement is actually a ExpressionStatement, and
    // the ExpressionStatement is a FunctionExpression with
    // a non-empty name, treat the expression as a FunctionDeclaration.
    FunctionExpressionNode *funExpr = MaybeToNamedFunction(stmt);
    if (funExpr)
        return make<FunctionDeclarationNode>(funExpr);

    return stmt;
}

StatementNode *
Parser::tryParseStatement()
{
    // Many statements begin with identifiable first tokens.
    const Token &tok0 = nextToken(Tokenizer::InputElement_Div, true);

    // Variable statement.
    if (tok0.isVarKeyword()) {
        tok0.debug_markUsed();
        return parseVariableStatement();
    }

    // If statement.
    else if (tok0.isIfKeyword()) {
        tok0.debug_markUsed();
        return parseIfStatement();
    }

    // For statement.
    else if (tok0.isForKeyword()) {
        tok0.debug_markUsed();
        return parseForStatement();
    }

    // Block.
    else if (tok0.isOpenBrace()) {
        tok0.debug_markUsed();
        BlockNode *block = tryParseBlock();
        if (block)
            return block;
    }

    // While statement.
    else if (tok0.isWhileKeyword()) {
        tok0.debug_markUsed();
        return parseWhileStatement();
    }

    // Do/While statement.
    else if (tok0.isDoKeyword()) {
        tok0.debug_markUsed();
        return parseDoWhileStatement();
    }

    // Return statement.
    else if (tok0.isReturnKeyword()) {
        tok0.debug_markUsed();
        Token retToken(tok0);
        return parseReturnStatement(retToken);
    }

    // Break statement.
    else if (tok0.isBreakKeyword()) {
        tok0.debug_markUsed();
        Token breakToken(tok0);
        return parseBreakStatement(breakToken);
    }

    // Continue statement.
    else if (tok0.isContinueKeyword()) {
        tok0.debug_markUsed();
        Token continueToken(tok0);
        return parseContinueStatement(continueToken);
    }

    // Switch statement.
    else if (tok0.isSwitchKeyword()) {
        tok0.debug_markUsed();
        return parseSwitchStatement();
    }

    // Try statement.
    else if (tok0.isTryKeyword()) {
        tok0.debug_markUsed();
        return parseTryStatement();
    }

    // Throw statement.
    else if (tok0.isThrowKeyword()) {
        tok0.debug_markUsed();
        return parseThrowStatement();
    }

    // Empty statement.
    else if (tok0.isSemicolon()) {
        tok0.debug_markUsed();
        return make<EmptyStatementNode>();
    }

    // With statement.
    else if (tok0.isWithKeyword()) {
        tok0.debug_markUsed();
        return parseWithStatement();
    }

    // Debugger statement.
    else if (tok0.isDebuggerKeyword()) {
        tok0.debug_markUsed();
        return parseDebuggerStatement();
    }

    // Push back token and try parsing an expression or labelled statement.
    pushBackLastToken();

    // Expression statement.
    // Also indicates if a label-form was seen.
    bool sawLabel = false;
    ExpressionNode *expr = tryParseExpressionStatement(sawLabel);
    WH_ASSERT_IF(sawLabel, expr && expr->isIdentifier());

    if (!sawLabel && expr)
        return make<ExpressionStatementNode>(expr);

    if (!expr)
        return nullptr;

    // Saw a labelled statement.
    return tryParseLabelledStatement(ToIdentifier(expr)->token());
}

VariableStatementNode *
Parser::parseVariableStatement()
{
    // Var token already encountered.

    DeclarationList declarations(allocatorFor<VariableDeclaration>());
    for (;;) {
        // Always parse a VariableDeclaration
        VariableDeclaration decl = parseVariableDeclaration();
        declarations.push_back(decl);

        const Token *tok = checkGetNextToken<Token::Comma, Token::Semicolon>();
        if (!tok) {
            return emitError("Expected comma or semicolon after variable "
                             "declaration.");
        }

        if (tok->isSemicolon()) {
            tok->debug_markUsed();
            break;
        }

        WH_ASSERT(tok->isComma());
        tok->debug_markUsed();
    }

    return make<VariableStatementNode>(declarations);
}

VariableDeclaration
Parser::parseVariableDeclaration()
{
    const Token *tok0 = checkGetNextToken<Token::IdentifierName>(true);
    if (!tok0)
        emitError("Expected identifier in variable declaration.");
    IdentifierNameToken name(*tok0);

    const Token &tok1 = nextToken();
    if (tok1.isAssign()) {
        tok1.debug_markUsed();
        ExpressionNode *expr = tryParseExpression(Prec_Assignment,
                                                  /*expectSemicolon=*/true);
        if (!expr)
            emitError("Expected expression in variable declaration.");

        return VariableDeclaration(IdentifierNameToken(name), expr);
    }

    // If semicolon follows var, initializer is empty.
    if (tok1.isSemicolon()) {
        pushBackLastToken();
        return VariableDeclaration(IdentifierNameToken(name), nullptr);
    }

    // Otherwise, if next token is on different line from current one, insert
    // automatic semicolon.
    if (name.newlineOccursBefore(tok1)) {
        pushBackLastToken();
        pushBackAutomaticSemicolon();
        return VariableDeclaration(IdentifierNameToken(name), nullptr);
    }

    // Otherwise, bad variable declaration.
    return emitError("Invalid variable declaration.");
}

IfStatementNode *
Parser::parseIfStatement()
{
    // If keyword already read.

    if (!checkNextToken<Token::OpenParen>())
        emitError("Expected open parenthesis after 'if' keyword.");

    ExpressionNode *expr = tryParseExpression();
    if (!expr)
        emitError("Expected expression within 'if' conditional.");

    if (!checkNextToken<Token::CloseParen>())
        emitError("Expected close parenthesis after 'if' conditional.");

    StatementNode *trueBody = tryParseStatement();
    if (!trueBody)
        emitError("Expected 'if' body.");

    if (!checkNextKeywordToken<Token::ElseKeyword>())
        return make<IfStatementNode>(expr, trueBody, nullptr);

    StatementNode *falseBody = tryParseStatement();
    if (!falseBody)
        emitError("Expected 'else' body.");

    return make<IfStatementNode>(expr, trueBody, falseBody);
}

IterationStatementNode *
Parser::parseForStatement()
{
    // For keyword already read.

    if (!checkNextToken<Token::OpenParen>())
        emitError("Expected open parenthesis after 'for' keyword.");

    // Read for expression.  Check initial part of for clause to identify
    // whether it's a "for in" for regular for statement.
    TokenizerMark markFor = tokenizer_.mark();
    IdentifierNameToken varName;
    bool isForIn = false;
    bool isInitVarDecl = false;
    bool isEmptyVarDecl = false;
    bool isEmptyLastVarDecl = false;
    if (checkNextKeywordToken<Token::VarKeyword>()) {
        const Token *tok1 = checkGetNextToken<Token::IdentifierName>();
        if (!tok1)
            emitError("Invalid var declaration in for-loop clause.");

        varName = IdentifierNameToken(*tok1);

        const Token *tok2 = checkGetNextKeywordToken<Token::Assign,
                                                     Token::InKeyword,
                                                     Token::Comma,
                                                     Token::Semicolon>();
        if (!tok2)
            emitError("Invalid var declaration in for-loop clause.");
        tok2->debug_markUsed();

        if (tok2->isAssign())
            isInitVarDecl = true;
        else if (tok2->isInKeyword())
            isForIn = true;
        else if (tok2->isComma())
            isEmptyVarDecl = true;
        else // tok2->isSemicolon()
            isEmptyLastVarDecl = true;
    }

    // handle 'for..in'
    if (isForIn) {
        ExpressionNode *expr = tryParseExpression();
        if (!expr)
            emitError("Could not parse for..in clause expression.");

        // read closing ')'
        if (!checkNextToken<Token::CloseParen>())
            emitError("Did not read close parenthesis for for..in clause.");

        // Read body statement.
        StatementNode *body = tryParseStatement();
        if (!body)
            emitError("Failed to parse body statement for for..in.");

        return make<ForInVarStatementNode>(varName, expr, body);
    }

    // handle variable declaration
    if (isInitVarDecl || isEmptyVarDecl || isEmptyLastVarDecl) {
        WH_ASSERT_IF(isInitVarDecl, !isEmptyVarDecl && !isEmptyLastVarDecl);
        WH_ASSERT_IF(isEmptyVarDecl, !isInitVarDecl && !isEmptyLastVarDecl);
        WH_ASSERT_IF(isEmptyLastVarDecl, !isInitVarDecl && !isEmptyVarDecl);

        DeclarationList declarations(allocatorFor<VariableDeclaration>());
        ExpressionNode *initExpr = nullptr;

        bool moreVars = true;
        if (isInitVarDecl) {
            initExpr = tryParseExpression(Prec_Assignment);
            if (!initExpr)
                emitError("Failed to parse for-loop var initializer.");

            // Expect comma or semicolon after this.
            const Token *tok1 = checkGetNextToken<Token::Comma,
                                                  Token::Semicolon>();
            if (!tok1)
                emitError("Need ',' or ';' after for-loop var declaration");

            if (tok1->isSemicolon())
                moreVars = false;

            tok1->debug_markUsed();

        } else if (isEmptyLastVarDecl) {
            moreVars = false;
        }

        declarations.push_back(VariableDeclaration(varName, initExpr));

        // Parse more variable declarations
        if (moreVars) {
            while (moreVars) {
                const Token *tok2 = checkGetNextToken<Token::IdentifierName>();
                if (!tok2)
                    emitError("Expected identifier in for-loop variables.");

                varName = IdentifierNameToken(*tok2);

                const Token *tok3 = checkGetNextToken<Token::Assign,
                                                      Token::Comma,
                                                      Token::Semicolon>();
                if (!tok3)
                    emitError("Invalid var declaration in for-loop clause.");
                tok3->debug_markUsed();

                initExpr = nullptr;
                if (tok3->isAssign()) {
                    initExpr = tryParseExpression(Prec_Assignment);
                    if (!initExpr)
                        emitError("Failed to parse for-loop var initializer.");

                    // Expect comma or semicolon after this.
                    const Token *tok4 = checkGetNextToken<Token::Comma,
                                                          Token::Semicolon>();
                    if (!tok4) {
                        emitError("Need ',' or ';' after for-loop var "
                                  "declaration");
                    }

                    if (tok4->isSemicolon())
                        moreVars = false;

                    tok4->debug_markUsed();

                } else if (tok3->isSemicolon()) {
                    moreVars = false;
                }

                declarations.push_back(VariableDeclaration(varName, initExpr));
            }
        }

        ExpressionNode *condition = nullptr;
        ExpressionNode *update = nullptr;

        // Parse condition expression in for loop.
        if (!checkNextToken<Token::Semicolon>()) {
            // Parse the condition (now that we know it's not empty)
            condition = tryParseExpression();
            if (!condition)
                emitError("Invalid condition expression in for loop.");

            // Expect semicolon.
            if (!checkNextToken<Token::Semicolon>())
                emitError("Expected semicolon after condition in for loop.");
        }

        // Parse the update expression in for loop.
        if (!checkNextToken<Token::CloseParen>()) {
            update = tryParseExpression();
            if (!update)
                emitError("Invalid update expression in for loop.");

            // Expect semicolon.
            if (!checkNextToken<Token::CloseParen>())
                emitError("Expected close paren after condition in for loop.");
        }

        // Parse the for-loop body.
        StatementNode *body = tryParseStatement();
        if (!body)
            emitError("Invalid for loop body.");

        return make<ForLoopVarStatementNode>(declarations, condition, update,
                                             body);
    }

    // Handle regular for loop.
    tokenizer_.gotoMark(markFor);

    ExpressionNode *init = nullptr;
    ExpressionNode *condition = nullptr;
    ExpressionNode *update = nullptr;

    // Parse initial expression in for loop.
    if (!checkNextToken<Token::Semicolon>()) {
        // Parse the init (now that we know it's not empty)
        init = tryParseExpression(/*forbidIn=*/true);
        if (!init)
            emitError("Invalid condition expression in for loop.");

        const Token *tok5 = checkGetNextToken<Token::Semicolon,
                                              Token::InKeyword>();
        if (!tok5) {
            emitError("Expected semicolon or 'in' keyword after init in "
                      "for loop.");
        }
        tok5->debug_markUsed();

        if (tok5->isInKeyword()) {
            if (!IsLeftHandSideExpression(init))
                emitError("Invalid left-hand side in for..in.");
            
        }

        // Expect semicolon.
        if (!checkNextToken<Token::Semicolon>())
            emitError("Expected semicolon after init in for loop.");
    }

    // Parse condition expression in for loop.
    if (!checkNextToken<Token::Semicolon>()) {
        // Parse the condition (now that we know it's not empty)
        condition = tryParseExpression();
        if (!condition)
            emitError("Invalid condition expression in for loop.");

        // Expect semicolon.
        if (!checkNextToken<Token::Semicolon>())
            emitError("Expected semicolon after condition in for loop.");
    }

    // Parse the update expression in for loop.
    if (!checkNextToken<Token::CloseParen>()) {
        update = tryParseExpression();
        if (!update)
            emitError("Invalid update expression in for loop.");

        // Expect semicolon.
        if (!checkNextToken<Token::CloseParen>())
            emitError("Expected close paren after condition in for loop.");
    }

    // Parse the for-loop body.
    StatementNode *body = tryParseStatement();
    if (!body)
        emitError("Invalid for loop body.");

    return make<ForLoopStatementNode>(init, condition, update, body);
}

BlockNode *
Parser::tryParseBlock()
{
    TokenizerMark mark = tokenizer_.mark();

    // Open brace token already encountered.

    // Read statements within block.
    SourceElementList list(allocatorFor<SourceElementNode *>());
    for (;;) {
        if (checkNextToken<Token::CloseBrace>())
            break;

        SourceElementNode *stmt = tryParseSourceElement();
        if (!stmt) {
            tokenizer_.gotoMark(mark);
            return nullptr;
        }

        list.push_back(stmt);
    }

    return make<BlockNode>(std::move(list));
}

IterationStatementNode *
Parser::parseWhileStatement()
{
    // While keyword already encountered.

    if (!checkNextToken<Token::OpenParen>())
        emitError("Expected open paren after while keyword.");

    ExpressionNode *condition = tryParseExpression();
    if (!condition)
        emitError("Error parsing while loop condition.");

    if (!checkNextToken<Token::CloseParen>())
        emitError("Expected close paren after while loop condition.");

    StatementNode *body = tryParseStatement();
    if (!body)
        emitError("Invalid while loop body.");

    return make<WhileStatementNode>(condition, body);
}

IterationStatementNode *
Parser::parseDoWhileStatement()
{
    // Do keyword already encountered.

    StatementNode *body = tryParseStatement();
    if (!body)
        emitError("Invalid do-while loop body.");

    if (!checkNextToken<Token::WhileKeyword>())
        emitError("Expected while keyword after do-while loop body.");

    if (!checkNextToken<Token::OpenParen>())
        emitError("Expected open paren after while keyword.");

    ExpressionNode *condition = tryParseExpression();
    if (!condition)
        emitError("Error parsing while loop condition.");

    if (!checkNextToken<Token::CloseParen>())
        emitError("Expected close paren after while loop condition.");

    return make<DoWhileStatementNode>(body, condition);
}

ReturnStatementNode *
Parser::parseReturnStatement(const Token &retToken)
{
    // Return keyword already encountered.
    const Token &tok = nextToken();

    if (tok.isEnd() || retToken.newlineOccursBefore(tok)) {
        // Implicit semicolon.
        pushBackLastToken();
        return make<ReturnStatementNode>(nullptr);
    }

    if (tok.isSemicolon()) {
        tok.debug_markUsed();
        return make<ReturnStatementNode>(nullptr);
    }

    // Parse return expression.
    pushBackLastToken();
    ExpressionNode *node = tryParseExpression(Prec_Comma,
                                              /*expectSemicolon=*/true);
    if (!node)
        emitError("Invalid return expression.");

    if (!checkNextToken<Token::Semicolon>())
        emitError("Return statement not terminated properly.");

    return make<ReturnStatementNode>(node);
}

BreakStatementNode *
Parser::parseBreakStatement(const Token &breakToken)
{
    // Break keyword already encountered.
    const Token &tok = nextToken();

    if (tok.isEnd() || breakToken.newlineOccursBefore(tok)) {
        // Implicit semicolon.
        pushBackLastToken();
        return make<BreakStatementNode>();
    }

    if (tok.isSemicolon()) {
        tok.debug_markUsed();
        return make<BreakStatementNode>();
    }

    if (tok.isIdentifierName()) {
        IdentifierNameToken ident(tok);
        const Token &tok2 = nextToken();
        if (tok2.isEnd() || ident.newlineOccursBefore(tok2)) {
            pushBackLastToken();
            return make<BreakStatementNode>(ident);
        }

        if (tok2.isSemicolon()) {
            tok2.debug_markUsed();
            return make<BreakStatementNode>(ident);
        }
    }

    return emitError("Invalid break statement.");
}

ContinueStatementNode *
Parser::parseContinueStatement(const Token &continueToken)
{
    // Continue keyword already encountered.
    const Token &tok = nextToken();

    if (tok.isEnd() || continueToken.newlineOccursBefore(tok)) {
        // Implicit semicolon.
        pushBackLastToken();
        return make<ContinueStatementNode>();
    }

    if (tok.isSemicolon()) {
        tok.debug_markUsed();
        return make<ContinueStatementNode>();
    }

    if (tok.isIdentifierName()) {
        IdentifierNameToken ident(tok);
        const Token &tok2 = nextToken();
        if (tok2.isEnd() || ident.newlineOccursBefore(tok2)) {
            pushBackLastToken();
            return make<ContinueStatementNode>(ident);
        }

        if (tok2.isSemicolon()) {
            tok2.debug_markUsed();
            return make<ContinueStatementNode>(ident);
        }
    }

    return emitError("Invalid continue statement.");
}

SwitchStatementNode *
Parser::parseSwitchStatement()
{
    // Switch keyword already encountered.

    if (!checkNextToken<Token::OpenParen>())
        emitError("Expected open paren after switch.");

    ExpressionNode *value = tryParseExpression();
    if (!value)
        emitError("Error parsing switch value expression.");

    if (!checkNextToken<Token::CloseParen>())
        emitError("Expected close paren after switch value expression.");

    if (!checkNextToken<Token::OpenBrace>())
        emitError("Expected open brace for switch block.");

    bool gotDefault = false;

    // Parse case clauses
    SwitchStatementNode::CaseClauseList caseClauses(
                            allocatorFor<SwitchStatementNode::CaseClause>());
    for (;;) {
        const Token *tok = checkGetNextToken<Token::CloseBrace,
                                             Token::CaseKeyword,
                                             Token::DefaultKeyword>();
        if (!tok)
            emitError("Expected '}' or 'case'.");
        tok->debug_markUsed();

        if (tok->isCloseBrace())
            break;

        ExpressionNode *expr = nullptr;

        // Got case or default keyword.
        if (tok->isDefaultKeyword()) {
            if (gotDefault)
                emitError("Duplicate default case class.");
            gotDefault = true;
        } else {
            expr = tryParseExpression();
            if (!expr)
                emitError("Invalid expression for case value.");
        }

        // Expect colon.
        if (!checkNextToken<Token::Colon>())
            emitError("Expected colon after case value expression.");

        // Parse statements.
        StatementList stmts(allocatorFor<StatementNode *>());

        for (;;) {
            StatementNode *stmt = tryParseStatement();
            if (!stmt)
                break;

            stmts.push_back(stmt);
        }

        // Add case clause.
        caseClauses.push_back(SwitchStatementNode::CaseClause(
                                            expr, std::move(stmts)));
    }

    return make<SwitchStatementNode>(value, std::move(caseClauses));
}

TryStatementNode *
Parser::parseTryStatement()
{
    // Try keyword already encountered.

    if (!checkNextToken<Token::OpenBrace>())
        emitError("Expected '{' after try.");

    BlockNode *tryBlock = tryParseBlock();
    if (!tryBlock)
        emitError("Error parsing try block.");

    const Token *nextTok = checkGetNextToken<Token::CatchKeyword,
                                             Token::FinallyKeyword>();
    if (!nextTok)
        emitError("Expected catch or finally block after try.");

    nextTok->debug_markUsed();

    if (nextTok->isFinallyKeyword()) {
        if (!checkNextToken<Token::OpenBrace>())
            emitError("Expected '{' for finally block.");

        BlockNode *finallyBlock = tryParseBlock();
        if (!finallyBlock)
            emitError("Error parsing finally block.");

        return make<TryFinallyStatementNode>(tryBlock, finallyBlock);
    }

    // Catch or Catch+Finally
    if (!checkNextToken<Token::OpenParen>())
        emitError("Expected '(' after catch.");

    const Token *nameTok = checkGetNextToken<Token::IdentifierName>();
    if (!nameTok)
        emitError("Expected catch argument name.");

    IdentifierNameToken catchName(*nameTok);

    if (!checkNextToken<Token::CloseParen>())
        emitError("Expected ')' after catch argument name.");

    if (!checkNextToken<Token::OpenBrace>())
        emitError("Expected '{' for catch block.");

    BlockNode *catchBlock = tryParseBlock();
    if (!catchBlock)
        emitError("Error parsing catch block.");

    BlockNode *finallyBlock = nullptr;
    if (checkNextToken<Token::FinallyKeyword>()) {
        if (!checkNextToken<Token::OpenBrace>())
            emitError("Expected '{' for finally block.");

        finallyBlock = tryParseBlock();
        if (!finallyBlock)
            emitError("Error parsing finally block.");
    }

    if (!finallyBlock)
        return make<TryCatchStatementNode>(tryBlock, catchName, catchBlock);

    return make<TryCatchFinallyStatementNode>(tryBlock, catchName, catchBlock,
                                              finallyBlock);
}

ThrowStatementNode *
Parser::parseThrowStatement()
{
    // Throw keyword already encountered.
    const Token &tok = nextToken();

    if (tok.isEnd())
        emitError("Invalid throw statement.");

    if (tok.newlineOccursBefore(tok))
        emitError("Newline not allowed between throw and expression.");

    // Parse throw expression.
    pushBackLastToken();
    ExpressionNode *expr = tryParseExpression(Prec_Comma,
                                              /*expectSemicolon=*/true);
    if (!expr)
        emitError("Invalid throw expression.");

    if (!checkNextToken<Token::Semicolon>())
        emitError("Throw statement not terminated properly.");

    return make<ThrowStatementNode>(expr);
}

WithStatementNode *
Parser::parseWithStatement()
{
    // With keyword already encountered.

    if (!checkNextToken<Token::OpenParen>())
        emitError("Expected open paren after with keyword.");

    ExpressionNode *expr = tryParseExpression();
    if (!expr)
        emitError("Could not parse with clause.");

    if (!checkNextToken<Token::CloseParen>())
        emitError("Expected close paren after with clause.");

    StatementNode *body = tryParseStatement();
    if (!body)
        emitError("Could not parse with body.");

    return make<WithStatementNode>(expr, body);
}

DebuggerStatementNode *
Parser::parseDebuggerStatement()
{
    // Debugger keyword already encountered.
    const Token &tok = nextToken();

    if (tok.isEnd() || tok.newlineOccursBefore(tok)) {
        // Implicit semicolon.
        pushBackLastToken();
        return make<DebuggerStatementNode>();
    }

    if (tok.isSemicolon()) {
        tok.debug_markUsed();
        return make<DebuggerStatementNode>();
    }

    return emitError("Invalid debugger statement.");
}

ExpressionNode *
Parser::tryParseExpressionStatement(bool &sawLabel)
{
    WH_ASSERT(!sawLabel);

    ExpressionNode *expr = tryParseExpression(/*forbidIn=*/false,
                                              /*prec=*/Prec_Comma,
                                              /*expectSemicolon=*/true);
    if (!expr)
        return nullptr;

    const Token &tok = nextToken();

    // If the expression is a raw identifier, and the following token
    // is a colon, then we are parsing a labelled statement.  Note that
    // for caller.
    if (expr->isIdentifier() && tok.isColon()) {
        sawLabel = true;
        tok.debug_markUsed();
        return expr;
    }

    // Otherwise, expression statement must be terminated with semicolon.
    // tryParseExpression will add an automatic semicolon to the token
    // stream if appropriate.
    if (tok.isSemicolon()) {
        tok.debug_markUsed();
        return expr;
    }

    return emitError("Expression statement not properly terminated.");
}

LabelledStatementNode *
Parser::tryParseLabelledStatement(const IdentifierNameToken &label)
{
    StatementNode *stmt = tryParseStatement();
    if (!stmt)
        emitError("No statement after label.");

    return make<LabelledStatementNode>(label, stmt);
}

ExpressionNode *
Parser::tryParseExpression(bool forbidIn, Precedence prec,
                           bool expectSemicolon)
{
    TokenizerMark markExpr = tokenizer_.mark();

    // Read first token.
    // Allow RegExps and keywords.
    const Token &tok = nextToken(Tokenizer::InputElement_RegExp, true);
    tok.debug_markUsed();

    ExpressionNode *curExpr = nullptr;

    // Parse initial "starter" expression.
    if (tok.isIdentifierName()) {
        curExpr = make<IdentifierNode>(IdentifierNameToken(tok));

    } else if (tok.isNumericLiteral()) {
        curExpr = make<NumericLiteralNode>(NumericLiteralToken(tok));

    } else if (tok.isStringLiteral()) {
        curExpr = make<StringLiteralNode>(StringLiteralToken(tok));

    } else if (tok.isFalseLiteral()) {
        curExpr = make<BooleanLiteralNode>(FalseLiteralToken(tok));

    } else if (tok.isTrueLiteral()) {
        curExpr = make<BooleanLiteralNode>(TrueLiteralToken(tok));

    } else if (tok.isNullLiteral()) {
        curExpr = make<NullLiteralNode>(NullLiteralToken(tok));

    } else if (tok.isRegularExpressionLiteral()) {
        curExpr = make<RegularExpressionLiteralNode>(
                        RegularExpressionLiteralToken(tok));

    } else if (tok.isThisKeyword()) {
        curExpr = make<ThisNode>(ThisKeywordToken(tok));

    } else if (tok.isOpenParen()) {
        // Parse sub-expression
        ExpressionNode *subexpr = tryParseExpression();
        if (!subexpr)
            emitError("Could not parse expression within parenthesis.");
        curExpr = make<ParenthesizedExpressionNode>(subexpr);

        // Consume the close parenthesis.
        if (!checkNextToken<Token::CloseParen>())
            emitError("Expected close parenthesis.");

    } else if (tok.isOpenBracket()) {
        // Parse an array literal.
        curExpr = tryParseArrayLiteral();
        if (!curExpr)
            emitError("Could not parse array literal.");

    } else if (tok.isOpenBrace()) {
        // Parse object literal.  This can't be a block statement because
        // that has already been tried before.
        curExpr = tryParseObjectLiteral();
        if (!curExpr)
            emitError("Could not parse object literal.");

    } else if (tok.isFunctionKeyword()) {
        // Parse function literal.
        curExpr = tryParseFunction();
        if (!curExpr)
            emitError("Could not parse function literal.");

    } else if (tok.isNewKeyword()) {
        ExpressionNode *cons = tryParseExpression(forbidIn, Prec_Member);
        if (!cons)
            emitError("Could not parse new expression.");

        ExpressionList args(allocatorFor<ExpressionNode *>());
        if (checkNextToken<Token::OpenParen>())
            parseArguments(args);

        curExpr = make<NewExpressionNode>(cons, args);

    } else if (tok.isLogicalNot()) {
        ExpressionNode *expr = tryParseExpression(forbidIn, Prec_Unary);
        if (!expr)
            emitError("Error parsing unary expression.");

        curExpr = make<LogicalNotExpressionNode>(expr);

    } else if (tok.isMinus()) {
        ExpressionNode *expr = tryParseExpression(forbidIn, Prec_Unary);
        if (!expr)
            emitError("Error parsing unary expression.");

        curExpr = make<NegativeExpressionNode>(expr);

    } else if (tok.isPlus()) {
        ExpressionNode *expr = tryParseExpression(forbidIn, Prec_Unary);
        if (!expr)
            emitError("Error parsing unary expression.");

        curExpr = make<PositiveExpressionNode>(expr);

    } else if (tok.isTilde()) {
        ExpressionNode *expr = tryParseExpression(forbidIn, Prec_Unary);
        if (!expr)
            emitError("Error parsing unary expression.");

        curExpr = make<BitNotExpressionNode>(expr);

    } else if (tok.isPlusPlus()) {
        ExpressionNode *expr = tryParseExpression(forbidIn, Prec_Unary);
        if (!expr)
            emitError("Error parsing unary expression.");

        if (!IsLeftHandSideExpression(expr))
            emitError("Expected lvalue for pre-increment expression.");

        curExpr = make<PreIncrementExpressionNode>(expr);

    } else if (tok.isMinusMinus()) {
        ExpressionNode *expr = tryParseExpression(forbidIn, Prec_Unary);
        if (!expr)
            emitError("Error parsing unary expression.");

        if (!IsLeftHandSideExpression(expr))
            emitError("Expected lvalue for pre-decrement expression.");

        curExpr = make<PreDecrementExpressionNode>(expr);

    } else if (tok.isDeleteKeyword()) {
        ExpressionNode *expr = tryParseExpression(forbidIn, Prec_Unary);
        if (!expr)
            emitError("Error parsing unary expression.");

        if (!IsLeftHandSideExpression(expr))
            emitError("Expected lvalue for delete expression.");

        curExpr = make<DeleteExpressionNode>(expr);

    } else if (tok.isVoidKeyword()) {
        ExpressionNode *expr = tryParseExpression(forbidIn, Prec_Unary);
        if (!expr)
            emitError("Error parsing unary expression.");

        curExpr = make<VoidExpressionNode>(expr);

    } else if (tok.isTypeOfKeyword()) {
        ExpressionNode *expr = tryParseExpression(forbidIn, Prec_Unary);
        if (!expr)
            emitError("Error parsing unary expression.");

        curExpr = make<TypeOfExpressionNode>(expr);

    } else {
        // No expression possible, return.
        tokenizer_.gotoMark(markExpr);
        return nullptr;
    }

    // Read and fold operators while within precedence scope.
    for (;;) {
        // Note the current line.
        uint32_t preOperatorLine = tokenizer_.line();

        // Check next token.
        const Token &tok2 = nextToken(Tokenizer::InputElement_Div, true);
        tok2.debug_markUsed();

        WH_ASSERT(prec <= Prec_Member);
        WH_ASSERT(prec >= Prec_Comma);

        // OpenBracket: GetElement operation (precedence: Member)
        if (tok2.isOpenBracket()) {
            // Parse a sub-expression at lowest predecence.
            ExpressionNode *idxExpr = tryParseExpression();
            if (!idxExpr)
                emitError("Could not parse GetElement sub-expression.");

            // Subsequent token must be a ']'
            if (!checkNextToken<Token::CloseBracket>())
                emitError("Missing close bracket for GetElement.");

            curExpr = make<GetElementExpressionNode>(curExpr, idxExpr);
            continue;
        }

        // Dot: GetProperty operation (precedence: Member)
        if (tok2.isDot()) {
            // Must be followed by identifier.
            const Token *name = checkGetNextToken<Token::IdentifierName>(
                                                        /*checkKw=*/false);
            if (!name)
                emitError("No identifier after GetProperty notation.");

            curExpr = make<GetPropertyExpressionNode>(
                                    curExpr, IdentifierNameToken(*name));
            continue;
        }

        // OpenParen: Call operation (precedence: Member)
        if (tok2.isOpenParen()) {
            // Parse arguments.

            ExpressionList args(allocatorFor<ExpressionNode *>());
            parseArguments(args);

            curExpr = make<CallExpressionNode>(curExpr, args);
            continue;
        }

        // PlusPlus or MinusMinus: Increment/Decrement operation
        // (precedence: Postfix)
        if (tok2.isPlusPlus() || tok2.isMinusMinus()) {
            if (prec > Prec_Postfix) {
                tokenizer_.pushBackLastToken();
                break;
            }

            Token::Type type = tok2.type();

            if (!IsLeftHandSideExpression(curExpr))
                emitError("Expected lvalue for post-increment/decrement.");

            if (type == Token::PlusPlus)
                curExpr = make<PostIncrementExpressionNode>(curExpr);
            else
                curExpr = make<PostDecrementExpressionNode>(curExpr);

            continue;
        }

        // Star,Percent,Divide: Multiplicative
        if (tok2.isStar() || tok2.isPercent() || tok2.isDivide()) {
            if (prec > Prec_Multiplicative) {
                tokenizer_.pushBackLastToken();
                break;
            }

            Token::Type type = tok2.type();

            // Parse next expression
            ExpressionNode *nextExpr = tryParseExpression(forbidIn, Prec_Unary);
            if (!nextExpr)
                emitError("Could not parse multiplicative subexpression.");

            if (type == Token::Star)
                curExpr = make<MultiplyExpressionNode>(curExpr, nextExpr);
            else if (type == Token::Percent)
                curExpr = make<ModuloExpressionNode>(curExpr, nextExpr);
            else
                curExpr = make<DivideExpressionNode>(curExpr, nextExpr);

            continue;
        }

        // Plus,Minus: Additive
        if (tok2.isPlus() || tok2.isMinus()) {
            if (prec > Prec_Additive) {
                tokenizer_.pushBackLastToken();
                break;
            }

            Token::Type type = tok2.type();

            // Parse next expression
            ExpressionNode *nextExpr = tryParseExpression(forbidIn,
                                                          Prec_Multiplicative);
            if (!nextExpr)
                emitError("Could not parse additive subexpression.");

            if (type == Token::Plus)
                curExpr = make<AddExpressionNode>(curExpr, nextExpr);
            else
                curExpr = make<SubtractExpressionNode>(curExpr, nextExpr);

            continue;
        }

        // Shift
        if (tok2.isShiftLeft() || tok2.isShiftRight() ||
            tok2.isShiftUnsignedRight())
        {
            if (prec > Prec_Shift) {
                tokenizer_.pushBackLastToken();
                break;
            }

            Token::Type type = tok2.type();

            // Parse next expression
            ExpressionNode *nextExpr = tryParseExpression(forbidIn,
                                                          Prec_Additive);
            if (!nextExpr)
                emitError("Could not parse shift subexpression.");

            if (type == Token::ShiftLeft) {
                curExpr = make<LeftShiftExpressionNode>(curExpr, nextExpr);
            } else if (type == Token::ShiftRight) {
                curExpr = make<RightShiftExpressionNode>(curExpr, nextExpr);
            } else {
                curExpr = make<UnsignedRightShiftExpressionNode>(curExpr,
                                                                 nextExpr);
            }

            continue;
        }

        // If 'in' is forbidden, but seen, terminate expression here.
        if (forbidIn && tok2.isInKeyword())
            break;

        // Relational
        if (tok2.isLessThan() || tok2.isLessEqual() ||
            tok2.isGreaterThan() || tok2.isGreaterEqual() ||
            tok2.isInstanceOfKeyword() || tok2.isInKeyword())
        {
            if (prec > Prec_Relational) {
                tokenizer_.pushBackLastToken();
                break;
            }

            Token::Type type = tok2.type();

            // Parse next expression
            ExpressionNode *nextExpr = tryParseExpression(forbidIn,
                                                          Prec_Shift);
            if (!nextExpr)
                emitError("Could not parse relational subexpression.");

            if (type == Token::LessThan)
                curExpr = make<LessThanExpressionNode>(curExpr, nextExpr);
            else if (type == Token::LessEqual)
                curExpr = make<LessEqualExpressionNode>(curExpr, nextExpr);
            else if (type == Token::GreaterThan)
                curExpr = make<GreaterThanExpressionNode>(curExpr, nextExpr);
            else if (type == Token::GreaterEqual)
                curExpr = make<GreaterEqualExpressionNode>(curExpr, nextExpr);
            else if (type == Token::InstanceOfKeyword)
                curExpr = make<InstanceOfExpressionNode>(curExpr, nextExpr);
            else
                curExpr = make<InExpressionNode>(curExpr, nextExpr);

            continue;
            
        }

        // Equality
        if (tok2.isEqual() || tok2.isNotEqual() ||
            tok2.isStrictEqual() || tok2.isStrictNotEqual())
        {
            if (prec > Prec_Equality) {
                tokenizer_.pushBackLastToken();
                break;
            }

            Token::Type type = tok2.type();

            // Parse next expression
            ExpressionNode *nextExpr = tryParseExpression(forbidIn,
                                                          Prec_Relational);
            if (!nextExpr)
                emitError("Could not parse equality subexpression.");

            if (type == Token::Equal)
                curExpr = make<EqualExpressionNode>(curExpr, nextExpr);
            else if (type == Token::NotEqual)
                curExpr = make<NotEqualExpressionNode>(curExpr, nextExpr);
            else if (type == Token::StrictEqual)
                curExpr = make<StrictEqualExpressionNode>(curExpr, nextExpr);
            else
                curExpr = make<StrictNotEqualExpressionNode>(curExpr, nextExpr);

            continue;
            
        }

        // Bitwise And
        if (tok2.isBitAnd()) {
            if (prec > Prec_BitwiseAnd) {
                tokenizer_.pushBackLastToken();
                break;
            }

            // Parse next expression
            ExpressionNode *nextExpr = tryParseExpression(forbidIn,
                                                          Prec_Equality);
            if (!nextExpr)
                emitError("Could not parse bitwise-and subexpression.");

            curExpr = make<BitAndExpressionNode>(curExpr, nextExpr);

            continue;
            
        }

        // Bitwise Xor
        if (tok2.isBitXor()) {
            if (prec > Prec_BitwiseXor) {
                tokenizer_.pushBackLastToken();
                break;
            }

            // Parse next expression
            ExpressionNode *nextExpr = tryParseExpression(forbidIn,
                                                          Prec_BitwiseAnd);
            if (!nextExpr)
                emitError("Could not parse bitwise-xor subexpression.");

            curExpr = make<BitXorExpressionNode>(curExpr, nextExpr);

            continue;
            
        }

        // Bitwise Or
        if (tok2.isBitOr()) {
            if (prec > Prec_BitwiseOr) {
                tokenizer_.pushBackLastToken();
                break;
            }

            // Parse next expression
            ExpressionNode *nextExpr = tryParseExpression(forbidIn,
                                                          Prec_BitwiseXor);
            if (!nextExpr)
                emitError("Could not parse bitwise-or subexpression.");

            curExpr = make<BitOrExpressionNode>(curExpr, nextExpr);

            continue;
            
        }

        // Logical And
        if (tok2.isLogicalAnd()) {
            if (prec > Prec_LogicalAnd) {
                tokenizer_.pushBackLastToken();
                break;
            }

            // Parse next expression
            ExpressionNode *nextExpr = tryParseExpression(forbidIn,
                                                          Prec_BitwiseOr);
            if (!nextExpr)
                emitError("Could not parse logical-and subexpression.");

            curExpr = make<LogicalAndExpressionNode>(curExpr, nextExpr);

            continue;
            
        }

        // Logical Or
        if (tok2.isLogicalOr()) {
            if (prec > Prec_LogicalOr) {
                tokenizer_.pushBackLastToken();
                break;
            }

            // Parse next expression
            ExpressionNode *nextExpr = tryParseExpression(forbidIn,
                                                          Prec_LogicalAnd);
            if (!nextExpr)
                emitError("Could not parse logical-or subexpression.");

            curExpr = make<LogicalOrExpressionNode>(curExpr, nextExpr);

            continue;
            
        }

        // Conditional
        if (tok2.isQuestion()) {
            if (prec > Prec_Conditional) {
                tokenizer_.pushBackLastToken();
                break;
            }

            // Parse true expression
            ExpressionNode *trueExpr = tryParseExpression(forbidIn,
                                                          Prec_Assignment);
            if (!trueExpr)
                emitError("Could not parse conditional subexpression.");

            if (!checkNextToken<Token::Colon>())
                emitError("Expected colon in conditional.");

            // Parse false expression
            ExpressionNode *falseExpr = tryParseExpression(forbidIn,
                                                           Prec_Assignment);
            if (!falseExpr)
                emitError("Could not parse conditional subexpression.");

            curExpr = make<ConditionalExpressionNode>(curExpr, trueExpr,
                                                      falseExpr);
            continue;
        }

        // Assignment
        if (tok2.isAssign() || tok2.isStarAssign() ||
            tok2.isDivideAssign() || tok2.isPercentAssign() ||
            tok2.isPlusAssign() || tok2.isMinusAssign() ||
            tok2.isShiftLeftAssign() || tok2.isShiftRightAssign() || 
            tok2.isShiftUnsignedRightAssign() ||
            tok2.isBitAndAssign() || tok2.isBitXorAssign() ||
            tok2.isBitOrAssign())
        {
            if (prec > Prec_Assignment) {
                tokenizer_.pushBackLastToken();
                break;
            }

            Token::Type type = tok2.type();

            if (!IsLeftHandSideExpression(curExpr))
                emitError("Expected lvalue for assignment.");

            // Parse rhs expression
            ExpressionNode *rhsExpr = tryParseExpression(forbidIn,
                                                         Prec_Assignment);
            if (!rhsExpr)
                emitError("Could not parse assignment rhs.");

            if (type == Token::Assign) {
                curExpr = make<AssignExpressionNode>(curExpr, rhsExpr);
            } else if (type == Token::StarAssign) {
                curExpr = make<MultiplyAssignExpressionNode>(curExpr, rhsExpr);
            } else if (type == Token::DivideAssign) {
                curExpr = make<DivideAssignExpressionNode>(curExpr, rhsExpr);
            } else if (type == Token::PercentAssign) {
                curExpr = make<ModuloAssignExpressionNode>(curExpr, rhsExpr);
            } else if (type == Token::PlusAssign) {
                curExpr = make<AddAssignExpressionNode>(curExpr, rhsExpr);
            } else if (type == Token::MinusAssign) {
                curExpr = make<SubtractAssignExpressionNode>(curExpr, rhsExpr);
            } else if (type == Token::ShiftLeftAssign) {
                curExpr = make<LeftShiftAssignExpressionNode>(curExpr,
                                                              rhsExpr);
            } else if (type == Token::ShiftRightAssign) {
                curExpr = make<RightShiftAssignExpressionNode>(curExpr,
                                                               rhsExpr);
            } else if (type == Token::ShiftUnsignedRightAssign) {
                curExpr = make<UnsignedRightShiftAssignExpressionNode>(
                                                            curExpr, rhsExpr);
            } else if (type == Token::BitAndAssign) {
                curExpr = make<BitAndAssignExpressionNode>(curExpr, rhsExpr);
            } else if (type == Token::BitXorAssign) {
                curExpr = make<BitXorAssignExpressionNode>(curExpr, rhsExpr);
            } else {
                curExpr = make<BitOrAssignExpressionNode>(curExpr, rhsExpr);
            }

            continue;
        }

        // Comma
        if (tok2.isComma()) {
            if (prec > Prec_Comma) {
                tokenizer_.pushBackLastToken();
                break;
            }

            // Parse rhs expression
            ExpressionNode *nextExpr = tryParseExpression(forbidIn,
                                                          Prec_Assignment);
            if (!nextExpr)
                emitError("Could not parse comma sub-expression.");

            curExpr = make<CommaExpressionNode>(curExpr, nextExpr);

            continue;
        }

        // If not expecting a semicolon to terminate this expression,
        // just stop parsing here.
        if (!expectSemicolon) {
            pushBackLastToken();
            break;
        }

        // At this point, any other operator is not an acceptable
        // continuation of the expression.
        // However, if expecting a semicolon, a semicolon may be expected
        // to terminate the expression parsing.  Or, an implicit semicolon
        // may be assumed if a new line follows, or if this is the end of the
        // token stream.
        if (tok2.isSemicolon()) {
            pushBackLastToken();
            break;
        }

        if (tok2.isEnd() || tok2.startLine() > preOperatorLine) {
            pushBackLastToken();
            pushBackAutomaticSemicolon();
            break;
        }

        // Otherwise, return the expression.  tryParseExpressionStatement
        // may fail (if it doesn't find a labelled statement).
        break;
    }

    return curExpr;
}

ArrayLiteralNode *
Parser::tryParseArrayLiteral()
{
    // NOTE: Open bracket already read.

    ExpressionList exprList(allocatorFor<ExpressionNode *>());

    for (;;) {
        const Token *tok = checkGetNextToken<Token::Comma,
                                             Token::CloseBracket>();
        if (tok) {
            tok->debug_markUsed();

            if (tok->isComma()) {
                // Elision.
                exprList.push_back(nullptr);
                continue;
            }

            WH_ASSERT(tok->isCloseBracket());
            break;
        }

        ExpressionNode *expr = tryParseExpression(Prec_Assignment);
        if (!expr)
            emitError("Expected expression in array literal.");

        exprList.push_back(expr);
    }

    return make<ArrayLiteralNode>(exprList);
}

ObjectLiteralNode *
Parser::tryParseObjectLiteral()
{
    // NOTE: Open brace already read.

    ObjectLiteralNode::PropertyDefinitionList props(
                        allocatorFor<ObjectLiteralNode::PropertyDefinition>());

    for (;;) {
        const Token *name = checkGetNextToken<Token::IdentifierName,
                                              Token::StringLiteral,
                                              Token::NumericLiteral,
                                              Token::CloseBrace>(
                                                        /*checkKw=*/false);
        if (!name)
            emitError("Expected property definition name.");

        if (name->isCloseBrace()) {
            name->debug_markUsed();
            break;
        }

        bool isGetter = false;
        bool isSetter = false;
        Token nameToken(*name);

        if (name->isIdentifierName()) {
            if (IsGetToken(tokenizer_.source(), *name)) {
                name = checkGetNextToken<Token::IdentifierName,
                                         Token::StringLiteral,
                                         Token::NumericLiteral,
                                         Token::Colon>();
                if (!name)
                    emitError("Expected name or colon after 'get'.");
                name->debug_markUsed();

                if (!name->isColon()) {
                    isGetter = true;
                    nameToken = *name;
                }
            } else if (IsSetToken(tokenizer_.source(), *name)) {
                name = checkGetNextToken<Token::IdentifierName,
                                         Token::StringLiteral,
                                         Token::NumericLiteral,
                                         Token::Colon>();
                if (!name)
                    emitError("Expected name or colon after 'set'.");
                name->debug_markUsed();

                if (!name->isColon()) {
                    isSetter = true;
                    nameToken = *name;
                }
            } else {
                if (!checkNextToken<Token::Colon>())
                    emitError("Expected colon after object literal name.");
            }
        } else {
            if (!checkNextToken<Token::Colon>())
                emitError("Expected colon after object literal name.");
        }

        ObjectLiteralNode::PropertyDefinition *propDef = nullptr;

        // Handle getters and setters.
        if (isGetter) {
            if (!checkNextToken<Token::OpenParen>())
                emitError("Expected open paren in getter definition.");

            if (!checkNextToken<Token::CloseParen>())
                emitError("Expected close paren in getter definition.");

            if (!checkNextToken<Token::OpenBrace>())
                emitError("Expected open brace in getter definition.");

            // Parse function body.
            SourceElementList body(allocatorFor<SourceElementNode *>());
            parseFunctionBody(body);

            propDef = make<ObjectLiteralNode::GetterDefinition>(
                                                nameToken, std::move(body));
        } else if (isSetter) {
            if (!checkNextToken<Token::OpenParen>())
                emitError("Expected open paren in getter definition.");

            const Token *arg = checkGetNextToken<Token::IdentifierName>();
            if (!arg)
                emitError("Expected argument name in setter definition.");
            IdentifierNameToken argName(*arg);

            if (!checkNextToken<Token::CloseParen>())
                emitError("Expected close paren in getter definition.");

            if (!checkNextToken<Token::OpenBrace>())
                emitError("Expected open brace in getter definition.");

            // Parse function body.
            SourceElementList body(allocatorFor<SourceElementNode *>());
            parseFunctionBody(body);

            propDef = make<ObjectLiteralNode::SetterDefinition>(
                                        nameToken, argName, std::move(body));
        } else {
            // Value property definition.
            ExpressionNode *expr = tryParseExpression(Prec_Assignment);
            if (!expr)
                emitError("Expected object literal slot value expression.");

            propDef = make<ObjectLiteralNode::ValueDefinition>(
                                                        nameToken, expr);
        }
        props.push_back(propDef);

        // After this, expect close brace or comma.
        const Token *close = checkGetNextToken<Token::Comma,
                                               Token::CloseBrace>();
        if (!close)
            emitError("Expected ',' or '}' after object literal expression.");

        if (close->isCloseBrace()) {
            close->debug_markUsed();
            break;
        }

        close->debug_markUsed();
    }

    return make<ObjectLiteralNode>(props);
}

FunctionExpressionNode *
Parser::tryParseFunction()
{
    // NOTE: Function keyword already read.

    Maybe<IdentifierNameToken> name;

    const Token &tok0 = nextToken(Tokenizer::InputElement_Div, true);
    if (tok0.isOpenParen()) {
        tok0.debug_markUsed();
        // Do nothing.
    } else if (tok0.isIdentifierName()) {
        // Token following identifiern name must be '('
        name = IdentifierNameToken(tok0);
        if (!checkNextToken<Token::OpenParen>())
            emitError("No open paren after function name.");
    } else if (tok0.isKeyword(strict_)) {
        // Identifier name token can't be keyword.
        emitError("Function name is keyword.");
    }

    // Parse formal parameter list.
    FunctionExpressionNode::FormalParameterList params(
                            allocatorFor<IdentifierNameToken>());

    // Check for immediate close paren.
    const Token &tok1 = nextToken(Tokenizer::InputElement_Div, true);
    if (tok1.isCloseParen()) {
        tok1.debug_markUsed();

    } else {
        for (;;) {
            if (tok1.isKeyword(strict_))
                emitError("Function formal can't be a keyword.");
            
            if (!tok1.isIdentifierName())
                emitError("Function formal argument expected.");

            params.push_back(IdentifierNameToken(tok1));

            const Token &tok2 = nextToken(Tokenizer::InputElement_Div, false);

            if (tok2.isCloseParen()) {
                tok2.debug_markUsed();
                break;
            }

            if (!tok2.isComma())
                emitError("Bad token following function formal argument.");
        }
    }

    // Open brace must follow.
    if (!checkNextToken<Token::OpenBrace>())
        emitError("No open brace after function signature.");

    // Now parse function body.
    SourceElementList body(allocatorFor<SourceElementNode *>());
    parseFunctionBody(body);

    if (name) {
        return make<FunctionExpressionNode>(*name,
                                            std::move(params),
                                            std::move(body));
    }

    return make<FunctionExpressionNode>(std::move(params), std::move(body));
}

void
Parser::parseFunctionBody(SourceElementList &body)
{
    for (;;) {
        SourceElementNode *node = tryParseSourceElement();
        if (!node)
            break;

        body.push_back(node);
    }

    if (!checkNextToken<Token::CloseBrace>())
        emitError("Invalid function body.");
}

void
Parser::parseArguments(ExpressionList &list)
{
    if (checkNextToken<Token::CloseParen>())
        return;
    for (;;) {
        ExpressionNode *expr = tryParseExpression(Prec_Assignment);
        if (!expr)
            emitError("Expected argument expression.");
        list.push_back(expr);
        const Token *tok = checkGetNextToken<Token::CloseParen,
                                             Token::Comma>();
        if (!tok)
            emitError("Expected comma or close paren after arg expression.");

        tok->debug_markUsed();
        if (tok->isCloseParen())
            break;
    }
}

void
Parser::pushBackAutomaticSemicolon()
{
    WH_ASSERT(!hasAutomaticSemicolon_);
    automaticSemicolon_ = tokenizer_.getAutomaticSemicolon();
    hasAutomaticSemicolon_ = true;
}

void
Parser::pushBackLastToken()
{
    WH_ASSERT(!hasAutomaticSemicolon_);
    if (justReadAutomaticSemicolon_) {
        hasAutomaticSemicolon_ = true;
        justReadAutomaticSemicolon_ = false;
        return;
    }
    tokenizer_.pushBackLastToken();
}

const Token &
Parser::nextToken(Tokenizer::InputElementKind kind, bool checkKeywords)
{
    if (hasAutomaticSemicolon_) {
        hasAutomaticSemicolon_ = false;
        justReadAutomaticSemicolon_ = true;
        return automaticSemicolon_;
    }
    justReadAutomaticSemicolon_ = false;

    // Read next valid token.
    for (;;) {
        const Token &tok = tokenizer_.readToken(kind, checkKeywords);
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
    throw ParserError();
}


} // namespace Whisper
