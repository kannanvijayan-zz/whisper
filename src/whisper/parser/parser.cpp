
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
        return tryParseFile();
    } catch (ParserError err) {
        return nullptr;
    }
}

FileNode *
Parser::tryParseFile()
{
    // Try to parse a module declaration.
    ModuleDeclNode *module = tryParseModuleDecl();

    // Try to parse more import declarations.
    FileNode::ImportDeclList imports(allocatorFor<ImportDeclNode *>());
    for (;;) {
        ImportDeclNode *import = tryParseImportDecl();
        if (!import)
            break;

        imports.push_back(import);
    }

    // Parse file contents.
    FileNode::FileDeclarationList decls(allocatorFor<FileDeclarationNode *>());
    for (;;) {
        FileDeclarationNode *decl = tryParseFileDeclaration();
        if (!decl)
            break;

        decls.push_back(decl);
    }

    // Next token must be end of input.
    if (!checkNextToken<Token::End>())
        return emitError("Expected end of input.");

    return make<FileNode>(module, std::move(imports), std::move(decls));
}

ModuleDeclNode *
Parser::tryParseModuleDecl()
{
    //
    // module a.b.c;
    //

    // Check for module keyword
    if (!checkNextToken<Token::ModuleKeyword>())
        return nullptr;

    // Parse module path.
    IdentifierTokenList path(allocatorFor<IdentifierToken>());
    for (;;) {
        const Token &tok = nextToken();

        if (tok.isIdentifier()) {
            path.push_back(IdentifierToken(tok));

            const Token *cont = checkGetNextToken<Token::Dot,
                                                  Token::Semicolon>();
            if (!cont)
                return emitError("Malformed module declaration.");
            cont->debug_markUsed();

            if (cont->isDot())
                continue;

            WH_ASSERT(cont->isSemicolon());
            break;
        }

        // Check for premature end.
        if (tok.isSemicolon() || tok.isEnd()) {
            pushBackLastToken();
            return emitError("Unexpected end of module declaration.");
        }

        pushBackLastToken();
        return emitError("Malformed module declaration.");
    }

    return make<ModuleDeclNode>(std::move(path));
}

