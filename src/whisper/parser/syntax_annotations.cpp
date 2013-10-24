
#include "parser/syntax_annotations.hpp"
#include "parser/syntax_tree.hpp"
#include "parser/syntax_tree_inlines.hpp"

#include <stdlib.h>

namespace Whisper {
namespace AST {


bool
SyntaxAnnotator::annotate()
{
    try {
        annotate(root_, nullptr);
        return true;
    } catch (SyntaxAnnotatorError &err) {
        WH_ASSERT(hasError());
        return false;
    }
}

void
SyntaxAnnotator::annotate(BaseNode *node, BaseNode *parent)
{
    WH_ASSERT(node != nullptr);
    switch (node->type()) {
#define CASE_(type) \
      case NodeType::type: \
        annotate##type(node->to##type(), parent); \
        break;
      WHISPER_DEFN_SYNTAX_NODES(CASE_);
#undef CASE_
      default:
        WH_UNREACHABLE("Invalid node type.");
    }
}

void
SyntaxAnnotator::annotateThis(ThisNode *node, BaseNode *parent)
{
}

void
SyntaxAnnotator::annotateIdentifier(IdentifierNode *node, BaseNode *parent)
{
}

void
SyntaxAnnotator::annotateNullLiteral(NullLiteralNode *node, BaseNode *parent)
{
}

void
SyntaxAnnotator::annotateBooleanLiteral(BooleanLiteralNode *node,
                                        BaseNode *parent)
{
}

void
SyntaxAnnotator::annotateNumericLiteral(NumericLiteralNode *node,
                                        BaseNode *parent)
{
    //
    // FIXME FIXME  Handle hex, values. FIXME FIXME
    //

    // Parse the numeric literal.
    const NumericLiteralToken &tok = node->value();
    const char *text = reinterpret_cast<const char *>(tok.text(source_));
    uint32_t length = tok.length();

    WH_ASSERT(length > 0);

    if (tok.hasFlag(Token::Numeric_Double)) {
        // Parse a double out of it.
        double dval = strtod(text, nullptr);
        node->setAnnotation(make<NumericLiteralAnnotation>(dval));
        return;
    }

    // Try to parse an int32
    const char *cur = text;
    bool neg = false;
    if (*cur == '-') {
        neg = true;
        cur += 1;
    }
    constexpr int32_t LIMIT = 0x07FFFFFFu;

    bool failed = false;
    int32_t accum = 0;
    while (cur < text + length) {
        uint8_t digit = *cur;
        accum *= 10;
        accum += (digit - '0');
        if (accum > LIMIT) {
            failed = true;
            break;
        }
    }

    // Check if successfully parsed int32.
    if (!failed) {
        if (neg)
            accum *= -1;
        node->setAnnotation(make<NumericLiteralAnnotation>(accum));
        return;
    }

    // Otherwise, integer literal was close to limits.  Use strtod.
    double dval = strtod(text, nullptr);
    if (dval < INT32_MIN || dval > INT32_MAX) {
        node->setAnnotation(make<NumericLiteralAnnotation>(dval));
        return;
    }

    // Otherwise, value fits within an int32.. use it.
    WH_ASSERT((dval - ToInt32(dval)) == 0);
    accum = ToInt32(dval);
    node->setAnnotation(make<NumericLiteralAnnotation>(accum));
}

void
SyntaxAnnotator::annotateStringLiteral(StringLiteralNode *node,
                                       BaseNode *parent)
{}

void
SyntaxAnnotator::annotateRegularExpressionLiteral(
        RegularExpressionLiteralNode *node, BaseNode *parent)
{}

void
SyntaxAnnotator::annotateArrayLiteral(ArrayLiteralNode *node, BaseNode *parent)
{
    for (ExpressionNode *expr : node->elements()) {
        // expr may be null for array hole entries (e.g. [a,,b])
        if (expr)
            annotate(expr, node);
    }
}

void
SyntaxAnnotator::annotateObjectLiteral(ObjectLiteralNode *node,
                                       BaseNode *parent)
{
    emitError("Cannot handle object literal yet!");
}

void
SyntaxAnnotator::annotateParenthesizedExpression(
        ParenthesizedExpressionNode *node, BaseNode *parent)
{
    annotate(node->subexpression(), node);
}

void
SyntaxAnnotator::annotateFunctionExpression(
        FunctionExpressionNode *node, BaseNode *parent)
{
    for (SourceElementNode *sourceElem : node->functionBody()) {
        WH_ASSERT(sourceElem != nullptr);
        annotate(sourceElem, node);
    }
}

void
SyntaxAnnotator::annotateGetElementExpression(
        GetElementExpressionNode *node, BaseNode *parent)
{
    annotate(node->object(), node);
    annotate(node->element(), node);
}

void
SyntaxAnnotator::annotateGetPropertyExpression(
        GetPropertyExpressionNode *node, BaseNode *parent)
{
    annotate(node->object(), node);
}

void
SyntaxAnnotator::annotateNewExpression(
        NewExpressionNode *node, BaseNode *parent)
{
    annotate(node->constructor(), node);
    for (ExpressionNode *arg : node->arguments()) {
        WH_ASSERT(arg != nullptr);
        annotate(arg, node);
    }
}

void
SyntaxAnnotator::annotateCallExpression(
        CallExpressionNode *node, BaseNode *parent)
{
    annotate(node->function(), node);
    for (ExpressionNode *arg : node->arguments()) {
        WH_ASSERT(arg != nullptr);
        annotate(arg, node);
    }
}

#define DEF_UNARY_(name) \
void \
SyntaxAnnotator::annotate##name(name##Node *node, BaseNode *parent) \
{ \
    annotate(node->subexpression(), node); \
}

DEF_UNARY_(PostIncrementExpression)
DEF_UNARY_(PreIncrementExpression)
DEF_UNARY_(PostDecrementExpression)
DEF_UNARY_(PreDecrementExpression)
DEF_UNARY_(DeleteExpression)
DEF_UNARY_(VoidExpression)
DEF_UNARY_(TypeOfExpression)
DEF_UNARY_(PositiveExpression)
DEF_UNARY_(NegativeExpression)
DEF_UNARY_(BitNotExpression)
DEF_UNARY_(LogicalNotExpression)

#undef DEF_UNARY_


#define DEF_BINARY_(name) \
void \
SyntaxAnnotator::annotate##name(name##Node *node, BaseNode *parent) \
{ \
    annotate(node->lhs(), node); \
    annotate(node->rhs(), node); \
}

DEF_BINARY_(MultiplyExpression)
DEF_BINARY_(DivideExpression)
DEF_BINARY_(ModuloExpression)
DEF_BINARY_(AddExpression)
DEF_BINARY_(SubtractExpression)
DEF_BINARY_(LeftShiftExpression)
DEF_BINARY_(RightShiftExpression)
DEF_BINARY_(UnsignedRightShiftExpression)
DEF_BINARY_(LessThanExpression)
DEF_BINARY_(GreaterThanExpression)
DEF_BINARY_(LessEqualExpression)
DEF_BINARY_(GreaterEqualExpression)
DEF_BINARY_(InstanceOfExpression)
DEF_BINARY_(InExpression)
DEF_BINARY_(EqualExpression)
DEF_BINARY_(NotEqualExpression)
DEF_BINARY_(StrictEqualExpression)
DEF_BINARY_(StrictNotEqualExpression)
DEF_BINARY_(BitAndExpression)
DEF_BINARY_(BitXorExpression)
DEF_BINARY_(BitOrExpression)
DEF_BINARY_(LogicalAndExpression)
DEF_BINARY_(LogicalOrExpression)
DEF_BINARY_(CommaExpression)

#undef DEF_BINARY_

void
SyntaxAnnotator::annotateConditionalExpression(
        ConditionalExpressionNode *node, BaseNode *parent)
{
    annotate(node->condition(), node);
    annotate(node->trueExpression(), node);
    annotate(node->falseExpression(), node);
}


#define DEF_ASSIGN_(name) \
void \
SyntaxAnnotator::annotate##name(name##Node *node, BaseNode *parent) \
{ \
    annotate(node->lhs(), node); \
    annotate(node->rhs(), node); \
}

DEF_ASSIGN_(AssignExpression)
DEF_ASSIGN_(AddAssignExpression)
DEF_ASSIGN_(SubtractAssignExpression)
DEF_ASSIGN_(MultiplyAssignExpression)
DEF_ASSIGN_(ModuloAssignExpression)
DEF_ASSIGN_(LeftShiftAssignExpression)
DEF_ASSIGN_(RightShiftAssignExpression)
DEF_ASSIGN_(UnsignedRightShiftAssignExpression)
DEF_ASSIGN_(BitAndAssignExpression)
DEF_ASSIGN_(BitOrAssignExpression)
DEF_ASSIGN_(BitXorAssignExpression)
DEF_ASSIGN_(DivideAssignExpression)

#undef DEF_ASSIGN_

void
SyntaxAnnotator::annotateBlock(BlockNode *node, BaseNode *parent)
{
    for (SourceElementNode *sourceElem : node->sourceElements()) {
        WH_ASSERT(sourceElem != nullptr);
        annotate(sourceElem, node);
    }
}

void
SyntaxAnnotator::annotateVariableStatement(
        VariableStatementNode *node, BaseNode *parent)
{
    for (const VariableDeclaration &decl : node->declarations()) {
        if (decl.initialiser())
            annotate(decl.initialiser(), node);
    }
}

void
SyntaxAnnotator::annotateEmptyStatement(
        EmptyStatementNode *node, BaseNode *parent)
{
}

void
SyntaxAnnotator::annotateExpressionStatement(
        ExpressionStatementNode *node, BaseNode *parent)
{
    annotate(node->expression(), node);
}

void
SyntaxAnnotator::annotateIfStatement(IfStatementNode *node, BaseNode *parent)
{
    annotate(node->condition(), node);
    annotate(node->trueBody(), node);
    annotate(node->falseBody(), node);
}

void
SyntaxAnnotator::annotateDoWhileStatement(
        DoWhileStatementNode *node, BaseNode *parent)
{
    annotate(node->body(), node);
    annotate(node->condition(), node);
}

void
SyntaxAnnotator::annotateWhileStatement(
        WhileStatementNode *node, BaseNode *parent)
{
    annotate(node->condition(), node);
    annotate(node->body(), node);
}

void
SyntaxAnnotator::annotateForLoopStatement(
        ForLoopStatementNode *node, BaseNode *parent)
{
    if (node->initial())
        annotate(node->initial(), node);

    if (node->condition())
        annotate(node->condition(), node);

    if (node->update())
        annotate(node->update(), node);

    annotate(node->body(), node);
}

void
SyntaxAnnotator::annotateForLoopVarStatement(
        ForLoopVarStatementNode *node, BaseNode *parent)
{
    for (const VariableDeclaration &decl : node->initial()) {
        if (decl.initialiser())
            annotate(decl.initialiser(), node);
    }

    if (node->condition())
        annotate(node->condition(), node);

    if (node->update())
        annotate(node->update(), node);

    annotate(node->body(), node);
}

void
SyntaxAnnotator::annotateForInStatement(
        ForInStatementNode *node, BaseNode *parent)
{
    annotate(node->lhs(), node);
    annotate(node->object(), node);
    annotate(node->body(), node);
}

void
SyntaxAnnotator::annotateForInVarStatement(
        ForInVarStatementNode *node, BaseNode *parent)
{
    annotate(node->object(), node);
    annotate(node->body(), node);
}

void
SyntaxAnnotator::annotateContinueStatement(
        ContinueStatementNode *node, BaseNode *parent)
{
}

void
SyntaxAnnotator::annotateBreakStatement(
        BreakStatementNode *node, BaseNode *parent)
{
}

void
SyntaxAnnotator::annotateReturnStatement(
        ReturnStatementNode *node, BaseNode *parent)
{
    if (node->value())
        annotate(node->value(), node);
}

void
SyntaxAnnotator::annotateWithStatement(
        WithStatementNode *node, BaseNode *parent)
{
    annotate(node->value(), node);
    annotate(node->body(), node);
}

void
SyntaxAnnotator::annotateSwitchStatement(
        SwitchStatementNode *node, BaseNode *parent)
{
    annotate(node->value(), node);
    for (const SwitchStatementNode::CaseClause &clause : node->caseClauses()) {
        if (clause.expression())
            annotate(clause.expression(), node);
        for (StatementNode *stmt : clause.statements())
            annotate(stmt, node);
    }
}

void
SyntaxAnnotator::annotateLabelledStatement(
        LabelledStatementNode *node, BaseNode *parent)
{
    annotate(node->statement(), node);
}

void
SyntaxAnnotator::annotateThrowStatement(
        ThrowStatementNode *node, BaseNode *parent)
{
    annotate(node->value(), node);
}

void
SyntaxAnnotator::annotateTryCatchStatement(
        TryCatchStatementNode *node, BaseNode *parent)
{
    annotate(node->tryBlock(), node);
    annotate(node->catchBlock(), node);
}

void
SyntaxAnnotator::annotateTryFinallyStatement(
        TryFinallyStatementNode *node, BaseNode *parent)
{
    annotate(node->tryBlock(), node);
    annotate(node->finallyBlock(), node);
}

void
SyntaxAnnotator::annotateTryCatchFinallyStatement(
        TryCatchFinallyStatementNode *node, BaseNode *parent)
{
    annotate(node->tryBlock(), node);
    annotate(node->catchBlock(), node);
    annotate(node->finallyBlock(), node);
}

void
SyntaxAnnotator::annotateDebuggerStatement(
        DebuggerStatementNode *node, BaseNode *parent)
{
}

void
SyntaxAnnotator::annotateProgram(ProgramNode *node, BaseNode *parent)
{
    // Program nodes are always root nodes.
    WH_ASSERT(parent == nullptr);

    for (SourceElementNode *sourceElem : node->sourceElements()) {
        annotate(sourceElem, node);
    }
}

void
SyntaxAnnotator::annotateFunctionDeclaration(FunctionDeclarationNode *node,
                                               BaseNode *parent)
{
    // Visit the FunctionExpression body source elems, but with the
    // declaration node as parent.
    FunctionExpressionNode *func = node->func();
    for (SourceElementNode *sourceElem : func->functionBody()) {
        annotate(sourceElem, node);
    }
}


void
SyntaxAnnotator::emitError(const char *error)
{
    WH_ASSERT(!error_);
    error_ = error;
    throw SyntaxAnnotatorError();
}


} // namespace AST
} // namespace Whisper
