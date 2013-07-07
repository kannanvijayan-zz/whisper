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
        Print##ntype<Printer>(src, To##ntype(node), pr, tabDepth); break;
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
PrintDeclarationList(const CodeSource &src, const DeclarationList &decls,
                     Printer pr, int tabDepth)
{
    bool first = true;
    for (auto decl : decls) {
        if (!first)
            pr(", ");
        PrintToken(src, decl.name(), pr);
        if (decl.initialiser()) {
            pr(" = ");
            PrintNode(src, decl.initialiser(), pr, tabDepth);
        }
    };
}

template <typename Printer>
void
PrintThis(const CodeSource &src, const ThisNode *node, Printer pr,
          int tabDepth)
{
    pr("this");
}

template <typename Printer>
void
PrintIdentifier(const CodeSource &src, const IdentifierNode *node, Printer pr,
                int tabDepth)
{
    pr("ID:");
    PrintToken(src, node->token(), pr);
}

template <typename Printer>
void
PrintNullLiteral(const CodeSource &src, const NullLiteralNode *node,
                 Printer pr, int tabDepth)
{
    pr("null");
}

template <typename Printer>
void
PrintBooleanLiteral(const CodeSource &src, const BooleanLiteralNode *node,
                    Printer pr, int tabDepth)
{
    if (node->isFalse())
        pr("false");
    else
        pr("true");
}

template <typename Printer>
void
PrintNumericLiteral(const CodeSource &src, const NumericLiteralNode *node,
                    Printer pr, int tabDepth)
{
    pr("NUM:");
    PrintToken(src, node->value(), pr);
}

template <typename Printer>
void
PrintStringLiteral(const CodeSource &src, const StringLiteralNode *node,
                   Printer pr, int tabDepth)
{
    pr("STR:");
    PrintToken(src, node->value(), pr);
}

template <typename Printer>
void
PrintRegularExpressionLiteral(const CodeSource &src,
                              const RegularExpressionLiteralNode *node,
                              Printer pr, int tabDepth)
{
    pr("RE:");
    PrintToken(src, node->value(), pr);
}

template <typename Printer>
void
PrintArrayLiteral(const CodeSource &src, const ArrayLiteralNode *node,
                  Printer pr, int tabDepth)
{
    pr("[");
    PrintExpressionList(src, node->elements(), pr, tabDepth);
    pr("]");
}

template <typename Printer>
void
PrintObjectLiteral(const CodeSource &src, const ObjectLiteralNode *node,
                   Printer pr, int tabDepth)
{
    pr("{");
    pr("FIXME_OBJECT_LITERAL");
    /*
    bool first = true;
    for (auto propDef : node->propertyDefinitions()) {
        if (!first)
            pr(", ");
        pr("FIXME_PROP");
    }
    */
    pr("}");
}

template <typename Printer>
void
PrintParenthesizedExpression(const CodeSource &src,
                             const ParenthesizedExpressionNode *node,
                             Printer pr, int tabDepth)
{
    pr("(");
    PrintNode(src, node->subexpression(), pr, tabDepth);
    pr(")");
}

template <typename Printer>
void
PrintFunctionExpression(const CodeSource &src,
                        const FunctionExpressionNode *node,
                        Printer pr, int tabDepth)
{
    pr("function ");
    if (node->name())
        PrintToken(src, *node->name(), pr);
    pr("(");
    bool first = true;
    for (auto arg : node->formalParameters()) {
        if (!first)
            pr(",");
        PrintToken(src, arg, pr);
        first = false;
    }
    pr(") {\n");
    for (auto sourceElem : node->functionBody()) {
        PrintTabDepth(tabDepth+1, pr);
        PrintNode(src, sourceElem, pr, tabDepth + 1);
        pr("\n");
    }
    PrintTabDepth(tabDepth, pr);
    pr("}");
}

template <typename Printer>
void
PrintGetElementExpression(const CodeSource &src,
                          const GetElementExpressionNode *node,
                          Printer pr, int tabDepth)
{
    PrintNode(src, node->object(), pr, tabDepth);
    pr("[");
    PrintNode(src, node->element(), pr, tabDepth);
    pr("]");
}

template <typename Printer>
void
PrintGetPropertyExpression(const CodeSource &src,
                           const GetPropertyExpressionNode *node,
                           Printer pr, int tabDepth)
{
    PrintNode(src, node->object(), pr, tabDepth);
    pr(".");
    PrintToken(src, node->property(), pr);
}

