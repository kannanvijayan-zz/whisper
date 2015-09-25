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
    void Print##ntype(SourceReader const& src, ntype##Node const* n, \
                      Printer pr, int tabDepth);
    WHISPER_DEFN_SYNTAX_NODES(PREDEC_)
#undef PREDEC_

template <typename Printer>
void
PrintNode(SourceReader const& src, BaseNode const* node, Printer pr,
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
PrintToken(SourceReader const& src, Token const& token, Printer pr)
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
PrintParenExpr(SourceReader const& src,
               ParenExprNode const* node,
               Printer pr, int tabDepth)
{
    pr("(");
    PrintNode(src, node->subexpr(), pr, tabDepth);
    pr(")");
}

template <typename Printer>
void
PrintNameExpr(SourceReader const& src,
              NameExprNode const* node,
              Printer pr, int tabDepth)
{
    PrintToken(src, node->name(), pr);
}

template <typename Printer>
void
PrintIntegerExpr(SourceReader const& src,
                 IntegerExprNode const* node,
                 Printer pr, int tabDepth)
{
    PrintToken(src, node->token(), pr);
}

template <typename Printer>
void
PrintDotExpr(SourceReader const& src,
             DotExprNode const* node,
             Printer pr, int tabDepth)
{
    PrintNode(src, node->target(), pr, tabDepth);
    pr(".");
    PrintToken(src, node->name(), pr);
}

template <typename Printer>
void
PrintArrowExpr(SourceReader const& src,
               ArrowExprNode const* node,
               Printer pr, int tabDepth)
{
    PrintNode(src, node->target(), pr, tabDepth);
    pr("->");
    PrintToken(src, node->name(), pr);
}

template <typename Printer>
void
PrintCallExpr(SourceReader const& src,
              CallExprNode const* node,
              Printer pr, int tabDepth)
{
    PrintNode(src, node->callee(), pr, tabDepth);
    pr("(");
    uint32_t i = 0;
    for (Expression const* arg : node->args()) {
        if (i > 0)
            pr(", ");
        PrintNode(src, arg, pr, tabDepth);
        i++;
    }
    pr(")");
}

template <typename Printer>
void
PrintPosExpr(SourceReader const& src,
             PosExprNode const* node,
             Printer pr, int tabDepth)
{
    pr("+");
    PrintNode(src, node->subexpr(), pr, tabDepth);
}

template <typename Printer>
void
PrintNegExpr(SourceReader const& src,
             NegExprNode const* node,
             Printer pr, int tabDepth)
{
    pr("-");
    PrintNode(src, node->subexpr(), pr, tabDepth);
}

template <typename Printer>
void
PrintMulExpr(SourceReader const& src,
             MulExprNode const* node,
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
PrintDivExpr(SourceReader const& src,
             DivExprNode const* node,
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
PrintAddExpr(SourceReader const& src,
             AddExprNode const* node,
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
PrintSubExpr(SourceReader const& src,
             SubExprNode const* node,
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
PrintEmptyStmt(SourceReader const& src, EmptyStmtNode const* node,
               Printer pr, int tabDepth)
{
    pr(";\n");
}

template <typename Printer>
void
PrintExprStmt(SourceReader const& src,
              ExprStmtNode const* node,
              Printer pr, int tabDepth)
{
    PrintNode(src, node->expr(), pr, tabDepth);
    pr(";\n");
}

template <typename Printer>
void
PrintReturnStmt(SourceReader const& src,
                ReturnStmtNode const* node,
                Printer pr, int tabDepth)
{
    pr("return");
    if (node->hasExpr()) {
        pr(" ");
        PrintNode(src, node->expr(), pr, tabDepth);
    }
    pr(";\n");
}

template <typename Printer>
static void
PrintBlock(SourceReader const& src,
           Block const* block,
           Printer pr, int tabDepth)
{
    pr("{\n");
    for (Statement const* stmt : block->statements()) {
        PrintTabDepth(tabDepth+1, pr);
        PrintNode(src, stmt, pr, tabDepth+1);
    }
    PrintTabDepth(tabDepth, pr);
    pr("}");
}

template <typename Printer>
void
PrintIfStmt(SourceReader const& src,
            IfStmtNode const* node,
            Printer pr, int tabDepth)
{
    pr("if (");
    PrintNode(src, node->ifPair().cond(), pr, tabDepth);
    pr(") ");
    PrintBlock(src, node->ifPair().block(), pr, tabDepth);

    for (IfStmtNode::CondPair const& elsifPair : node->elsifPairs()) {
        pr(" elsif (");
        PrintNode(src, elsifPair.cond(), pr, tabDepth);
        pr(") ");
        PrintBlock(src, elsifPair.block(), pr, tabDepth);
    }

    if (node->hasElseBlock()) {
        pr(" else ");
        PrintBlock(src, node->elseBlock(), pr, tabDepth);
    }
    pr("\n");
}

template <typename Printer>
void
PrintDefStmt(SourceReader const& src,
             DefStmtNode const* node,
             Printer pr, int tabDepth)
{
    pr("def ");
    PrintToken(src, node->name(), pr);
    pr("(");
    unsigned i = 0;
    for (IdentifierToken const& paramName : node->paramNames()) {
        if (i > 0)
            pr(", ");
        PrintToken(src, paramName, pr);
        i++;
    }
    pr(") ");
    PrintBlock(src, node->bodyBlock(), pr, tabDepth);
    pr("\n");
}

template <typename Printer>
void
PrintVarStmt(SourceReader const& src,
             VarStmtNode const* node,
             Printer pr, int tabDepth)
{
    pr("var ");
    unsigned i = 0;
    for (BindingStatement::Binding const& binding : node->bindings()) {
        if (i > 0)
            pr(", ");
        PrintToken(src, binding.name(), pr);
        if (binding.hasValue()) {
            pr(" = ");
            PrintNode(src, binding.value(), pr, tabDepth);
        }
        i++;
    }
    pr(";\n");
}

template <typename Printer>
void
PrintConstStmt(SourceReader const& src,
               ConstStmtNode const* node,
               Printer pr, int tabDepth)
{
    pr("const ");
    unsigned i = 0;
    for (BindingStatement::Binding const& binding : node->bindings()) {
        if (i > 0)
            pr(", ");
        PrintToken(src, binding.name(), pr);
        WH_ASSERT(binding.hasValue());
        pr(" = ");
        PrintNode(src, binding.value(), pr, tabDepth);
        i++;
    }
    pr(";\n");
}

template <typename Printer>
void
PrintLoopStmt(SourceReader const& src,
              LoopStmtNode const* node,
              Printer pr, int tabDepth)
{
    pr("loop ");
    PrintBlock(src, node->bodyBlock(), pr, tabDepth);
    pr("\n");
}

template <typename Printer>
void
PrintFile(SourceReader const& src, FileNode const* node,
          Printer pr, int tabDepth)
{
    for (Statement const* stmt : node->statements()) {
        PrintTabDepth(tabDepth, pr);
        PrintNode(src, stmt, pr, tabDepth);
    }
}



} // namespace AST
} // namespace Whisper

#endif // WHISPER__PARSER__SYNTAX_TREE_INLINES_HPP
