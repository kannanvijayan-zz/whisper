
#include "parser/parser.hpp"

//
// The tokenizer parses a code source into a series of tokens.
//

namespace Whisper {


ProgramNode *
Parser::parseProgram()
{
    ProgramNode::SourceElementList sourceElements(
                            allocatorFor<SourceElementNode *>());

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
}

SourceElementNode *
Parser::tryParseSourceElement()
{
    // Try to parse a statement.
    StatementNode *stmt = tryParseStatement();
    if (!stmt) {
        if (checkNextToken<Token::End>()) {
            tokenizer_.pushBackLastToken();
            return nullptr;
        }
        return emitError("Could not parse source element.");
    }

    // If the statement is actually a ExpressionStatement, and
    // the ExpressionStatement is a FunctionExpression with
    // a non-empty name, treat the expression as a FunctionDeclaration.
    if (stmt->isExpressionStatement()) {
        ExpressionNode *expr = ToExpressionStatement(stmt)->expression();
        if (expr->isFunctionExpression() && ToFunctionExpression(expr)->name())
            return make<FunctionDeclarationNode>(ToFunctionExpression(expr));
    }

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
        return parseReturnStatement();
    }

    // Break statement.
    else if (tok0.isBreakKeyword()) {
        tok0.debug_markUsed();
        return parseBreakStatement();
    }