template <typename Printer>
void
PrintNewExpression(const CodeSource &src, const NewExpressionNode *node,
                   Printer pr, int tabDepth)
{
    pr("new ");
    PrintNode(src, node->constructor(), pr, tabDepth);
    pr("(");
    PrintExpressionList(src, node->arguments(), pr, tabDepth);
    pr(")");
}

template <typename Printer>
void
PrintCallExpression(const CodeSource &src, const CallExpressionNode *node,
                    Printer pr, int tabDepth)
{
    PrintNode(src, node->function(), pr, tabDepth);
    pr("(");
    PrintExpressionList(src, node->arguments(), pr, tabDepth);
    pr(")");
}

template <typename Printer>
void
PrintUnaryExpression(const CodeSource &src,
                     const ExpressionNode *subexpr,
                     Printer pr, int tabDepth,
                     const char *op, bool post=false)
{
    if (!post)
        pr(op);
    PrintNode(src, subexpr, pr, tabDepth);
    if (post)
        pr(op);
}

template <typename Printer>
void
PrintPostIncrementExpression(const CodeSource &src,
                             const PostIncrementExpressionNode *node,
                             Printer pr, int tabDepth)
{
    PrintUnaryExpression<Printer>(
        src, node->subexpression(), pr, tabDepth, "++", true);
}

template <typename Printer>
void
PrintPreIncrementExpression(const CodeSource &src,
                            const PreIncrementExpressionNode *node,
                            Printer pr, int tabDepth)
{
    PrintUnaryExpression<Printer>(
        src, node->subexpression(), pr, tabDepth, "++", false);
}

template <typename Printer>
void
PrintPostDecrementExpression(const CodeSource &src,
                             const PostDecrementExpressionNode *node,
                             Printer pr, int tabDepth)
{
    PrintUnaryExpression<Printer>(
        src, node->subexpression(), pr, tabDepth, "--", true);
}

template <typename Printer>
void
PrintPreDecrementExpression(const CodeSource &src,
                            const PreDecrementExpressionNode *node,
                            Printer pr, int tabDepth)
{
    PrintUnaryExpression<Printer>(
        src, node->subexpression(), pr, tabDepth, "--", false);
}


template <typename Printer>
void
PrintDeleteExpression(const CodeSource &src,
                      const DeleteExpressionNode *node,
                      Printer pr, int tabDepth)
{
    PrintUnaryExpression<Printer>(
        src, node->subexpression(), pr, tabDepth, "delete ", false);
}

template <typename Printer>
void
PrintVoidExpression(const CodeSource &src,
                    const VoidExpressionNode *node,
                    Printer pr, int tabDepth)
{
    PrintUnaryExpression<Printer>(
        src, node->subexpression(), pr, tabDepth, "void ", false);
}

template <typename Printer>
void
PrintTypeOfExpression(const CodeSource &src,
                      const TypeOfExpressionNode *node,
                      Printer pr, int tabDepth)
{
    PrintUnaryExpression<Printer>(
        src, node->subexpression(), pr, tabDepth, "typeof ", false);
}

template <typename Printer>
void
PrintPositiveExpression(const CodeSource &src,
                        const PositiveExpressionNode *node,
                        Printer pr, int tabDepth)
{
    PrintUnaryExpression<Printer>(
        src, node->subexpression(), pr, tabDepth, "+", false);
}

template <typename Printer>
void
PrintNegativeExpression(const CodeSource &src,
                        const NegativeExpressionNode *node,
                        Printer pr, int tabDepth)
{
    PrintUnaryExpression<Printer>(
        src, node->subexpression(), pr, tabDepth, "-", false);
}

template <typename Printer>
void
PrintBitNotExpression(const CodeSource &src,
                      const BitNotExpressionNode *node,
                      Printer pr, int tabDepth)
{
    PrintUnaryExpression<Printer>(
        src, node->subexpression(), pr, tabDepth, "~", false);
}

template <typename Printer>
void
PrintLogicalNotExpression(const CodeSource &src,
                          const LogicalNotExpressionNode *node,
                          Printer pr, int tabDepth)
{
    PrintUnaryExpression<Printer>(
        src, node->subexpression(), pr, tabDepth, "!", false);
}


