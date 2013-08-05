
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

BaseNode::BaseNode(NodeType type)
  : type_(type)
{}

NodeType
BaseNode::type() const
{
    return type_;
}

#define METHODS_(node) \
bool BaseNode::is##node() const { \
    return type_ == node; \
} \
const node##Node *BaseNode::to##node() const { \
    WH_ASSERT(is##node()); \
    return reinterpret_cast<const node##Node *>(this); \
} \
node##Node *BaseNode::to##node() { \
    WH_ASSERT(is##node()); \
    return reinterpret_cast<node##Node *>(this); \
}
WHISPER_DEFN_SYNTAX_NODES(METHODS_)
#undef METHODS_

bool
BaseNode::isStatement() const
{
    return false;
}

bool
BaseNode::isBinaryExpression() const
{
    return false;
}

const BaseBinaryExpressionNode *
BaseNode::toBinaryExpression() const
{
    WH_ASSERT(isBinaryExpression());
    return reinterpret_cast<const BaseBinaryExpressionNode *>(this);
}

BaseBinaryExpressionNode *
BaseNode::toBinaryExpression()
{
    WH_ASSERT(isBinaryExpression());
    return reinterpret_cast<BaseBinaryExpressionNode *>(this);
}

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

//
// SourceElementNode
//

SourceElementNode::SourceElementNode(NodeType type)
  : BaseNode(type)
{}

//
// StatementNode
//

StatementNode::StatementNode(NodeType type)
  : SourceElementNode(type)
{}

