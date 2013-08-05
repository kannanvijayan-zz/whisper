
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

//
// ParenthesizedExpressionNode
//

ParenthesizedExpressionNode::ParenthesizedExpressionNode(
        ExpressionNode *subexpression)
  : ExpressionNode(ParenthesizedExpression),
    subexpression_(subexpression)
{}

ExpressionNode *
ParenthesizedExpressionNode::subexpression() const
{
    return subexpression_;
}

//
// FunctionExpressionNode
//

FunctionExpressionNode::FunctionExpressionNode(
        FormalParameterList &&formalParameters,
        SourceElementList &&functionBody)
  : ExpressionNode(FunctionExpression),
    name_(),
    formalParameters_(formalParameters),
    functionBody_(functionBody)
{}

FunctionExpressionNode::FunctionExpressionNode(
        const IdentifierNameToken &name,
        FormalParameterList &&formalParameters,
        SourceElementList &&functionBody)
  : ExpressionNode(FunctionExpression),
    name_(name),
    formalParameters_(formalParameters),
    functionBody_(functionBody)
{}

const Maybe<IdentifierNameToken> &
FunctionExpressionNode::name() const
{
    return name_;
}

const FormalParameterList &
FunctionExpressionNode::formalParameters() const
{
    return formalParameters_;
}

const SourceElementList &
FunctionExpressionNode::functionBody() const
{
    return functionBody_;
}

//
// GetElementExpressionNode
//

GetElementExpressionNode::GetElementExpressionNode(
        ExpressionNode *object, ExpressionNode *element)
  : ExpressionNode(GetElementExpression),
    object_(object),
    element_(element)
{}

ExpressionNode *
GetElementExpressionNode::object() const
{
    return object_;
}

ExpressionNode *
GetElementExpressionNode::element() const
{
    return element_;
}

//
// GetPropertyExpressionNode
//

GetPropertyExpressionNode::GetPropertyExpressionNode(
        ExpressionNode *object, const IdentifierNameToken &property)
  : ExpressionNode(GetPropertyExpression),
    object_(object),
    property_(property)
{}

ExpressionNode *GetPropertyExpressionNode::object() const
{
    return object_;
}

const IdentifierNameToken &GetPropertyExpressionNode::property() const
{
    return property_;
}

//
// NewExpressionNode
//

NewExpressionNode::NewExpressionNode(
        ExpressionNode *constructor, ExpressionList &&arguments)
  : ExpressionNode(NewExpression),
    constructor_(constructor),
    arguments_(arguments)
{}

ExpressionNode *
NewExpressionNode::constructor() const
{
    return constructor_;
}

const ExpressionList &
NewExpressionNode::arguments() const
{
    return arguments_;
}

//
// CallExpressionNode
//

CallExpressionNode::CallExpressionNode(
        ExpressionNode *function, ExpressionList &&arguments)
  : ExpressionNode(CallExpression),
    function_(function),
    arguments_(arguments)
{}

ExpressionNode *
CallExpressionNode::function() const
{
    return function_;
}

const ExpressionList &
CallExpressionNode::arguments() const
{
    return arguments_;
}

//
// BaseUnaryExpressionNode
//

BaseUnaryExpressionNode::BaseUnaryExpressionNode(
        NodeType type, ExpressionNode *subexpression)
  : ExpressionNode(type),
    subexpression_(subexpression)
{}

ExpressionNode *
BaseUnaryExpressionNode::subexpression() const
{
    return subexpression_;
}

//
// BaseBinaryExpressionNode
//

BaseBinaryExpressionNode::BaseBinaryExpressionNode(NodeType type,
                         ExpressionNode *lhs,
                         ExpressionNode *rhs)
  : ExpressionNode(type),
    lhs_(lhs),
    rhs_(rhs)
{}

ExpressionNode *
BaseBinaryExpressionNode::lhs() const
{
    return lhs_;
}

ExpressionNode *
BaseBinaryExpressionNode::rhs() const
{
    return rhs_;
}

bool
BaseBinaryExpressionNode::isBinaryExpression() const
{
    return true;
}

//
// ConditionalExpressionNode
//