template <typename Printer>
void
PrintBinaryExpression(const CodeSource &src,
                      const ExpressionNode *rhs,
                      const ExpressionNode *lhs,
                      Printer pr, int tabDepth,
                      const char *op)
{
    PrintNode(src, lhs, pr, tabDepth);
    pr(" ");
    pr(op);
    pr(" ");
    PrintNode(src, rhs, pr, tabDepth);
}

template <typename Printer>
void
PrintMultiplyExpression(const CodeSource &src,
                        const MultiplyExpressionNode *node,
                        Printer pr, int tabDepth)
{
    PrintBinaryExpression<Printer>(
        src, node->rhs(), node->lhs(), pr, tabDepth, "*");
}

template <typename Printer>
void
PrintDivideExpression(const CodeSource &src,
                      const DivideExpressionNode *node,
                      Printer pr, int tabDepth)
{
    PrintBinaryExpression<Printer>(
        src, node->rhs(), node->lhs(), pr, tabDepth, "/");
}

template <typename Printer>
void
PrintModuloExpression(const CodeSource &src,
                      const ModuloExpressionNode *node,
                      Printer pr, int tabDepth)
{
    PrintBinaryExpression<Printer>(
        src, node->rhs(), node->lhs(), pr, tabDepth, "%");
}

template <typename Printer>
void
PrintAddExpression(const CodeSource &src,
                   const AddExpressionNode *node,
                   Printer pr, int tabDepth)
{
    PrintBinaryExpression<Printer>(
        src, node->rhs(), node->lhs(), pr, tabDepth, "+");
}

template <typename Printer>
void
PrintSubtractExpression(const CodeSource &src,
                        const SubtractExpressionNode *node,
                        Printer pr, int tabDepth)
{
    PrintBinaryExpression<Printer>(
        src, node->rhs(), node->lhs(), pr, tabDepth, "-");
}

template <typename Printer>
void
PrintLeftShiftExpression(const CodeSource &src,
                         const LeftShiftExpressionNode *node,
                         Printer pr, int tabDepth)
{
    PrintBinaryExpression<Printer>(
        src, node->rhs(), node->lhs(), pr, tabDepth, "<<");
}

template <typename Printer>
void
PrintRightShiftExpression(const CodeSource &src,
                          const RightShiftExpressionNode *node,
                          Printer pr, int tabDepth)
{
    PrintBinaryExpression<Printer>(
        src, node->rhs(), node->lhs(), pr, tabDepth, ">>");
}

template <typename Printer>
void
PrintUnsignedRightShiftExpression(const CodeSource &src,
                                  const UnsignedRightShiftExpressionNode *node,
                                  Printer pr, int tabDepth)
{
    PrintBinaryExpression<Printer>(
        src, node->rhs(), node->lhs(), pr, tabDepth, ">>>");
}

template <typename Printer>
void
PrintLessThanExpression(const CodeSource &src,
                        const LessThanExpressionNode *node,
                        Printer pr, int tabDepth)
{
    PrintBinaryExpression<Printer>(
        src, node->rhs(), node->lhs(), pr, tabDepth, "<");
}

template <typename Printer>
void
PrintGreaterThanExpression(const CodeSource &src,
                           const GreaterThanExpressionNode *node,
                           Printer pr, int tabDepth)
{
    PrintBinaryExpression<Printer>(
        src, node->rhs(), node->lhs(), pr, tabDepth, ">");
}

template <typename Printer>
void
PrintLessEqualExpression(const CodeSource &src,
                         const LessEqualExpressionNode *node,
                         Printer pr, int tabDepth)
{
    PrintBinaryExpression<Printer>(
        src, node->rhs(), node->lhs(), pr, tabDepth, "<=");
}

template <typename Printer>
void
PrintGreaterEqualExpression(const CodeSource &src,
                            const GreaterEqualExpressionNode *node,
                            Printer pr, int tabDepth)
{
    PrintBinaryExpression<Printer>(
        src, node->rhs(), node->lhs(), pr, tabDepth, ">=");
}

template <typename Printer>
void
PrintInstanceOfExpression(const CodeSource &src,
                          const InstanceOfExpressionNode *node,
                          Printer pr, int tabDepth)
{
    PrintBinaryExpression<Printer>(
        src, node->rhs(), node->lhs(), pr, tabDepth, "instanceof");
}

