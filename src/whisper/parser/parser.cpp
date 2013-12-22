
#include "spew.hpp"
#include "parser/parser.hpp"

//
// The tokenizer parses a code source into a series of tokens.
//

namespace Whisper {


//
// Parser
//

Parser::Parser(Tokenizer &tokenizer)
  : tokenizer_(tokenizer)
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
        return emitError("Invalid function declaration.");

    return make<FileNode>(module, std::move(imports), std::move(decls));
}

ModuleDeclNode *
Parser::tryParseModuleDecl()
{
    // Check for module keyword
    if (!checkNextToken<Token::ModuleKeyword>())
        return nullptr;

    // Parse module path.
    IdentifierTokenList path(allocatorFor<IdentifierToken>());
    for (;;) {
        const Token &tok = nextToken();

        // Expect an identifier.
        if (tok.isIdentifier()) {
            path.push_back(IdentifierToken(tok));

            const Token *cont = checkGetNextToken<Token::Dot,
                                                  Token::Semicolon>();
            if (!cont)
                return emitError("Malformed module declaration.");

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
                                                  Token::CloseParen>();
            if (!cont)
                return emitError("Malformed member clause in import.");
            cont->debug_markUsed();

            if (cont->isCloseParen()) {
                members.push_back(ImportDeclNode::Member(member));
                break;
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
    // FIXME finish this.
    return nullptr;
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