ConditionalExpressionNode::ConditionalExpressionNode(
        ExpressionNode *condition,
        ExpressionNode *trueExpression,
        ExpressionNode *falseExpression)
  : ExpressionNode(ConditionalExpression),
    condition_(condition),
    trueExpression_(trueExpression),
    falseExpression_(falseExpression)
{}

ExpressionNode *
ConditionalExpressionNode::condition() const
{
    return condition_;
}

ExpressionNode *
ConditionalExpressionNode::trueExpression() const
{
    return trueExpression_;
}

ExpressionNode *
ConditionalExpressionNode::falseExpression() const
{
    return falseExpression_;
}

//
// BaseAssignmentExpressionNode
//

BaseAssignmentExpressionNode::BaseAssignmentExpressionNode(
        NodeType type, ExpressionNode *lhs, ExpressionNode *rhs)
  : ExpressionNode(type),
    lhs_(lhs),
    rhs_(rhs)
{}

ExpressionNode *
BaseAssignmentExpressionNode::lhs() const
{
    return lhs_;
}

ExpressionNode *
BaseAssignmentExpressionNode::rhs() const
{
    return rhs_;
}

//
// BlockNode
//

BlockNode::BlockNode(SourceElementList &&sourceElements)
  : StatementNode(Block),
    sourceElements_(sourceElements)
{}

const SourceElementList &
BlockNode::sourceElements() const
{
    return sourceElements_;
}

//
// VariableStatementNode
//

VariableStatementNode::VariableStatementNode(DeclarationList &&declarations)
  : StatementNode(VariableStatement),
    declarations_(declarations)
{}

const DeclarationList &
VariableStatementNode::declarations() const
{
    return declarations_;
}

//
// EmptyStatementNode
//

EmptyStatementNode::EmptyStatementNode() : StatementNode(EmptyStatement) {}

//
// ExpressionStatementNode
//

ExpressionStatementNode::ExpressionStatementNode(ExpressionNode *expression)
  : StatementNode(ExpressionStatement),
    expression_(expression)
{}

ExpressionNode *
ExpressionStatementNode::expression() const
{
    return expression_;
}

//
// IfStatementNode
//

IfStatementNode::IfStatementNode(ExpressionNode *condition,
                                 StatementNode *trueBody,
                                 StatementNode *falseBody)
  : StatementNode(IfStatement),
    condition_(condition),
    trueBody_(trueBody),
    falseBody_(falseBody)
{}

ExpressionNode *
IfStatementNode::condition() const
{
    return condition_;
}

StatementNode *
IfStatementNode::trueBody() const
{
    return trueBody_;
}

StatementNode *
IfStatementNode::falseBody() const
{
    return falseBody_;
}

//
// IterationStatementNode
//

IterationStatementNode::IterationStatementNode(NodeType type)
  : StatementNode(type)
{}

//
// DoWhileStatementNode
//

DoWhileStatementNode::DoWhileStatementNode(
        StatementNode *body, ExpressionNode *condition)
  : IterationStatementNode(DoWhileStatement),
    body_(body),
    condition_(condition)
{}

StatementNode *
DoWhileStatementNode::body() const
{
    return body_;
}

ExpressionNode *
DoWhileStatementNode::condition() const
{
    return condition_;
}

//
// WhileStatementNode
//

WhileStatementNode::WhileStatementNode(
        ExpressionNode *condition, StatementNode *body)
  : IterationStatementNode(WhileStatement),
    condition_(condition),
    body_(body)
{}

ExpressionNode *
WhileStatementNode::condition() const
{
    return condition_;
}

StatementNode *
WhileStatementNode::body() const
{
    return body_;
}

//
// ForLoopStatementNode
//

ForLoopStatementNode::ForLoopStatementNode(
        ExpressionNode *initial, ExpressionNode *condition,
        ExpressionNode *update, StatementNode *body)
  : IterationStatementNode(ForLoopStatement),
    initial_(initial),
    condition_(condition),
    update_(update),
    body_(body)
{}

ExpressionNode *
ForLoopStatementNode::initial() const
{
    return initial_;
}

ExpressionNode *
ForLoopStatementNode::condition() const
{
    return condition_;
}

ExpressionNode *
ForLoopStatementNode::update() const
{
    return update_;
}