    // Continue statement.
    else if (tok0.isContinueKeyword()) {
        tok0.debug_markUsed();
        return parseContinueStatement();
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
    tokenizer_.pushBackLastToken();

    // Expression statement.
    ExpressionStatementNode *exprStmt = tryParseExpressionStatement();
    if (exprStmt)
        return exprStmt;

    // Labelled statement.
    LabelledStatementNode *labStmt = tryParseLabelledStatement();
    if (labStmt)
        return labStmt;

    return nullptr;
}

VariableStatementNode *
Parser::parseVariableStatement()
{
    // Var token already encountered.

    VariableStatementNode::DeclarationList declarations(
                            allocatorFor<VariableDeclaration>());
    for (;;) {
        // Always parse a VariableDeclaration
        VariableDeclaration decl = parseVariableDeclaration();

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
        declarations.push_back(decl);
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
        AssignmentExpressionNode *expr = tryParseAssignmentExpression();
        if (!expr)
            emitError("Expected expression in variable declaration.");

        return VariableDeclaration(IdentifierNameToken(name), expr);
    }

    // If semicolon follows var, initializer is empty.
    if (tok1.isSemicolon()) {
        tokenizer_.rewindToToken(tok1);
        return VariableDeclaration(IdentifierNameToken(name), nullptr);
    }

    // Otherwise, if next token is on different line from current one, insert
    // automatic semicolon.
    if (name.newlineOccursBefore(tok1)) {
        tokenizer_.rewindToToken(tok1);
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
    ExpressionNode *expr = nullptr;
    bool isForIn = false;
    bool isInitVarDecl = false;
    bool isEmptyVarDecl = false;
    bool isEmptyLastVarDecl = false;
    if (checkNextToken<Token::VarKeyword>()) {
        const Token *tok1 = checkGetNextToken<Token::IdentifierName>();
        if (!tok1)
            emitError("Invalid var declaration in for-loop clause.");

        varName = IdentifierNameToken(*tok1);

        const Token *tok2 = checkGetNextToken<Token::Assign,
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

        VariableStatementNode::DeclarationList declarations(
                                allocatorFor<VariableDeclaration>());
        AssignmentExpressionNode *initExpr = nullptr;

        bool moreVars = true;
        if (isInitVarDecl) {
            initExpr = tryParseAssignmentExpression();
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
                    initExpr = tryParseAssignmentExpression();
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
    BlockNode::SourceElementList list(allocatorFor<SourceElementNode *>());
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
Parser::parseReturnStatement()
{
    Token retToken = tokenizer_.lastToken();

    // Return keyword already encountered.
    const Token &tok = nextToken();

    if (retToken.newlineOccursBefore(tok)) {
        // Implicit semicolon.
        tokenizer_.pushBackLastToken();
        return make<ReturnStatementNode>(nullptr);
    }

    if (tok.isSemicolon()) {
        tok.debug_markUsed();
        return make<ReturnStatementNode>(nullptr);
    }

    // Parse return expression.
    tokenizer_.pushBackLastToken();
    ExpressionNode *node = tryParseExpression();
    if (!node)
        emitError("Invalid return expression.");

    return make<ReturnStatementNode>(node);
}

BreakStatementNode *
Parser::parseBreakStatement()
{
    Token breakToken = tokenizer_.lastToken();

    // Break keyword already encountered.
    const Token &tok = nextToken();

    if (breakToken.newlineOccursBefore(tok)) {
        // Implicit semicolon.
        tokenizer_.pushBackLastToken();
        return make<BreakStatementNode>();
    }

    if (tok.isSemicolon()) {
        tok.debug_markUsed();
        return make<BreakStatementNode>();
    }

    if (tok.isIdentifierName()) {
        IdentifierNameToken ident(tok);
        const Token &tok2 = nextToken();
        if (ident.newlineOccursBefore(tok2)) {
            tokenizer_.pushBackLastToken();
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
Parser::parseContinueStatement()
{
    Token continueToken = tokenizer_.lastToken();

    // Continue keyword already encountered.
    const Token &tok = nextToken();

    if (continueToken.newlineOccursBefore(tok)) {
        // Implicit semicolon.
        tokenizer_.pushBackLastToken();
        return make<ContinueStatementNode>();
    }

    if (tok.isSemicolon()) {
        tok.debug_markUsed();
        return make<ContinueStatementNode>();
    }

    if (tok.isIdentifierName()) {
        IdentifierNameToken ident(tok);
        const Token &tok2 = nextToken();
        if (ident.newlineOccursBefore(tok2)) {
            tokenizer_.pushBackLastToken();
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
    /* TODO: Implement. */
    return emitError("Switch statement not implemented yet.");
}

TryStatementNode *
Parser::parseTryStatement()
{
    /* TODO: Implement. */
    return emitError("Try statement not implemented yet.");
}

ThrowStatementNode *
Parser::parseThrowStatement()
{
    /* TODO: Implement. */
    return emitError("Throw statement not implemented yet.");
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
    /* TODO: Implement. */
    return emitError("Debugger statement not implemented yet.");
}

LabelledStatementNode *
Parser::tryParseLabelledStatement()
{
    /* TODO: Implement. */
    return emitError("Labelled statement not implemented yet.");
}

ExpressionStatementNode *
Parser::tryParseExpressionStatement()
{
    ExpressionNode *expr = tryParseExpression();
    if (!expr)
        return nullptr;

    return make<ExpressionStatementNode>(expr);
}

ExpressionNode *
Parser::tryParseExpression(bool forbidIn, Precedence prec)
{
    TokenizerMark markExpr = tokenizer_.mark();

    for (;;) {
        // Read first token.
        // Allow RegExps and keywords.
        const Token &tok = nextToken(Tokenizer::InputElement_RegExp, true);
        tok.debug_markUsed();

        // Handle most common tokens first: identifiers, literals, etc..
        // Which are themselves atomic expressions.
        ExpressionNode *atom = nullptr;
        if (tok.isIdentifierName()) {
            atom = make<IdentifierNode>(IdentifierNameToken(tok));

        } else if (tok.isNumericLiteral()) {
            atom = make<NumericLiteralNode>(NumericLiteralToken(tok));

        } else if (tok.isStringLiteral()) {
            atom = make<StringLiteralNode>(StringLiteralToken(tok));

        } else if (tok.isFalseLiteral()) {
            atom = make<BooleanLiteralNode>(FalseLiteralToken(tok));

        } else if (tok.isTrueLiteral()) {
            atom = make<BooleanLiteralNode>(TrueLiteralToken(tok));

        } else if (tok.isNullLiteral()) {
            atom = make<NullLiteralNode>(NullLiteralToken(tok));

        } else if (tok.isRegularExpressionLiteral()) {
            atom = make<RegularExpressionLiteralNode>(
                            RegularExpressionLiteralToken(tok));

        } else if (tok.isThisKeyword()) {
            atom = make<ThisNode>(ThisKeywordToken(tok));

        } else if (tok.isOpenParen()) {
            // Parse sub-expression
            ExpressionNode *subexpr = tryParseExpression();
            if (!subexpr)
                emitError("Could not parse expression within parenthesis.");
            atom = make<ParenthesizedExpressionNode>(subexpr);

        } else if (tok.isOpenBracket()) {
            // Parse an array literal.
            atom = tryParseArrayLiteral();
            if (!atom)
                emitError("Could not parse array literal.");

        } else if (tok.isOpenBrace()) {
            // Parse object literal.  This can't be a block statement because
            // that has already been tried before.
            atom = tryParseObjectLiteral();
            if (!atom)
                emitError("Could not parse object literal.");

        } else if (tok.isFunctionKeyword()) {
            // Parse function literal.
            atom = tryParseFunction();
            if (!atom)
                emitError("Could not parse function literal.");

        } else if (tok.isNewKeyword()) {
            ExpressionNode *cons = tryParseExpression(forbidIn, Prec_New);
            if (!cons)
                emitError("Could not parse new expression.");

            NewExpressionNode::ExpressionList args(
                                allocatorFor<ExpressionNode *>());
            if (checkNextToken<Token::OpenParen>())
                parseArguments(args);

            atom = make<NewExpressionNode>(cons, args);

        } else if (tok.isLogicalNot()) {
            ExpressionNode *expr = tryParseExpression(forbidIn, Prec_Unary);
            if (!expr)
                emitError("Error parsing unary expression.");

            atom = make<LogicalNotExpressionNode>(expr);

        } else if (tok.isMinus()) {
            ExpressionNode *expr = tryParseExpression(forbidIn, Prec_Unary);
            if (!expr)
                emitError("Error parsing unary expression.");

            atom = make<NegativeExpressionNode>(expr);

        } else if (tok.isPlus()) {
            ExpressionNode *expr = tryParseExpression(forbidIn, Prec_Unary);
            if (!expr)
                emitError("Error parsing unary expression.");

            atom = make<PositiveExpressionNode>(expr);

        } else if (tok.isTilde()) {
            ExpressionNode *expr = tryParseExpression(forbidIn, Prec_Unary);
            if (!expr)
                emitError("Error parsing unary expression.");

            atom = make<BitNotExpressionNode>(expr);

        } else if (tok.isPlusPlus()) {
            ExpressionNode *expr = tryParseExpression(forbidIn, Prec_Unary);
            if (!expr)
                emitError("Error parsing unary expression.");

            atom = make<PreIncrementExpressionNode>(expr);

        } else if (tok.isMinusMinus()) {
            ExpressionNode *expr = tryParseExpression(forbidIn, Prec_Unary);
            if (!expr)
                emitError("Error parsing unary expression.");

            atom = make<PreDecrementExpressionNode>(expr);

        } else if (tok.isDeleteKeyword()) {
            ExpressionNode *expr = tryParseExpression(forbidIn, Prec_Unary);
            if (!expr)
                emitError("Error parsing unary expression.");

            atom = make<DeleteExpressionNode>(expr);

        } else if (tok.isVoidKeyword()) {
            ExpressionNode *expr = tryParseExpression(forbidIn, Prec_Unary);
            if (!expr)
                emitError("Error parsing unary expression.");

            atom = make<VoidExpressionNode>(expr);

        } else if (tok.isTypeOfKeyword()) {
            ExpressionNode *expr = tryParseExpression(forbidIn, Prec_Unary);
            if (!expr)
                emitError("Error parsing unary expression.");

            atom = make<TypeOfExpressionNode>(expr);

        } else {
            // No expression possible, return.
            tokenizer_.gotoMark(markExpr);
            return nullptr;
        }

        // Check next token.
        const Token &tok2 = nextToken(Tokenizer::InputElement_Div, true);

        // If next token is within predecence, then fold into atom.
        // Check for:
        //      OpenBracket         - GET_ELEMENT
        //      Dot                 - GET_PROPERTY
        //      OpenParen           - CALL
        //      PlusPlus            - INCREMENT
        //      MinusMinus          - DECREMENT
        //      Star,Percent,Divide - MULTIPLICATIVE
        //      Plus,Minus          - ADDITIVE
        //      ShiftLeft,ShiftRight,ShiftUnsignedRight - SHIFT
        //      ... FIXME HERE

    }

    /* TODO: Implement. */
    return nullptr;
}

ArrayLiteralNode *
Parser::tryParseArrayLiteral()
{
    // NOTE: Open bracket already read.

    /* TODO: Implement. */
    return emitError("Array literal parsing not yet implemented.");
}

ObjectLiteralNode *
Parser::tryParseObjectLiteral()
{
    // NOTE: Open brace already read.

    /* TODO: Implement. */
    return emitError("Object literal parsing not yet implemented.");
}

FunctionExpressionNode *
Parser::tryParseFunction()
{
    // NOTE: Function keyword already read.

    Maybe<IdentifierNameToken> name;

    const Token &tok0 = nextToken(Tokenizer::InputElement_Div, true);
    if (tok0.isOpenParen()) {
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
    if (!tok1.isCloseParen()) {
        for (;;) {
            if (tok1.isKeyword(strict_))
                emitError("Function formal can't be a keyword.");
            
            if (!tok1.isIdentifierName())
                emitError("Function formal argument expected.");

            params.push_back(IdentifierNameToken(tok1));

            const Token &tok1 = nextToken(Tokenizer::InputElement_Div, false);

            if (tok1.isCloseParen())
                break;

            if (!tok1.isComma())
                emitError("Bad token following function formal argument.");
        }
    }

    // Open brace must follow.
    if (!checkNextToken<Token::OpenBrace>())
        emitError("No open brace after function signature.");

    // Now parse function body.
    FunctionExpressionNode::SourceElementList body(
                            allocatorFor<SourceElementNode *>());
    for (;;) {
        SourceElementNode *node = tryParseSourceElement();
        if (!node)
            break;

        body.push_back(node);
    }

    if (!checkNextToken<Token::CloseBrace>())
        emitError("Invalid function body statement.");

    if (name) {
        return make<FunctionExpressionNode>(*name,
                                            std::move(params),
                                            std::move(body));
    }

    return make<FunctionExpressionNode>(std::move(params), std::move(body));
}

void
Parser::parseArguments(NewExpressionNode::ExpressionList &list)
{
    // FIXME TODO: Implement
}

void
Parser::pushBackAutomaticSemicolon()
{
    WH_ASSERT(!hasAutomaticSemicolon_);
    automaticSemicolon_ = tokenizer_.getAutomaticSemicolon();
    hasAutomaticSemicolon_ = true;
}

const Token &
Parser::nextToken(Tokenizer::InputElementKind kind, bool checkKeywords)
{
    if (hasAutomaticSemicolon_) {
        hasAutomaticSemicolon_ = false;
        return automaticSemicolon_;
    }

    // Read next valid token.
    for (;;) {
        const Token &tok = tokenizer_.readToken(kind, checkKeywords);
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