bool
StatementNode::isStatement() const
{
    return true;
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

//
// ExpressionNode
//

ExpressionNode::ExpressionNode(NodeType type)
  : BaseNode(type)
{}

bool
ExpressionNode::isNamedFunction()
{
    if (!isFunctionExpression())
        return false;
    return toFunctionExpression()->name() ? true : false;
}

//
// LiteralExpressionNode
//

LiteralExpressionNode::LiteralExpressionNode(NodeType type)
  : ExpressionNode(type)
{}

//
// VariableDeclaration
//

VariableDeclaration::VariableDeclaration(const IdentifierNameToken &name,
                                         ExpressionNode *initialiser)
  : name_(name),
    initialiser_(initialiser)
{}

const IdentifierNameToken &
VariableDeclaration::name() const
{
    return name_;
}

ExpressionNode *
VariableDeclaration::initialiser() const
{
    return initialiser_;
}

//
// ThisNode
//

ThisNode::ThisNode(const ThisKeywordToken &token)
  : ExpressionNode(This),
    token_(token)
{}

const ThisKeywordToken &
ThisNode::token() const
{
    return token_;
}

//
// IdentifierNode
//

IdentifierNode::IdentifierNode(const IdentifierNameToken &token)
  : ExpressionNode(Identifier),
    token_(token)
{}

const IdentifierNameToken &
IdentifierNode::token() const
{
    return token_;
}

//
// NullLiteralNode
//

NullLiteralNode::NullLiteralNode(const NullLiteralToken &token)
  : LiteralExpressionNode(NullLiteral),
    token_(token)
{}

const NullLiteralToken &
NullLiteralNode::token() const
{
    return token_;
}

//
// BooleanLiteralNode
//

BooleanLiteralNode::BooleanLiteralNode(const FalseLiteralToken &token)
  : LiteralExpressionNode(BooleanLiteral),
    token_(token)
{}

BooleanLiteralNode::BooleanLiteralNode(const TrueLiteralToken &token)
  : LiteralExpressionNode(BooleanLiteral),
    token_(token)
{}

bool
BooleanLiteralNode::isFalse() const
{
    return token_.hasFirst();
}

bool
BooleanLiteralNode::isTrue() const
{
    return token_.hasSecond();
}

//
// NumericLiteralNode
//

NumericLiteralNode::NumericLiteralNode(const NumericLiteralToken &value)
  : LiteralExpressionNode(NumericLiteral),
    value_(value)
{}

const NumericLiteralToken
NumericLiteralNode::value() const
{
    return value_;
}

//
// StringLiteralNode
//

StringLiteralNode::StringLiteralNode(const StringLiteralToken &value)
  : LiteralExpressionNode(StringLiteral),
    value_(value)
{}

const StringLiteralToken
StringLiteralNode::value() const
{
    return value_;
}

//
// RegularExpressionLiteralNode
//

RegularExpressionLiteralNode::RegularExpressionLiteralNode(
        const RegularExpressionLiteralToken &value)
  : LiteralExpressionNode(RegularExpressionLiteral),
    value_(value)
{}

const RegularExpressionLiteralToken
RegularExpressionLiteralNode::value() const
{
    return value_;
}

//
// ArrayLiteralNode
//

ArrayLiteralNode::ArrayLiteralNode(ExpressionList &&elements)
  : LiteralExpressionNode(ArrayLiteral),
    elements_(elements)
{}

const ExpressionList &
ArrayLiteralNode::elements() const
{
    return elements_;
}

//
// ObjectLiteralNode
//

// PropertyDefinition

ObjectLiteralNode::PropertyDefinition::PropertyDefinition(
        SlotKind kind, const Token &name)
  : kind_(kind), name_(name)
{
    WH_ASSERT(name_.isIdentifierName() ||
              name_.isStringLiteral() ||
              name_.isNumericLiteral());
}

ObjectLiteralNode::SlotKind
ObjectLiteralNode::PropertyDefinition::kind() const
{
    return kind_;
}

bool
ObjectLiteralNode::PropertyDefinition::isValueSlot() const
{
    return kind_ == Value;
}

bool
ObjectLiteralNode::PropertyDefinition::isGetterSlot() const
{
    return kind_ == Getter;
}

bool
ObjectLiteralNode::PropertyDefinition::isSetterSlot() const
{
    return kind_ == Setter;
}

const ObjectLiteralNode::ValueDefinition *
ObjectLiteralNode::PropertyDefinition::toValueSlot() const
{
    WH_ASSERT(isValueSlot());
    return reinterpret_cast<const ValueDefinition *>(this);
}

const ObjectLiteralNode::GetterDefinition *
ObjectLiteralNode::PropertyDefinition::toGetterSlot() const
{
    WH_ASSERT(isGetterSlot());
    return reinterpret_cast<const GetterDefinition *>(this);
}

const ObjectLiteralNode::SetterDefinition *
ObjectLiteralNode::PropertyDefinition::toSetterSlot() const
{
    WH_ASSERT(isSetterSlot());
    return reinterpret_cast<const SetterDefinition *>(this);
}

bool
ObjectLiteralNode::PropertyDefinition::hasIdentifierName() const
{
    return name_.isIdentifierName();
}

bool
ObjectLiteralNode::PropertyDefinition::hasStringName() const
{
    return name_.isStringLiteral();
}

bool
ObjectLiteralNode::PropertyDefinition::hasNumericName() const
{
    return name_.isNumericLiteral();
}

const IdentifierNameToken &
ObjectLiteralNode::PropertyDefinition::identifierName() const
{
    WH_ASSERT(hasIdentifierName());
    return reinterpret_cast<const IdentifierNameToken &>(name_);
}

const StringLiteralToken &
ObjectLiteralNode::PropertyDefinition::stringName() const
{
    WH_ASSERT(hasStringName());
    return reinterpret_cast<const StringLiteralToken &>(name_);
}

const NumericLiteralToken &
ObjectLiteralNode::PropertyDefinition::numericName() const
{
    WH_ASSERT(hasNumericName());
    return reinterpret_cast<const NumericLiteralToken &>(name_);
}

const Token &
ObjectLiteralNode::PropertyDefinition::name() const
{
    return name_;
}

// ValueDefinition


ObjectLiteralNode::ValueDefinition::ValueDefinition(
        const Token &name, ExpressionNode *value)
  : PropertyDefinition(Value, name), value_(value)
{}

ExpressionNode *
ObjectLiteralNode::ValueDefinition::value() const
{
    return value_;
}

// AccessorDefinition

ObjectLiteralNode::AccessorDefinition::AccessorDefinition(
        SlotKind kind, const Token &name, SourceElementList &&body)
  : PropertyDefinition(kind, name), body_(body)
{}

const SourceElementList &
ObjectLiteralNode::AccessorDefinition::body() const
{
    return body_;
}

// GetterDefinition

ObjectLiteralNode::GetterDefinition::GetterDefinition(
        const Token &name, SourceElementList &&body)
  : AccessorDefinition(Getter, name, std::move(body))
{}

// SetterDefinition

ObjectLiteralNode::SetterDefinition::SetterDefinition(
        const Token &name, const IdentifierNameToken &parameter,
        SourceElementList &&body)
  : AccessorDefinition(Setter, name, std::move(body)),
    parameter_(parameter)
{}

const IdentifierNameToken &
ObjectLiteralNode::SetterDefinition::parameter() const
{
    return parameter_;
}


ObjectLiteralNode::ObjectLiteralNode(
        PropertyDefinitionList &&propertyDefinitions)
  : LiteralExpressionNode(ObjectLiteral),
    propertyDefinitions_(propertyDefinitions)
{}

const ObjectLiteralNode::PropertyDefinitionList &
ObjectLiteralNode::propertyDefinitions() const
{
    return propertyDefinitions_;
}


} // namespace AST
} // namespace Whisper