StatementNode *
ForLoopStatementNode::body() const
{
    return body_;
}

//
// ForLoopVarStatementNode
//

ForLoopVarStatementNode::ForLoopVarStatementNode(
        DeclarationList &&initial, ExpressionNode *condition,
        ExpressionNode *update, StatementNode *body)
  : IterationStatementNode(ForLoopVarStatement),
    initial_(initial),
    condition_(condition),
    update_(update),
    body_(body)
{}

const DeclarationList &
ForLoopVarStatementNode::initial() const
{
    return initial_;
}

DeclarationList &
ForLoopVarStatementNode::initial()
{
    return initial_;
}

ExpressionNode *
ForLoopVarStatementNode::condition() const
{
    return condition_;
}

ExpressionNode *
ForLoopVarStatementNode::update() const
{
    return update_;
}

StatementNode *
ForLoopVarStatementNode::body() const
{
    return body_;
}

//
// ForInStatementNode
//

ForInStatementNode::ForInStatementNode(
        ExpressionNode *lhs, ExpressionNode *object, StatementNode *body)
  : IterationStatementNode(ForInStatement),
    lhs_(lhs),
    object_(object),
    body_(body)
{}

ExpressionNode *
ForInStatementNode::lhs() const
{
    return lhs_;
}

ExpressionNode *
ForInStatementNode::object() const
{
    return object_;
}

StatementNode *
ForInStatementNode::body() const
{
    return body_;
}

//
// ForInVarStatementNode
//

ForInVarStatementNode::ForInVarStatementNode(
        const IdentifierNameToken &name, ExpressionNode *object,
        StatementNode *body)
  : IterationStatementNode(ForInVarStatement),
    name_(name),
    object_(object),
    body_(body)
{}

const IdentifierNameToken &
ForInVarStatementNode::name() const
{
    return name_;
}

ExpressionNode *
ForInVarStatementNode::object() const
{
    return object_;
}

StatementNode *
ForInVarStatementNode::body() const
{
    return body_;
}

//
// ContinueStatementNode
//

ContinueStatementNode::ContinueStatementNode()
  : StatementNode(ContinueStatement),
    label_()
{}

ContinueStatementNode::ContinueStatementNode(const IdentifierNameToken &label)
  : StatementNode(ContinueStatement),
    label_(label)
{}

const Maybe<IdentifierNameToken> &
ContinueStatementNode::label() const
{
    return label_;
}

//
// BreakStatementNode
//

BreakStatementNode::BreakStatementNode()
  : StatementNode(BreakStatement),
    label_()
{}

BreakStatementNode::BreakStatementNode(const IdentifierNameToken &label)
  : StatementNode(BreakStatement),
    label_(label)
{}

const Maybe<IdentifierNameToken> &
BreakStatementNode::label() const
{
    return label_;
}

//
// ReturnStatementNode
//

ReturnStatementNode::ReturnStatementNode(ExpressionNode *value)
  : StatementNode(ReturnStatement),
    value_(value)
{}

ExpressionNode *
ReturnStatementNode::value() const
{
    return value_;
}

//
// WithStatementNode
//

WithStatementNode::WithStatementNode(
        ExpressionNode *value, StatementNode *body)
  : StatementNode(WithStatement),
    value_(value),
    body_(body)
{}

ExpressionNode *
WithStatementNode::value() const
{
    return value_;
}

StatementNode *
WithStatementNode::body() const
{
    return body_;
}

//
// SwitchStatementNode
//

SwitchStatementNode::CaseClause::CaseClause(
        ExpressionNode *expression, StatementList &&statements)
  : expression_(expression),
    statements_(statements)
{}

SwitchStatementNode::CaseClause::CaseClause(const CaseClause &other)
  : expression_(other.expression_),
    statements_(other.statements_)
{}

SwitchStatementNode::CaseClause::CaseClause(CaseClause &&other)
  : expression_(other.expression_),
    statements_(std::move(other.statements_))
{}

ExpressionNode *
SwitchStatementNode::CaseClause::expression() const
{
    return expression_;
}

const StatementList &
SwitchStatementNode::CaseClause::statements() const
{
    return statements_;
}