ImportDeclNode *
Parser::tryParseImportDecl()
{
    //
    // import a.b.c;
    // import a.b.c as foo;
    //
    // import a.b.c (x as m, y as y, z as qqq);
    // import a.b.c (x as m, y, z as qqq); # equivalent
    //

    // Check for import keyword
    if (!checkNextToken<Token::ImportKeyword>())
        return nullptr;

    // Parse import path.
    IdentifierTokenList path(allocatorFor<IdentifierToken>());
    bool parseAs = false;
    bool parseMembers = false;
    bool gotSemicolon = false;
    for (;;) {
        const Token &tok = nextToken();

        // Expect an identifier.
        if (tok.isIdentifier()) {
            path.push_back(IdentifierToken(tok));

            const Token *cont = checkGetNextToken<Token::Dot,
                                                  Token::Semicolon,
                                                  Token::AsKeyword,
                                                  Token::OpenParen>();
            if (!cont)
                return emitError("Malformed import declaration.");
            cont->debug_markUsed();

            if (cont->isDot())
                continue;

            WH_ASSERT(cont->isSemicolon() || cont->isAsKeyword() ||
                      cont->isOpenParen());

            if (cont->isAsKeyword())
                parseAs = true;
            else if (cont->isOpenParen())
                parseMembers = true;
            else if (cont->isSemicolon())
                gotSemicolon = true;
            break;
        }

        // Check for premature end.
        if (tok.isSemicolon() || tok.isEnd()) {
            pushBackLastToken();
            return emitError("Unexpected end of import declaration.");
        }

        pushBackLastToken();
        return emitError("Malformed import declaration.");
    }

    WH_ASSERT_IF(parseAs, !gotSemicolon && !parseMembers);
    WH_ASSERT_IF(parseMembers, !gotSemicolon && !parseAs);
    WH_ASSERT_IF(gotSemicolon, !parseAs && !parseMembers);

    IdentifierToken asName;
    if (parseAs) {
        const Token *maybeAsName = checkGetNextToken<Token::Identifier>();
        if (!maybeAsName)
            return emitError("Expected name after 'as' in import declaration.");
        asName = IdentifierToken(*maybeAsName);

        const Token *cont = checkGetNextToken<Token::Semicolon, 
                                              Token::OpenParen>();
        if (!cont)
            return emitError("Malformed import declaration.");
        cont->debug_markUsed();

        if (cont->isOpenParen())
            parseMembers = true;
        else if (cont->isSemicolon())
            gotSemicolon = true;
    }

    WH_ASSERT_IF(gotSemicolon, !parseMembers);
    WH_ASSERT_IF(parseMembers, !gotSemicolon);
  
    ImportDeclNode::MemberList members(allocatorFor<ImportDeclNode::Member>());
    if (parseMembers) {
        for (;;) {
            // Get member name.
            const Token *maybeMember = checkGetNextToken<Token::Identifier>();
            if (!maybeMember)
                return emitError("Malformed member clause in import.");
            IdentifierToken member = IdentifierToken(*maybeMember);

            const Token *cont = checkGetNextToken<Token::AsKeyword,
                                                  Token::Comma,
                                                  Token::CloseParen>();
            if (!cont)
                return emitError("Malformed member clause in import.");
            cont->debug_markUsed();

            if (cont->isCloseParen()) {
                members.push_back(ImportDeclNode::Member(member));
                break;
            }

            if (cont->isComma()) {
                members.push_back(ImportDeclNode::Member(member));
                continue;
            }

            WH_ASSERT(cont->isAsKeyword());
            const Token *memberAsName = checkGetNextToken<Token::Identifier>();
            if (!memberAsName) {
                return emitError("Expected identifier after 'as' in import "
                                 "declaration.");
            }

            members.push_back(
                ImportDeclNode::Member(member, IdentifierToken(*memberAsName)));

            cont = checkGetNextToken<Token::Comma, Token::CloseParen>();
            if (!cont)
                return emitError("Malformed member clause in import.");
            cont->debug_markUsed();

            if (cont->isComma())
                continue;

            WH_ASSERT(cont->isCloseParen());
            break;
        }
    }

    if (!gotSemicolon) {
        if (!checkNextToken<Token::Semicolon>())
            return emitError("Expected semicolon after import declaration.");
    }

    return make<ImportDeclNode>(std::move(path), asName, std::move(members));
}

FileDeclarationNode *
Parser::tryParseFileDeclaration()
{
    //
    // func ...;
    // public func ...;
    // private func ...;
    //

    VisibilityToken visibility;

    const Token *tok;

    // Check for visibility prefix.
    {
        const Token &vis = nextToken();
        if (vis.isPublicKeyword() || vis.isPrivateKeyword()) {
            visibility = VisibilityToken(vis);
            const Token &afterVis = nextToken();
            tok = &afterVis;
        } else {
            tok = &vis;
        }
    }

    // Check next token.
    if (tok->isFuncKeyword()) {
        tok->debug_markUsed();
        return tryParseFuncDecl(visibility);
    }

    // If visibility modifier was given, but no recognizable continuation
    // follows, error.
    if (!visibility.isINVALID())
        return emitError("Unknown file declaration after visibility modifier.");

    pushBackLastToken();
    return nullptr;
}