template <typename Printer>
void
PrintInExpression(const CodeSource &src,
                  const InExpressionNode *node,
                  Printer pr, int tabDepth)
{
    PrintBinaryExpression<Printer>(
        src, node->rhs(), node->lhs(), pr, tabDepth, "in");
}

template <typename Printer>
void
PrintEqualExpression(const CodeSource &src,
                     const EqualExpressionNode *node,
                     Printer pr, int tabDepth)
{
    PrintBinaryExpression<Printer>(
        src, node->rhs(), node->lhs(), pr, tabDepth, "==");
}

template <typename Printer>
void
PrintNotEqualExpression(const CodeSource &src,
                        const NotEqualExpressionNode *node,
                        Printer pr, int tabDepth)
{
    PrintBinaryExpression<Printer>(
        src, node->rhs(), node->lhs(), pr, tabDepth, "!=");
}

template <typename Printer>
void
PrintStrictEqualExpression(const CodeSource &src,
                           const StrictEqualExpressionNode *node,
                           Printer pr, int tabDepth)
{
    PrintBinaryExpression<Printer>(
        src, node->rhs(), node->lhs(), pr, tabDepth, "===");
}

template <typename Printer>
void
PrintStrictNotEqualExpression(const CodeSource &src,
            const StrictNotEqualExpressionNode *node,
            Printer pr, int tabDepth)
{
    PrintBinaryExpression<Printer>(
        src, node->rhs(), node->lhs(), pr, tabDepth, "!==");
}

template <typename Printer>
void
PrintBitAndExpression(const CodeSource &src,
                      const BitAndExpressionNode *node,
                      Printer pr, int tabDepth)
{
    PrintBinaryExpression<Printer>(
        src, node->rhs(), node->lhs(), pr, tabDepth, "&");
}

template <typename Printer>
void
PrintBitXorExpression(const CodeSource &src,
                      const BitXorExpressionNode *node,
                      Printer pr, int tabDepth)
{
    PrintBinaryExpression<Printer>(
        src, node->rhs(), node->lhs(), pr, tabDepth, "^");
}

template <typename Printer>
void
PrintBitOrExpression(const CodeSource &src,
                     const BitOrExpressionNode *node,
                     Printer pr, int tabDepth)
{
    PrintBinaryExpression<Printer>(
        src, node->rhs(), node->lhs(), pr, tabDepth, "|");
}

template <typename Printer>
void
PrintLogicalAndExpression(const CodeSource &src,
                          const LogicalAndExpressionNode *node,
                          Printer pr, int tabDepth)
{
    PrintBinaryExpression<Printer>(
        src, node->rhs(), node->lhs(), pr, tabDepth, "&&");
}

template <typename Printer>
void
PrintLogicalOrExpression(const CodeSource &src,
                         const LogicalOrExpressionNode *node,
                         Printer pr, int tabDepth)
{
    PrintBinaryExpression<Printer>(
        src, node->rhs(), node->lhs(), pr, tabDepth, "||");
}

template <typename Printer>
void
PrintCommaExpression(const CodeSource &src, const CommaExpressionNode *node,
                     Printer pr, int tabDepth)
{
    PrintBinaryExpression<Printer>(
        src, node->rhs(), node->lhs(), pr, tabDepth, ",");
}

template <typename Printer>
void
PrintConditionalExpression(const CodeSource &src,
                           const ConditionalExpressionNode *node,
                           Printer pr, int tabDepth)
{
    PrintNode(src, node->condition(), pr, tabDepth);
    pr(" ? ");
    PrintNode(src, node->trueExpression(), pr, tabDepth);
    pr(" : ");
    PrintNode(src, node->falseExpression(), pr, tabDepth);
}

template <typename Printer>
void
PrintAssignExpression(const CodeSource &src,
                      const AssignExpressionNode *node,
                      Printer pr, int tabDepth)
{
    PrintBinaryExpression(src, node->lhs(), node->rhs(), pr, tabDepth, "=");
}

template <typename Printer>
void
PrintAddAssignExpression(const CodeSource &src,
                         const AddAssignExpressionNode *node,
                         Printer pr, int tabDepth)
{
    PrintBinaryExpression(src, node->lhs(), node->rhs(), pr, tabDepth, "+=");
}