SwitchStatementNode::SwitchStatementNode(
        ExpressionNode *value, CaseClauseList &&caseClauses)
  : StatementNode(SwitchStatement),
    value_(value),
    caseClauses_(caseClauses)
{}

ExpressionNode *
SwitchStatementNode::value() const
{
    return value_;
}

const SwitchStatementNode::CaseClauseList &
SwitchStatementNode::caseClauses() const
{
    return caseClauses_;
}

//
// LabelledStatementNode
//

LabelledStatementNode::LabelledStatementNode(
        const IdentifierNameToken &label, StatementNode *statement)
  : StatementNode(LabelledStatement),
    label_(label),
    statement_(statement)
{}

const IdentifierNameToken &
LabelledStatementNode::label() const
{
    return label_;
}

StatementNode *
LabelledStatementNode::statement() const
{
    return statement_;
}

//
// ThrowStatementNode
//

ThrowStatementNode::ThrowStatementNode(ExpressionNode *value)
  : StatementNode(ThrowStatement),
    value_(value)
{}

ExpressionNode *
ThrowStatementNode::value() const
{
    return value_;
}


//
// TryStatementNode;
//

TryStatementNode::TryStatementNode(NodeType type)
  : StatementNode(type)
{}


//
// TryCatchStatementNode
//

TryCatchStatementNode::TryCatchStatementNode(
        BlockNode *tryBlock, const IdentifierNameToken &catchName,
        BlockNode *catchBlock)
  : TryStatementNode(TryCatchStatement),
    tryBlock_(tryBlock),
    catchName_(catchName),
    catchBlock_(catchBlock)
{}

BlockNode *
TryCatchStatementNode::tryBlock() const
{
    return tryBlock_;
}

const IdentifierNameToken &
TryCatchStatementNode::catchName() const
{
    return catchName_;
}

BlockNode *
TryCatchStatementNode::catchBlock() const
{
    return catchBlock_;
}

//
// TryFinallyStatementNode
//

TryFinallyStatementNode::TryFinallyStatementNode(
        BlockNode *tryBlock, BlockNode *finallyBlock)
  : TryStatementNode(TryFinallyStatement),
    tryBlock_(tryBlock),
    finallyBlock_(finallyBlock)
{}

BlockNode *
TryFinallyStatementNode::tryBlock() const
{
    return tryBlock_;
}

BlockNode *
TryFinallyStatementNode::finallyBlock() const
{
    return finallyBlock_;
}

//
// TryCatchFinallyStatementNode
//

TryCatchFinallyStatementNode::TryCatchFinallyStatementNode(
        BlockNode *tryBlock, const IdentifierNameToken &catchName,
        BlockNode *catchBlock, BlockNode *finallyBlock)
  : TryStatementNode(TryCatchFinallyStatement),
    tryBlock_(tryBlock),
    catchName_(catchName),
    catchBlock_(catchBlock),
    finallyBlock_(finallyBlock)
{}

BlockNode *
TryCatchFinallyStatementNode::tryBlock() const
{
    return tryBlock_;
}

const IdentifierNameToken &
TryCatchFinallyStatementNode::catchName() const
{
    return catchName_;
}

BlockNode *
TryCatchFinallyStatementNode::catchBlock() const
{
    return catchBlock_;
}

BlockNode *
TryCatchFinallyStatementNode::finallyBlock() const
{
    return finallyBlock_;
}

//
// DebuggerStatementNode
//

DebuggerStatementNode::DebuggerStatementNode()
  : StatementNode(DebuggerStatement)
{}

//
// FunctionDeclarationNode
//

FunctionDeclarationNode::FunctionDeclarationNode(FunctionExpressionNode *func)
  : SourceElementNode(FunctionDeclaration),
    func_(func)
{
    WH_ASSERT(func->name());
}

FunctionExpressionNode *
FunctionDeclarationNode::func() const
{
    return func_;
}

//
// ProgramNode
//

ProgramNode::ProgramNode(SourceElementList &&sourceElements)
  : BaseNode(Program),
    sourceElements_(sourceElements)
{}

const SourceElementList &
ProgramNode::sourceElements() const
{
    return sourceElements_;
}



} // namespace AST
} // namespace Whisper
