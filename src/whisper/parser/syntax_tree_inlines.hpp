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
    void Print##ntype(const SourceReader &src, const ntype##Node *n, \
                      Printer pr, int tabDepth);
    WHISPER_DEFN_SYNTAX_NODES(PREDEC_)
#undef PREDEC_

template <typename Printer>
void
PrintNode(const SourceReader &src, const BaseNode *node, Printer pr,
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
PrintToken(const SourceReader &src, const Token &token, Printer pr)
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
PrintParenExpr(const SourceReader &src,
               const ParenExprNode *node,
               Printer pr, int tabDepth)
{
    pr("(");
    PrintNode(src, node->subexpr(), pr, tabDepth);
    pr(")");
}

template <typename Printer>
void
PrintIdentifierExpr(const SourceReader &src,
                    const IdentifierExprNode *node,
                    Printer pr, int tabDepth)
{
    pr("ID:");
    PrintToken(src, node->token(), pr);
}

template <typename Printer>
void
PrintIntegerLiteralExpr(const SourceReader &src,
                        const IntegerLiteralExprNode *node,
                        Printer pr, int tabDepth)
{
    pr("INT:");
    PrintToken(src, node->token(), pr);
}

template <typename Printer>
void
PrintPosExpr(const SourceReader &src,
             const PosExprNode *node,
             Printer pr, int tabDepth)
{
    pr("+");
    PrintNode(src, node->subexpr(), pr, tabDepth);
}

template <typename Printer>
void
PrintNegExpr(const SourceReader &src,
             const NegExprNode *node,
             Printer pr, int tabDepth)
{
    pr("-");
    PrintNode(src, node->subexpr(), pr, tabDepth);
}

template <typename Printer>
void
PrintMulExpr(const SourceReader &src,
             const MulExprNode *node,
             Printer pr, int tabDepth)
{
    pr("(");
    PrintNode(src, node->lhs(), pr, tabDepth);
    pr(" * ");
    PrintNode(src, node->rhs(), pr, tabDepth);
    pr(")");
}

template <typename Printer>
void
PrintDivExpr(const SourceReader &src,
             const DivExprNode *node,
             Printer pr, int tabDepth)
{
    pr("(");
    PrintNode(src, node->lhs(), pr, tabDepth);
    pr(" / ");
    PrintNode(src, node->rhs(), pr, tabDepth);
    pr(")");
}

template <typename Printer>
void
PrintAddExpr(const SourceReader &src,
             const AddExprNode *node,
             Printer pr, int tabDepth)
{
    pr("(");
    PrintNode(src, node->lhs(), pr, tabDepth);
    pr(" + ");
    PrintNode(src, node->rhs(), pr, tabDepth);
    pr(")");
}

template <typename Printer>
void
PrintSubExpr(const SourceReader &src,
             const SubExprNode *node,
             Printer pr, int tabDepth)
{
    pr("(");
    PrintNode(src, node->lhs(), pr, tabDepth);
    pr(" - ");
    PrintNode(src, node->rhs(), pr, tabDepth);
    pr(")");
}

template <typename Printer>
void
PrintEmptyStmt(const SourceReader &src, const EmptyStmtNode *node,
               Printer pr, int tabDepth)
{
    pr(";\n");
}

template <typename Printer>
void
PrintExprStmt(const SourceReader &src,
              const ExprStmtNode *node,
              Printer pr, int tabDepth)
{
    PrintNode(src, node->expr(), pr, tabDepth);
    pr(";\n");
}

template <typename Printer>
void
PrintFile(const SourceReader &src, const FileNode *node,
          Printer pr, int tabDepth)
{
    for (auto stmt : node->statements()) {
        PrintTabDepth(tabDepth, pr);
        PrintNode(src, stmt, pr, tabDepth);
        pr("\n");
    }
}



} // namespace AST
} // namespace Whisper

#endif // WHISPER__PARSER__SYNTAX_TREE_INLINES_HPP