template <typename Printer>
void
PrintSubtractAssignExpression(const CodeSource &src,
                              const SubtractAssignExpressionNode *node,
                              Printer pr, int tabDepth)
{
    PrintBinaryExpression(src, node->lhs(), node->rhs(), pr, tabDepth, "-=");
}

template <typename Printer>
void
PrintMultiplyAssignExpression(const CodeSource &src,
                              const MultiplyAssignExpressionNode *node,
                              Printer pr, int tabDepth)
{
    PrintBinaryExpression(src, node->lhs(), node->rhs(), pr, tabDepth, "*=");
}

template <typename Printer>
void
PrintModuloAssignExpression(const CodeSource &src,
                            const ModuloAssignExpressionNode *node,
                            Printer pr, int tabDepth)
{
    PrintBinaryExpression(src, node->lhs(), node->rhs(), pr, tabDepth, "%=");
}

template <typename Printer>
void
PrintLeftShiftAssignExpression(const CodeSource &src,
                               const LeftShiftAssignExpressionNode *node,
                               Printer pr, int tabDepth)
{
    PrintBinaryExpression(src, node->lhs(), node->rhs(), pr, tabDepth, "<<=");
}

template <typename Printer>
void
PrintRightShiftAssignExpression(const CodeSource &src,
                                const RightShiftAssignExpressionNode *node,
                                Printer pr, int tabDepth)
{
    PrintBinaryExpression(src, node->lhs(), node->rhs(), pr, tabDepth, ">>=");
}

template <typename Printer>
void
PrintUnsignedRightShiftAssignExpression(
    const CodeSource &src,
    const UnsignedRightShiftAssignExpressionNode *node,
    Printer pr, int tabDepth)
{
    PrintBinaryExpression(src, node->lhs(), node->rhs(), pr, tabDepth, ">>>=");
}

template <typename Printer>
void
PrintBitAndAssignExpression(const CodeSource &src,
                            const BitAndAssignExpressionNode *node,
                            Printer pr, int tabDepth)
{
    PrintBinaryExpression(src, node->lhs(), node->rhs(), pr, tabDepth, "&=");
}

template <typename Printer>
void
PrintBitOrAssignExpression(const CodeSource &src,
                           const BitOrAssignExpressionNode *node,
                           Printer pr, int tabDepth)
{
    PrintBinaryExpression(src, node->lhs(), node->rhs(), pr, tabDepth, "|=");
}

template <typename Printer>
void
PrintBitXorAssignExpression(const CodeSource &src,
                            const BitXorAssignExpressionNode *node,
                            Printer pr, int tabDepth)
{
    PrintBinaryExpression(src, node->lhs(), node->rhs(), pr, tabDepth, "^=");
}

template <typename Printer>
void
PrintDivideAssignExpression(const CodeSource &src,
                            const DivideAssignExpressionNode *node,
                            Printer pr, int tabDepth)
{
    PrintBinaryExpression(src, node->lhs(), node->rhs(), pr, tabDepth, "/=");
}


template <typename Printer>
void
PrintBlock(const CodeSource &src, const BlockNode *node, Printer pr,
           int tabDepth)
{
    pr("{\n");
    for (auto elem : node->sourceElements()) {
        PrintTabDepth(tabDepth+1, pr);
        PrintNode(src, elem, pr, tabDepth + 1);
        pr("\n");
    }
    PrintTabDepth(tabDepth, pr);
    pr("}\n");
}

template <typename Printer>
void
PrintVariableStatement(const CodeSource &src,
                       const VariableStatementNode *node,
                       Printer pr, int tabDepth)
{
    pr("var ");
    PrintDeclarationList(src, node->declarations(), pr, tabDepth);
    pr(";\n");
}

template <typename Printer>
void
PrintEmptyStatement(const CodeSource &src, const EmptyStatementNode *node,
                    Printer pr, int tabDepth)
{
    pr(";\n");
}

template <typename Printer>
void
PrintExpressionStatement(const CodeSource &src,
                         const ExpressionStatementNode *node,
                         Printer pr, int tabDepth)
{
    PrintNode(src, node->expression(), pr, tabDepth);
    pr(";\n");
}

