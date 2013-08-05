
#include "parser/syntax_tree.hpp"
#include "parser/syntax_tree_inlines.hpp"

namespace Whisper {
namespace AST {


const char *
NodeTypeString(NodeType nodeType)
{
    switch (nodeType) {
      case INVALID:
        return "INVALID";
    #define DEF_CASE_(node)   case node: return #node;
      WHISPER_DEFN_SYNTAX_NODES(DEF_CASE_)
    #undef DEF_CASE_
      case LIMIT:
        return "LIMIT";
      default:
        return "UNKNOWN";
    }
}

//
// BaseNode
//

bool
BaseNode::isLeftHandSideExpression()
{
    if (isIdentifier() || isGetElementExpression() ||
        isGetPropertyExpression())
    {
        return true;
    }

    if (isParenthesizedExpression()) {
        return toParenthesizedExpression()->subexpression()
                                          ->isLeftHandSideExpression();
    }

    return false;
}

bool
ExpressionNode::isNamedFunction()
{
    if (!isFunctionExpression())
        return false;
    return toFunctionExpression()->name() ? true : false;
}

FunctionExpressionNode *
StatementNode::statementToNamedFunction()
{
    WH_ASSERT(isExpressionStatement());

    ExpressionStatementNode *exprStmt = toExpressionStatement();
    WH_ASSERT(exprStmt->expression()->isFunctionExpression());

    FunctionExpressionNode *fun =
        exprStmt->expression()->toFunctionExpression();
    WH_ASSERT(fun->name());

    return fun;
}


} // namespace AST
} // namespace Whisper
