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
PrintNameExpr(const SourceReader &src,
              const NameExprNode *node,
              Printer pr, int tabDepth)
{
    PrintToken(src, node->name(), pr);
}

template <typename Printer>
void
PrintIntegerExpr(const SourceReader &src,
                 const IntegerExprNode *node,
                 Printer pr, int tabDepth)
{
    PrintToken(src, node->token(), pr);
}

template <typename Printer>
void
PrintDotExpr(const SourceReader &src,
             const DotExprNode *node,
             Printer pr, int tabDepth)
{
    PrintNode(src, node->target(), pr, tabDepth);
    pr(".");
    PrintToken(src, node->name(), pr);
}

template <typename Printer>
void
PrintArrowExpr(const SourceReader &src,
               const ArrowExprNode *node,
               Printer pr, int tabDepth)
{
    PrintNode(src, node->target(), pr, tabDepth);
    pr("->");
    PrintToken(src, node->name(), pr);
}

template <typename Printer>
void
PrintCallExpr(const SourceReader &src,
              const CallExprNode *node,
              Printer pr, int tabDepth)
{
    PrintNode(src, node->receiver(), pr, tabDepth);
    pr("(");
    uint32_t i = 0;
    for (const Expression *arg : node->args()) {
        if (i > 0)
            pr(", ");
        PrintNode(src, arg, pr, tabDepth);
        i++;
    }
    pr(")");
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
PrintReturnStmt(const SourceReader &src,
                const ReturnStmtNode *node,
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
PrintBlock(const SourceReader &src,
           const Block *block,
           Printer pr, int tabDepth)
{
    pr("{\n");
    for (const Statement *stmt : block->statements()) {
        PrintTabDepth(tabDepth+1, pr);
        PrintNode(src, stmt, pr, tabDepth+1);
    }
    PrintTabDepth(tabDepth, pr);
    pr("}");
}

template <typename Printer>
void
PrintIfStmt(const SourceReader &src,
            const IfStmtNode *node,
            Printer pr, int tabDepth)
{
    pr("if (");
    PrintNode(src, node->ifPair().cond(), pr, tabDepth);
    pr(") ");
    PrintBlock(src, node->ifPair().block(), pr, tabDepth);

    for (const IfStmtNode::CondPair &elsifPair : node->elsifPairs()) {
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
PrintDefStmt(const SourceReader &src,
             const DefStmtNode *node,
             Printer pr, int tabDepth)
{
    pr("def ");
    PrintToken(src, node->name(), pr);
    pr("(");
    unsigned i = 0;
    for (const IdentifierToken &paramName : node->paramNames()) {
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
PrintVarStmt(const SourceReader &src,
             const VarStmtNode *node,
             Printer pr, int tabDepth)
{
    pr("var ");
    unsigned i = 0;
    for (const BindingStatement::Binding &binding : node->bindings()) {
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
PrintConstStmt(const SourceReader &src,
               const ConstStmtNode *node,
               Printer pr, int tabDepth)
{
    pr("const ");
    unsigned i = 0;
    for (const BindingStatement::Binding &binding : node->bindings()) {
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
PrintLoopStmt(const SourceReader &src,
              const LoopStmtNode *node,
              Printer pr, int tabDepth)
{
    pr("loop ");
    PrintBlock(src, node->bodyBlock(), pr, tabDepth);
    pr("\n");
}

template <typename Printer>
void
PrintFile(const SourceReader &src, const FileNode *node,
          Printer pr, int tabDepth)
{
    for (const Statement *stmt : node->statements()) {
        PrintTabDepth(tabDepth, pr);
        PrintNode(src, stmt, pr, tabDepth);
    }
}



} // namespace AST
} // namespace Whisper

#endif // WHISPER__PARSER__SYNTAX_TREE_INLINES_HPP