FuncDeclNode *
Parser::tryParseFuncDecl(const VisibilityToken &visibility)
{
    //
    // func F(p1 : T1, p2 : T2, ...) : RT { ... }
    //

    // Function name.
    const Token *maybeName = checkGetNextToken<Token::Identifier>();
    if (!maybeName)
        return emitError("Expected function name.");
    IdentifierToken name(*maybeName);

    // Parameter list.
    if (!checkNextToken<Token::OpenParen>())
        return emitError("Expected '(' after function name.");

    // Parse function parameters.
    FuncDeclNode::ParamList params(allocatorFor<FuncDeclNode::Param>());

    if (!checkNextToken<Token::CloseParen>()) {
        for (;;) {
            // Parse parameter name.
            const Token *maybeParamName =
                checkGetNextToken<Token::Identifier>();
            if (!maybeParamName)
                return emitError("Expected function parameter name.");
            IdentifierToken paramName(*maybeParamName);

            // Parse separating colon.
            if (!checkNextToken<Token::Colon>())
                return emitError("Expected ':' after function parameter.");

            // Parse parameter type.
            TypenameNode *paramType = parseType();
            WH_ASSERT(paramType);

            // Add parameter.
            params.push_back(FuncDeclNode::Param(paramType, paramName));

            // Check for continuation.
            const Token *cont = checkGetNextToken<Token::Comma,
                                                  Token::CloseParen>();
            if (!cont)
                return emitError("Malformed function parameters.");
            cont->debug_markUsed();

            if (cont->isComma())
                continue;

            WH_ASSERT(cont->isCloseParen());
            break;
        }
    }

    // Parse colon after function params, before return type.
    if (!checkNextToken<Token::Colon>())
        return emitError("Expected ':' after function parameter list.");

    // Return type.
    TypenameNode *returnType = parseType();
    WH_ASSERT(returnType);

    // Parse function body.
    if (!checkNextToken<Token::OpenBrace>())
        return emitError("Expected '{' after function name.");

    BlockElementList elems(allocatorFor<BlockElementNode *>());
    for (;;) {
        BlockElementNode *elem = tryParseBlockElement();
        if (elem) {
            elems.push_back(elem);
            continue;
        }

        if (!checkNextToken<Token::CloseBrace>())
            return emitError("Expeced close brace for block.");

        break;
    }

    return make<FuncDeclNode>(visibility, returnType, name,
                              std::move(params),
                              make<BlockNode>(std::move(elems)));
}

TypenameNode *
Parser::parseType()
{
    const Token &tok = nextToken();
    if (tok.isIntKeyword())
        return make<IntTypeNode>(IntKeywordToken(tok));

    return emitError("Unrecognized type.");
}

BlockElementNode *
Parser::tryParseBlockElement()
{
    const Token &tok = nextToken();

    if (tok.isReturnKeyword()) {
        tok.debug_markUsed();
        return parseReturnStmt();
    }

    if (tok.isCloseBrace()) {
        pushBackLastToken();
        return nullptr;
    }
        
    return emitError("Unrecognized block element.");
}

ReturnStmtNode *
Parser::parseReturnStmt()
{
    // Try parsing return expression
    const Token &tok = nextToken();
    if (tok.isSemicolon())
        return make<ReturnStmtNode>(nullptr);

    pushBackLastToken();
    ExpressionNode *expr = parseExpression(Prec_Lowest, Token::Semicolon);
    WH_ASSERT(expr);

    return make<ReturnStmtNode>(expr);
}

ExpressionNode *
Parser::parseExpression(Precedence prec, Token::Type endType)
{
    WH_ASSERT(prec >= Prec_Lowest && prec < Prec_Highest);

    TokenizerMark markExpr = tokenizer_.mark();

    // Read first token.
    // Allow RegExps and keywords.
    const Token &tok = nextToken();
    tok.debug_markUsed();

    ExpressionNode *curExpr = nullptr;

    // Parse initial "starter" expression.
    if (tok.isIdentifier()) {
        curExpr = make<IdentifierExprNode>(IdentifierToken(tok));

    } else if (tok.isIntegerLiteral()) {
        curExpr = make<IntegerLiteralExprNode>(IntegerLiteralToken(tok));

    } else if (tok.isOpenParen()) {
        // Parse sub-expression
        auto subexpr = parseExpression(Prec_Lowest, Token::CloseParen);
        if (!subexpr)
            emitError("Could not parse expression within parenthesis.");
        curExpr = make<ParenExprNode>(subexpr);

    } else {
        // No expression possible, return.
        tokenizer_.gotoMark(markExpr);
        return emitError("Could not parse expression.");
    }

    // Recursively forward-parse expressions.
    for (;;) {
        WH_ASSERT(prec >= Prec_Lowest && prec < Prec_Highest);

        const Token &optok = nextToken();
        optok.debug_markUsed();

        // No matching continuation of expression.  Must be at end.
        if (optok.type() != endType) {
            pushBackLastToken();
            return emitError("Invalid expression.");
        }

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