template <typename Printer>
void
PrintIfStatement(const CodeSource &src, const IfStatementNode *node,
                 Printer pr, int tabDepth)
{
    pr("if (");
    PrintNode(src, node->condition(), pr, tabDepth+1);
    pr(")\n");
    PrintTabDepth(tabDepth + 1, pr);
    PrintNode(src, node->trueBody(), pr, tabDepth+1);
    if (node->falseBody()) {
        PrintTabDepth(tabDepth, pr);
        pr("else\n");
        PrintTabDepth(tabDepth + 1, pr);
        PrintNode(src, node->falseBody(), pr, tabDepth+1);
    }
}

template <typename Printer>
void
PrintDoWhileStatement(const CodeSource &src, const DoWhileStatementNode *node,
                      Printer pr, int tabDepth)
{
    pr("do\n");
    PrintTabDepth(tabDepth, pr);
    PrintNode(src, node->body(), pr, tabDepth+1);
    pr("while (");
    PrintNode(src, node->condition(), pr, tabDepth+1);
    pr(");\n");
}

template <typename Printer>
void
PrintWhileStatement(const CodeSource &src, const WhileStatementNode *node,
                    Printer pr, int tabDepth)
{
    pr("while (");
    PrintNode(src, node->condition(), pr, tabDepth+1);
    pr(") {\n");
    PrintTabDepth(tabDepth, pr);
    PrintNode(src, node->body(), pr, tabDepth+1);
}

template <typename Printer>
void
PrintForLoopStatement(const CodeSource &src, const ForLoopStatementNode *node,
                      Printer pr, int tabDepth)
{
    pr("for (");
    if (node->initial())
        PrintNode(src, node->initial(), pr, tabDepth+1);
    pr("; ");
    if (node->condition())
        PrintNode(src, node->condition(), pr, tabDepth+1);
    pr("; ");
    if (node->update())
        PrintNode(src, node->update(), pr, tabDepth+1);
    pr(") {\n");
    PrintTabDepth(tabDepth, pr);
    PrintNode(src, node->body(), pr, tabDepth+1);
}

template <typename Printer>
void
PrintForLoopVarStatement(const CodeSource &src,
                         const ForLoopVarStatementNode *node,
                         Printer pr, int tabDepth)
{
    pr("for (");
    PrintDeclarationList(src, node->initial(), pr, tabDepth);
    pr("; ");
    if (node->condition())
        PrintNode(src, node->condition(), pr, tabDepth+1);
    pr("; ");
    if (node->update())
        PrintNode(src, node->update(), pr, tabDepth+1);
    pr(") {\n");
    PrintTabDepth(tabDepth, pr);
    PrintNode(src, node->body(), pr, tabDepth+1);
}

template <typename Printer>
void
PrintForInStatement(const CodeSource &src, const ForInStatementNode *node,
                    Printer pr, int tabDepth)
{
    pr("for (");
    PrintNode(src, node->lhs(), pr, tabDepth);
    pr(" in ");
    PrintNode(src, node->object(), pr, tabDepth);
    pr(") {\n");
    PrintTabDepth(tabDepth, pr);
    PrintNode(src, node->body(), pr, tabDepth+1);
}

template <typename Printer>
void
PrintForInVarStatement(const CodeSource &src,
                       const ForInVarStatementNode *node,
                       Printer pr, int tabDepth)
{
    pr("for (");
    PrintToken(src, node->name(), pr);
    pr(" in ");
    PrintNode(src, node->object(), pr, tabDepth);
    pr(") {\n");
    PrintTabDepth(tabDepth, pr);
    PrintNode(src, node->body(), pr, tabDepth+1);
}

template <typename Printer>
void
PrintContinueStatement(const CodeSource &src,
                       const ContinueStatementNode *node,
                       Printer pr, int tabDepth)
{
    pr("continue");
    if (node->label()) {
        pr(" ");
        PrintToken(src, *node->label(), pr);
    }
    pr(";\n");
}

template <typename Printer>
void
PrintBreakStatement(const CodeSource &src, const BreakStatementNode *node,
                    Printer pr, int tabDepth)
{
    pr("break");
    if (node->label()) {
        pr(" ");
        PrintToken(src, *node->label(), pr);
    }
    pr(";\n");
}

template <typename Printer>
void
PrintReturnStatement(const CodeSource &src, const ReturnStatementNode *node,
                     Printer pr, int tabDepth)
{
    pr("return");
    if (node->value()) {
        pr(" ");
        PrintNode(src, node->value(), pr, tabDepth);
    }
    pr(";\n");
}

