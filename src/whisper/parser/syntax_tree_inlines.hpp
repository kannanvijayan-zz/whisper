#ifndef WHISPER__PARSER__SYNTAX_TREE_INLINES_HPP
#define WHISPER__PARSER__SYNTAX_TREE_INLINES_HPP

#include "parser/code_source.hpp"
#include "parser/syntax_tree.hpp"

namespace Whisper {
namespace AST {


////////////////////////////
//                        //
//  Debug Printing Funcs  //
//                        //
////////////////////////////

#define PREDEC_(ntype) \
    template <typename Printer> \
    void Print##ntype(const CodeSource &src, const ntype##Node *n, \
                      Printer pr, int tabDepth);
    WHISPER_DEFN_SYNTAX_NODES(PREDEC_)
#undef PREDEC_

template <typename Printer>
void
PrintNode(const CodeSource &src, const BaseNode *node, Printer pr,
          int tabDepth)
{
    switch (node->type()) {
    #define DEF_CASE_(ntype) \
      case ntype: \
        Print##ntype<Printer>(src, node->to##ntype(), pr, tabDepth); break;
        WHISPER_DEFN_SYNTAX_NODES(DEF_CASE_)
    #undef DEF_CASE_
      default:
        WH_UNREACHABLE("Invalid node type.");
    }
}

template <typename Printer>
void
PrintToken(const CodeSource &src, const Token &token, Printer pr)
{
    pr(token.text(src), token.length());
}

template <typename Printer>
void
PrintTabDepth(int tabDepth, Printer pr)
{
    for (int i = 0; i < tabDepth; i++)
        pr("  ");
}

template <typename Printer>
void
PrintExpressionList(const CodeSource &src, const ExpressionList &list,
                    Printer pr, int tabDepth)
{
    bool first = true;
    for (auto expr : list) {
        if (!first)
            pr(", ");
        PrintNode(src, expr, pr, tabDepth);
        first = false;
    }
}

template <typename Printer>
void
PrintBlockElementList(const CodeSource &src,
                      const BlockElementList &elems,
                      Printer pr, int tabDepth)
{
    for (auto elem : elems) {
        PrintTabDepth(tabDepth, pr);
        PrintNode(src, elem, pr, tabDepth);
        pr("\n");
    }
}

template <typename Printer>
void
PrintModulePath(const CodeSource &src,
                const IdentifierTokenList &path,
                Printer pr)
{
    bool first = true;
    for (auto part : path) {
        if (!first)
            pr(".");
        PrintToken(src, part, pr);
        first = false;
    }
}

template <typename Printer>
void
PrintIntType(const CodeSource &src,
             const IntTypeNode *node,
             Printer pr, int tabDepth)
{
    pr("int");
}

template <typename Printer>
void
PrintParenExpr(const CodeSource &src,
               const ParenExprNode *node,
               Printer pr, int tabDepth)
{
    pr("(");
    PrintNode(src, node->subexpr(), pr, tabDepth);
    pr(")");
}

template <typename Printer>
void
PrintIdentifierExpr(const CodeSource &src,
                    const IdentifierExprNode *node,
                    Printer pr, int tabDepth)
{
    pr("ID:");
    PrintToken(src, node->token(), pr);
}

template <typename Printer>
void
PrintIntegerLiteralExpr(const CodeSource &src,
                        const IntegerLiteralExprNode *node,
                        Printer pr, int tabDepth)
{
    pr("INT:");
    PrintToken(src, node->token(), pr);
}

template <typename Printer>
void
PrintBlock(const CodeSource &src, const BlockNode *node, Printer pr,
           int tabDepth)
{
    pr("{\n");
    PrintBlockElementList(src, node->elements(), pr, tabDepth+1);
    PrintTabDepth(tabDepth, pr);
    pr("}\n");
}

template <typename Printer>
void
PrintEmptyStmt(const CodeSource &src, const EmptyStmtNode *node,
               Printer pr, int tabDepth)
{
    pr(";\n");
}

template <typename Printer>
void
PrintExprStmt(const CodeSource &src,
              const ExprStmtNode *node,
              Printer pr, int tabDepth)
{
    PrintNode(src, node->expr(), pr, tabDepth);
    pr(";\n");
}

template <typename Printer>
void
PrintReturnStmt(const CodeSource &src, const ReturnStmtNode *node,
                Printer pr, int tabDepth)
{
    pr("return");
    if (node->hasValue()) {
        pr(" ");
        PrintNode(src, node->value(), pr, tabDepth);
    }
    pr(";");
}

template <typename Printer>
void
PrintFuncDecl(const CodeSource &src, const FuncDeclNode *node,
              Printer pr, int tabDepth)
{
    if (node->hasVisibility()) {
        PrintToken(src, node->visibility(), pr);
        pr(" ");
    }

    pr("func ");
    PrintToken(src, node->name(), pr);
    pr("(");
    bool first = true;
    for (auto param : node->params()) {
        if (!first)
            pr(",");
        PrintToken(src, param.name(), pr);
        pr(" : ");
        PrintNode(src, param.type(), pr, 0);
        first = false;
    }
    pr(") : ");
    PrintNode(src, node->returnType(), pr, 0);
    pr("\n");
    PrintTabDepth(tabDepth, pr);
    pr("{\n");
    PrintBlockElementList(src, node->body()->elements(), pr, tabDepth+1);
    PrintTabDepth(tabDepth, pr);
    pr("}");
}

template <typename Printer>
void
PrintModuleDecl(const CodeSource &src, const ModuleDeclNode *node,
                Printer pr, int tabDepth)
{
    PrintTabDepth(tabDepth, pr);
    pr("module ");
    PrintModulePath(src, node->path(), pr);
}

template <typename Printer>
void
PrintImportDecl(const CodeSource &src, const ImportDeclNode *node,
                Printer pr, int tabDepth)
{
    PrintTabDepth(tabDepth, pr);
    pr("import ");
    PrintModulePath(src, node->path(), pr);
    if (node->hasAsName()) {
        pr(" as ");
        PrintToken(src, node->asName(), pr);
    }

    if (!node->members().empty()) {
        pr(" (");
        bool first = true;
        for (auto member : node->members()) {
            if (!first)
                pr(", ");
            PrintToken(src, member.name(), pr);
            if (member.hasAsName()) {
                pr(" as ");
                PrintToken(src, member.asName(), pr);
            }
            first = false;
        }
        pr(")");
    }
}

template <typename Printer>
void
PrintFile(const CodeSource &src, const FileNode *node,
          Printer pr, int tabDepth)
{
    if (node->hasModule()) {
        PrintTabDepth(tabDepth, pr);
        PrintNode(src, node->module(), pr, tabDepth);
        pr(";\n\n");
    }
    if (!node->imports().empty()) {
        for (auto import : node->imports()) {
            PrintTabDepth(tabDepth, pr);
            PrintNode(src, import, pr, tabDepth);
            pr(";\n");
        }
        pr("\n");
    }

    for (auto elem : node->contents()) {
        PrintTabDepth(tabDepth, pr);
        PrintNode(src, elem, pr, tabDepth);
        pr("\n");
    }
}



} // namespace AST
} // namespace Whisper

#endif // WHISPER__PARSER__SYNTAX_TREE_INLINES_HPP