template <typename Printer>
void
PrintWithStatement(const CodeSource &src, const WithStatementNode *node,
                   Printer pr, int tabDepth)
{
    pr("with (");
    PrintNode(src, node->value(), pr, tabDepth);
    pr(")\n");
    PrintTabDepth(tabDepth+1, pr);
    PrintNode(src, node->body(), pr, tabDepth+1);
}

template <typename Printer>
void
PrintSwitchStatement(const CodeSource &src, const SwitchStatementNode *node,
                     Printer pr, int tabDepth)
{
    pr("switch (");
    PrintNode(src, node->value(), pr, tabDepth);
    pr(") {\n");
    for (auto switchCase : node->caseClauses()) {
        PrintTabDepth(tabDepth, pr);
        if (switchCase.expression()) {
            pr("case ");
            PrintNode(src, switchCase.expression(), pr, tabDepth+1);
            pr(":\n");
        } else {
            pr("default:\n");
        }
        for (auto stmt : switchCase.statements()) {
            PrintTabDepth(tabDepth+1, pr);
            PrintNode(src, stmt, pr, tabDepth+1);
        }
    }
    pr("}\n");
}

template <typename Printer>
void
PrintLabelledStatement(const CodeSource &src,
                       const LabelledStatementNode *node,
                       Printer pr, int tabDepth)
{
    PrintToken(src, node->label(), pr);
    pr(":\n");
    PrintTabDepth(tabDepth, pr);
    PrintNode(src, node->statement(), pr, tabDepth);
}

template <typename Printer>
void
PrintThrowStatement(const CodeSource &src, const ThrowStatementNode *node,
                    Printer pr, int tabDepth)
{
    pr("throw ");
    PrintNode(src, node->value(), pr, tabDepth);
    pr(";\n");
}

template <typename Printer>
void
PrintTryCatchStatement(const CodeSource &src,
                       const TryCatchStatementNode *node,
                       Printer pr, int tabDepth)
{
    pr("try ");
    PrintBlock(src, node->tryBlock(), pr, tabDepth + 1);
    PrintTabDepth(tabDepth, pr);
    pr("catch (");
    PrintToken(src, node->catchName(), pr);
    pr(") ");
    PrintBlock(src, node->catchBlock(), pr, tabDepth + 1);
}

template <typename Printer>
void
PrintTryFinallyStatement(const CodeSource &src,
                         const TryFinallyStatementNode *node,
                         Printer pr, int tabDepth)
{
    pr("try ");
    PrintBlock(src, node->tryBlock(), pr, tabDepth + 1);
    PrintTabDepth(tabDepth, pr);
    pr("finally ");
    PrintBlock(src, node->finallyBlock(), pr, tabDepth + 1);
}

template <typename Printer>
void
PrintTryCatchFinallyStatement(const CodeSource &src,
                              const TryCatchFinallyStatementNode *node,
                              Printer pr, int tabDepth)
{
    pr("try ");
    PrintBlock(src, node->tryBlock(), pr, tabDepth + 1);
    PrintTabDepth(tabDepth, pr);
    pr("catch (");
    PrintToken(src, node->catchName(), pr);
    pr(") ");
    PrintBlock(src, node->catchBlock(), pr, tabDepth + 1);
    PrintTabDepth(tabDepth, pr);
    pr("finally ");
    PrintBlock(src, node->finallyBlock(), pr, tabDepth + 1);
}

template <typename Printer>
void
PrintDebuggerStatement(const CodeSource &src,
                       const DebuggerStatementNode *node,
                       Printer pr, int tabDepth)
{
    pr("debugger;\n");
}

template <typename Printer>
void
PrintFunctionDeclaration(const CodeSource &src,
                         const FunctionDeclarationNode *node,
                         Printer pr, int tabDepth)
{
    PrintNode(src, node->func(), pr, tabDepth);
    pr("\n");
}

template <typename Printer>
void
PrintProgram(const CodeSource &src, const ProgramNode *node,
             Printer pr, int tabDepth)
{
    for (auto sourceElem : node->sourceElements()) {
        PrintNode(src, sourceElem, pr, tabDepth);
    }
}



} // namespace AST
} // namespace Whisper

#endif // WHISPER__PARSER__SYNTAX_TREE_INLINES_HPP
