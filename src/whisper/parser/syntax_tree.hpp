#ifndef WHISPER__PARSER__SYNTAX_TREE_HPP
#define WHISPER__PARSER__SYNTAX_TREE_HPP

#include <list>
#include "parser/tokenizer.hpp"
#include "parser/syntax_defn.hpp"

namespace Whisper {
namespace AST {

enum NodeType : uint8_t
{
    INVALID,
#define DEF_ENUM_(node)   node,
    WHISPER_DEFN_SYNTAX_NODES(DEF_ENUM_)
#undef DEF_ENUM_
    LIMIT
};

const char *NodeTypeString(NodeType nodeType);

//
// Pre-declarations.
//

class SourceElementNode;
class StatementNode;
class ExpressionNode;
class LiteralExpressionNode;
class VariableDeclaration;

class ThisNode;
class IdentifierNode;
class NullLiteralNode;
class BooleanLiteralNode;
class NumericLiteralNode;
class StringLiteralNode;
class RegularExpressionLiteralNode;
class ArrayLiteralNode;
class ObjectLiteralNode;
class ParenthesizedExpressionNode;
class FunctionExpressionNode;
class GetElementExpressionNode;
class GetPropertyExpressionNode;
class NewExpressionNode;
class CallExpressionNode;

class BaseUnaryExpressionNode;
template <NodeType TYPE> class UnaryExpressionNode;

typedef UnaryExpressionNode<PostIncrementExpression>
        PostIncrementExpressionNode;
typedef UnaryExpressionNode<PreIncrementExpression>
        PreIncrementExpressionNode;
typedef UnaryExpressionNode<PostDecrementExpression>
        PostDecrementExpressionNode;
typedef UnaryExpressionNode<PreDecrementExpression>
        PreDecrementExpressionNode;

typedef UnaryExpressionNode<DeleteExpression>
        DeleteExpressionNode;
typedef UnaryExpressionNode<VoidExpression>
        VoidExpressionNode;
typedef UnaryExpressionNode<TypeOfExpression>
        TypeOfExpressionNode;
typedef UnaryExpressionNode<PositiveExpression>
        PositiveExpressionNode;
typedef UnaryExpressionNode<NegativeExpression>
        NegativeExpressionNode;
typedef UnaryExpressionNode<BitNotExpression>
        BitNotExpressionNode;
typedef UnaryExpressionNode<LogicalNotExpression>
        LogicalNotExpressionNode;


class BaseBinaryExpressionNode;
template <NodeType TYPE> class BinaryExpressionNode;

typedef BinaryExpressionNode<MultiplyExpression>
        MultiplyExpressionNode;
typedef BinaryExpressionNode<DivideExpression>
        DivideExpressionNode;
typedef BinaryExpressionNode<ModuloExpression>
        ModuloExpressionNode;
typedef BinaryExpressionNode<AddExpression>
        AddExpressionNode;
typedef BinaryExpressionNode<SubtractExpression>
        SubtractExpressionNode;
typedef BinaryExpressionNode<LeftShiftExpression>
        LeftShiftExpressionNode;
typedef BinaryExpressionNode<RightShiftExpression>
        RightShiftExpressionNode;
typedef BinaryExpressionNode<UnsignedRightShiftExpression>
        UnsignedRightShiftExpressionNode;
typedef BinaryExpressionNode<LessThanExpression>
        LessThanExpressionNode;
typedef BinaryExpressionNode<GreaterThanExpression>
        GreaterThanExpressionNode;
typedef BinaryExpressionNode<LessEqualExpression>
        LessEqualExpressionNode;
typedef BinaryExpressionNode<GreaterEqualExpression>
        GreaterEqualExpressionNode;
typedef BinaryExpressionNode<InstanceOfExpression>
        InstanceOfExpressionNode;
typedef BinaryExpressionNode<InExpression>
        InExpressionNode;
typedef BinaryExpressionNode<EqualExpression>
        EqualExpressionNode;
typedef BinaryExpressionNode<NotEqualExpression>
        NotEqualExpressionNode;
typedef BinaryExpressionNode<StrictEqualExpression>
        StrictEqualExpressionNode;
typedef BinaryExpressionNode<StrictNotEqualExpression>
        StrictNotEqualExpressionNode;
typedef BinaryExpressionNode<BitAndExpression>
        BitAndExpressionNode;
typedef BinaryExpressionNode<BitXorExpression>
        BitXorExpressionNode;
typedef BinaryExpressionNode<BitOrExpression>
        BitOrExpressionNode;
typedef BinaryExpressionNode<LogicalAndExpression>
        LogicalAndExpressionNode;
typedef BinaryExpressionNode<LogicalOrExpression>
        LogicalOrExpressionNode;
typedef BinaryExpressionNode<CommaExpression>
        CommaExpressionNode;

class ConditionalExpressionNode;

class BaseAssignmentExpressionNode;
template <NodeType TYPE> class AssignmentExpressionNode;

typedef AssignmentExpressionNode<AssignExpression>
        AssignExpressionNode;
typedef AssignmentExpressionNode<AddAssignExpression>
        AddAssignExpressionNode;
typedef AssignmentExpressionNode<SubtractAssignExpression>
        SubtractAssignExpressionNode;
typedef AssignmentExpressionNode<MultiplyAssignExpression>
        MultiplyAssignExpressionNode;
typedef AssignmentExpressionNode<ModuloAssignExpression>
        ModuloAssignExpressionNode;
typedef AssignmentExpressionNode<LeftShiftAssignExpression>
        LeftShiftAssignExpressionNode;
typedef AssignmentExpressionNode<RightShiftAssignExpression>
        RightShiftAssignExpressionNode;
typedef AssignmentExpressionNode<UnsignedRightShiftAssignExpression>
        UnsignedRightShiftAssignExpressionNode;
typedef AssignmentExpressionNode<BitAndAssignExpression>
        BitAndAssignExpressionNode;
typedef AssignmentExpressionNode<BitOrAssignExpression>
        BitOrAssignExpressionNode;
typedef AssignmentExpressionNode<BitXorAssignExpression>
        BitXorAssignExpressionNode;
typedef AssignmentExpressionNode<DivideAssignExpression>
        DivideAssignExpressionNode;

class BlockNode;
class VariableStatementNode;
class EmptyStatementNode;
class ExpressionStatementNode;
class IfStatementNode;
class IterationStatementNode;
class DoWhileStatementNode;
class WhileStatementNode;
class ForLoopStatementNode;
class ForLoopVarStatementNode;
class ForInStatementNode;
class ForInVarStatementNode;
class ContinueStatementNode;
class BreakStatementNode;
class ReturnStatementNode;
class WithStatementNode;
class SwitchStatementNode;
class LabelledStatementNode;
class ThrowStatementNode;
class TryStatementNode;
class TryCatchStatementNode;
class TryFinallyStatementNode;
class TryCatchFinallyStatementNode;
class DebuggerStatementNode;
class FunctionDeclarationNode;
class ProgramNode;

//
// Base syntax element.
//
class BaseNode
{
  public:
    template <typename T>
    using Allocator = STLBumpAllocator<T>;

    typedef Allocator<uint8_t> StdAllocator;

    template <typename T>
    using List = std::list<T, Allocator<T>>;

  protected:
    NodeType type_;

    BaseNode(NodeType type);

  public:
    NodeType type() const;

#define METHODS_(node) \
    bool is##node() const; \
    const node##Node *to##node() const; \
    node##Node *to##node();
    WHISPER_DEFN_SYNTAX_NODES(METHODS_)
#undef METHODS_

    virtual bool isStatement() const;
    virtual bool isBinaryExpression() const;

    const BaseBinaryExpressionNode *toBinaryExpression() const;
    BaseBinaryExpressionNode *toBinaryExpression();

    bool isLeftHandSideExpression();
};

template <typename Printer>
void PrintNode(const CodeSource &source, const BaseNode *node, Printer printer,
               int tabDepth);

inline constexpr bool
IsValidAssignmentExpressionType(NodeType type)
{
    return (type >= WHISPER_SYNTAX_ASSIGN_MIN) &&
           (type <= WHISPER_SYNTAX_ASSIGN_MAX);
}

///////////////////////////////////////
//                                   //
//  Intermediate and Helper Classes  //
//                                   //
///////////////////////////////////////

class SourceElementNode : public BaseNode
{
  protected:
    SourceElementNode(NodeType type);
};

class StatementNode : public SourceElementNode
{
  protected:
    StatementNode(NodeType type);

  public:
    virtual bool isStatement() const override;

    FunctionExpressionNode *statementToNamedFunction();
};


class ExpressionNode : public BaseNode
{
  protected:
    ExpressionNode(NodeType type);

  public:
    bool isNamedFunction();
};

class LiteralExpressionNode : public ExpressionNode
{
  protected:
    LiteralExpressionNode(NodeType type);
};

class VariableDeclaration
{
  public:
    IdentifierNameToken name_;
    ExpressionNode *initialiser_;

  public:
    VariableDeclaration(const IdentifierNameToken &name,
                        ExpressionNode *initialiser);

    const IdentifierNameToken &name() const;
    ExpressionNode *initialiser() const;
};

typedef BaseNode::List<ExpressionNode *> ExpressionList;
typedef BaseNode::List<StatementNode *> StatementList;
typedef BaseNode::List<SourceElementNode *> SourceElementList;
typedef BaseNode::List<VariableDeclaration> DeclarationList;
typedef BaseNode::List<IdentifierNameToken> FormalParameterList;


///////////////////
//               //
//  Expressions  //
//               //
///////////////////

//
// ThisNode syntax element
//
class ThisNode : public ExpressionNode
{
  private:
    ThisKeywordToken token_;

  public:
    explicit ThisNode(const ThisKeywordToken &token);

    const ThisKeywordToken &token() const;
};

//
// IdentifierNode syntax element
//
class IdentifierNode : public ExpressionNode
{
  private:
    IdentifierNameToken token_;

  public:
    explicit IdentifierNode(const IdentifierNameToken &token);

    const IdentifierNameToken &token() const;
};

//
// NullLiteralNode syntax element
//
class NullLiteralNode : public LiteralExpressionNode
{
  private:
    NullLiteralToken token_;

  public:
    explicit NullLiteralNode(const NullLiteralToken &token);

    const NullLiteralToken &token() const;
};

//
// BooleanLiteralNode syntax element
//
class BooleanLiteralNode : public LiteralExpressionNode
{
  private:
    Either<FalseLiteralToken, TrueLiteralToken> token_;

  public:
    explicit BooleanLiteralNode(const FalseLiteralToken &token);
    explicit BooleanLiteralNode(const TrueLiteralToken &token);

    bool isFalse() const;
    bool isTrue() const;
};

//
// NumericLiteralNode syntax element
//
class NumericLiteralNode : public LiteralExpressionNode
{
  private:
    NumericLiteralToken value_;

  public:
    explicit NumericLiteralNode(const NumericLiteralToken &value);

    const NumericLiteralToken value() const;
};

//
// StringLiteralNode syntax element
//
class StringLiteralNode : public LiteralExpressionNode
{
  private:
    StringLiteralToken value_;

  public:
    explicit StringLiteralNode(const StringLiteralToken &value);

    const StringLiteralToken value() const;
};

//
// RegularExpressionLiteralNode syntax element
//
class RegularExpressionLiteralNode : public LiteralExpressionNode
{
  private:
    RegularExpressionLiteralToken value_;

  public:
    explicit RegularExpressionLiteralNode(
            const RegularExpressionLiteralToken &value);

    const RegularExpressionLiteralToken value() const;
};

//
// ArrayLiteralNode syntax element
//
class ArrayLiteralNode : public LiteralExpressionNode
{
  private:
    ExpressionList elements_;

  public:
    explicit ArrayLiteralNode(ExpressionList &&elements);

    const ExpressionList &elements() const;
};

//
// ObjectLiteralNode syntax element
//
class ObjectLiteralNode : public LiteralExpressionNode
{
  public:
    enum SlotKind { Value, Getter, Setter };

    class ValueDefinition;
    class GetterDefinition;
    class SetterDefinition;

    class PropertyDefinition
    {
      private:
        SlotKind kind_;
        Token name_;

      public:
        PropertyDefinition(SlotKind kind, const Token &name);

        SlotKind kind() const;

        bool isValueSlot() const;
        bool isGetterSlot() const;
        bool isSetterSlot() const;

        const ValueDefinition *toValueSlot() const;
        const GetterDefinition *toGetterSlot() const;
        const SetterDefinition *toSetterSlot() const;

        bool hasIdentifierName() const;
        bool hasStringName() const;
        bool hasNumericName() const;

        const IdentifierNameToken &identifierName() const;
        const StringLiteralToken &stringName() const;
        const NumericLiteralToken &numericName() const;

        const Token &name() const;
    };

    class ValueDefinition : public PropertyDefinition
    {
      private:
        ExpressionNode *value_;

      public:
        ValueDefinition(const Token &name, ExpressionNode *value);

        ExpressionNode *value() const;
    };

    class AccessorDefinition : public PropertyDefinition
    {
      protected:
        SourceElementList body_;

      public:
        AccessorDefinition(SlotKind kind, const Token &name,
                           SourceElementList &&body);

        const SourceElementList &body() const;
    };

    class GetterDefinition : public AccessorDefinition
    {
      public:
        GetterDefinition(const Token &name, SourceElementList &&body);
    };

    class SetterDefinition : public AccessorDefinition
    {
      private:
        IdentifierNameToken parameter_;

      public:
        SetterDefinition(const Token &name,
                         const IdentifierNameToken &parameter,
                         SourceElementList &&body);

        const IdentifierNameToken &parameter() const;
    };

    typedef List<PropertyDefinition *> PropertyDefinitionList;

  private:
    PropertyDefinitionList propertyDefinitions_;

  public:
    explicit ObjectLiteralNode(PropertyDefinitionList &&propertyDefinitions);

    const PropertyDefinitionList &propertyDefinitions() const;
};

//
// ParenthesizedExpressionNode syntax element
//
class ParenthesizedExpressionNode : public ExpressionNode
{
  private:
    ExpressionNode *subexpression_;

  public:
    explicit ParenthesizedExpressionNode(ExpressionNode *subexpression);

    ExpressionNode *subexpression() const;
};

//
// FunctionExpression syntax element
//
class FunctionExpressionNode : public ExpressionNode
{
  private:
    Maybe<IdentifierNameToken> name_;
    FormalParameterList formalParameters_;
    SourceElementList functionBody_;

  public:
    FunctionExpressionNode(FormalParameterList &&formalParameters,
                           SourceElementList &&functionBody);

    FunctionExpressionNode(const IdentifierNameToken &name,
                           FormalParameterList &&formalParameters,
                           SourceElementList &&functionBody);

    const Maybe<IdentifierNameToken> &name() const;

    const FormalParameterList &formalParameters() const;

    const SourceElementList &functionBody() const;
};

//
// GetElementExpression syntax element
//
class GetElementExpressionNode : public ExpressionNode
{
  private:
    ExpressionNode *object_;
    ExpressionNode *element_;

  public:
    GetElementExpressionNode(ExpressionNode *object, ExpressionNode *element);

    ExpressionNode *object() const;
    ExpressionNode *element() const;
};

//
// GetPropertyExpression syntax element
//
class GetPropertyExpressionNode : public ExpressionNode
{
  private:
    ExpressionNode *object_;
    IdentifierNameToken property_;

  public:
    GetPropertyExpressionNode(ExpressionNode *object,
                              const IdentifierNameToken &property);

    ExpressionNode *object() const;
    const IdentifierNameToken &property() const;
};

//
// NewExpression syntax element
//
class NewExpressionNode : public ExpressionNode
{
  private:
    ExpressionNode *constructor_;
    ExpressionList arguments_;

  public:
    NewExpressionNode(ExpressionNode *constructor, ExpressionList &&arguments);

    ExpressionNode *constructor() const;
    const ExpressionList &arguments() const;
};

//
// CallExpression syntax element
//
class CallExpressionNode : public ExpressionNode
{
  private:
    ExpressionNode *function_;
    ExpressionList arguments_;

  public:
    CallExpressionNode(ExpressionNode *function, ExpressionList &&arguments);

    ExpressionNode *function() const;
    const ExpressionList &arguments() const;
};

//
// UnaryExpression syntax element
//
class BaseUnaryExpressionNode : public ExpressionNode
{
  protected:
    ExpressionNode *subexpression_;

    explicit BaseUnaryExpressionNode(NodeType type,
                                     ExpressionNode *subexpression);

  public:
    ExpressionNode *subexpression() const;
};

template <NodeType TYPE>
class UnaryExpressionNode : public BaseUnaryExpressionNode
{
    static_assert(TYPE == PostIncrementExpression ||
                  TYPE == PreIncrementExpression ||
                  TYPE == PostDecrementExpression ||
                  TYPE == PreDecrementExpression ||
                  TYPE == DeleteExpression ||
                  TYPE == VoidExpression ||
                  TYPE == TypeOfExpression ||
                  TYPE == PositiveExpression ||
                  TYPE == NegativeExpression ||
                  TYPE == BitNotExpression ||
                  TYPE == LogicalNotExpression,
                  "Invalid IncDecExpressionNode type.");
  public:
    explicit inline UnaryExpressionNode(ExpressionNode *subexpression);
};

// PostIncrementExpressionNode;
// PreIncrementExpressionNode;
// PostDecrementExpressionNode;
// PreDecrementExpressionNode;

// DeleteExpressionNode;
// VoidExpressionNode;
// TypeOfExpressionNode;
// PositiveExpressionNode;
// NegativeExpressionNode;
// BitNotExpressionNode;
// LogicalNotExpressionNode;

//
// BinaryExpression syntax element
//
class BaseBinaryExpressionNode : public ExpressionNode
{
  private:
    ExpressionNode *lhs_;
    ExpressionNode *rhs_;

  public:
    BaseBinaryExpressionNode(NodeType type,
                             ExpressionNode *lhs,
                             ExpressionNode *rhs);

    ExpressionNode *lhs() const;
    ExpressionNode *rhs() const;

    virtual bool isBinaryExpression() const override;
};

template <NodeType TYPE>
class BinaryExpressionNode : public BaseBinaryExpressionNode
{
    static_assert(TYPE == MultiplyExpression ||
                  TYPE == DivideExpression ||
                  TYPE == ModuloExpression ||
                  TYPE == AddExpression ||
                  TYPE == SubtractExpression ||
                  TYPE == LeftShiftExpression ||
                  TYPE == RightShiftExpression ||
                  TYPE == UnsignedRightShiftExpression ||
                  TYPE == LessThanExpression ||
                  TYPE == GreaterThanExpression ||
                  TYPE == LessEqualExpression ||
                  TYPE == GreaterEqualExpression ||
                  TYPE == InstanceOfExpression ||
                  TYPE == InExpression ||
                  TYPE == EqualExpression ||
                  TYPE == NotEqualExpression ||
                  TYPE == StrictEqualExpression ||
                  TYPE == StrictNotEqualExpression ||
                  TYPE == BitAndExpression ||
                  TYPE == BitXorExpression ||
                  TYPE == BitOrExpression ||
                  TYPE == LogicalAndExpression ||
                  TYPE == LogicalOrExpression ||
                  TYPE == CommaExpression,
                  "Invalid IncDecExpressionNode type.");
  public:
    explicit BinaryExpressionNode(ExpressionNode *lhs, ExpressionNode *rhs);
};

// MultiplyExpressionNode;
// DivideExpressionNode;
// ModuloExpressionNode;
// AddExpressionNode;
// SubtractExpressionNode;
// LeftShiftExpressionNode;
// RightShiftExpressionNode;
// UnsignedRightShiftExpressionNode;
// LessThanExpressionNode;
// GreaterThanExpressionNode;
// LessEqualExpressionNode;
// GreaterEqualExpressionNode;
// InstanceOfExpressionNode;
// InExpressionNode;
// EqualExpressionNode;
// NotEqualExpressionNode;
// StrictEqualExpressionNode;
// StrictNotEqualExpressionNode;
// BitAndExpressionNode;
// BitXorExpressionNode;
// BitOrExpressionNode;
// LogicalAndExpressionNode;
// LogicalOrExpressionNode;
// CommaExpressionNode;

//
// ConditionalExpression syntax element
//
class ConditionalExpressionNode : public ExpressionNode
{
  private:
    ExpressionNode *condition_;
    ExpressionNode *trueExpression_;
    ExpressionNode *falseExpression_;

  public:
    ConditionalExpressionNode(ExpressionNode *condition,
                              ExpressionNode *trueExpression,
                              ExpressionNode *falseExpression);

    ExpressionNode *condition() const;
    ExpressionNode *trueExpression() const;
    ExpressionNode *falseExpression() const;
};

//
// AssignmentExpression syntax element
//
class BaseAssignmentExpressionNode : public ExpressionNode
{
  protected:
    ExpressionNode *lhs_;
    ExpressionNode *rhs_;

    BaseAssignmentExpressionNode(NodeType type,
                                 ExpressionNode *lhs,
                                 ExpressionNode *rhs);

  public:
    ExpressionNode *lhs() const;
    ExpressionNode *rhs() const;
};

template <NodeType TYPE>
class AssignmentExpressionNode : public BaseAssignmentExpressionNode
{
    static_assert(IsValidAssignmentExpressionType(TYPE),
                  "Invalid AssignmentExpressionNode type.");

  public:
    inline AssignmentExpressionNode(ExpressionNode *lhs, ExpressionNode *rhs);
};

// AssignExpressionNode;
// AddAssignExpressionNode;
// SubtractAssignExpressionNode;
// MultiplyAssignExpressionNode;
// ModuloAssignExpressionNode;
// LeftShiftAssignExpressionNode;
// RightShiftAssignExpressionNode;
// UnsignedRightShiftAssignExpressionNode;
// BitAndAssignExpressionNode;
// BitOrAssignExpressionNode;
// BitXorAssignExpressionNode;
// DivideAssignExpressionNode;


//////////////////
//              //
//  Statements  //
//              //
//////////////////

//
// Block syntax element
//
class BlockNode : public StatementNode
{
  private:
    SourceElementList sourceElements_;

  public:
    explicit BlockNode(SourceElementList &&sourceElements);

    const SourceElementList &sourceElements() const;
};

//
// VariableStatement syntax element
//
class VariableStatementNode : public StatementNode
{
  private:
    DeclarationList declarations_;

  public:
    explicit VariableStatementNode(DeclarationList &&declarations);

    const DeclarationList &declarations() const;
};

//
// EmptyStatement syntax element
//
class EmptyStatementNode : public StatementNode
{
  public:
    EmptyStatementNode();
};

//
// ExpressionStatement syntax element
//
class ExpressionStatementNode : public StatementNode
{
  private:
    ExpressionNode *expression_;

  public:
    explicit ExpressionStatementNode(ExpressionNode *expression);

    ExpressionNode *expression() const;
};

//
// IfStatement syntax element
//
class IfStatementNode : public StatementNode
{
  private:
    ExpressionNode *condition_;
    StatementNode *trueBody_;
    StatementNode *falseBody_;

  public:
    IfStatementNode(ExpressionNode *condition,
                    StatementNode *trueBody,
                    StatementNode *falseBody);

    ExpressionNode *condition() const;
    StatementNode *trueBody() const;
    StatementNode *falseBody() const;
};

//
// Base class for all iteration statements.
//
class IterationStatementNode : public StatementNode
{
  protected:
    IterationStatementNode(NodeType type);
};

//
// DoWhileStatement syntax element
//
class DoWhileStatementNode : public IterationStatementNode
{
  private:
    StatementNode *body_;
    ExpressionNode *condition_;

  public:
    DoWhileStatementNode(StatementNode *body, ExpressionNode *condition);

    StatementNode *body() const;
    ExpressionNode *condition() const;
};

//
// WhileStatement syntax element
//
class WhileStatementNode : public IterationStatementNode
{
  private:
    ExpressionNode *condition_;
    StatementNode *body_;

  public:
    WhileStatementNode(ExpressionNode *condition, StatementNode *body);

    ExpressionNode *condition() const;
    StatementNode *body() const;
};

//
// ForLoopStatement syntax element
//
class ForLoopStatementNode : public IterationStatementNode
{
  private:
    ExpressionNode *initial_;
    ExpressionNode *condition_;
    ExpressionNode *update_;
    StatementNode *body_;

  public:
    ForLoopStatementNode(ExpressionNode *initial,
                         ExpressionNode *condition,
                         ExpressionNode *update,
                         StatementNode *body);

    ExpressionNode *initial() const;
    ExpressionNode *condition() const;
    ExpressionNode *update() const;
    StatementNode *body() const;
};

//
// ForLoopVarStatement syntax element
//
class ForLoopVarStatementNode : public IterationStatementNode
{
  private:
    DeclarationList initial_;
    ExpressionNode *condition_;
    ExpressionNode *update_;
    StatementNode *body_;

  public:
    ForLoopVarStatementNode(DeclarationList &&initial,
                            ExpressionNode *condition,
                            ExpressionNode *update,
                            StatementNode *body);

    const DeclarationList &initial() const;
    DeclarationList &initial();
    ExpressionNode *condition() const;
    ExpressionNode *update() const;
    StatementNode *body() const;
};

//
// ForInStatement syntax element
//
class ForInStatementNode : public IterationStatementNode
{
  private:
    ExpressionNode *lhs_;
    ExpressionNode *object_;
    StatementNode *body_;

  public:
    ForInStatementNode(ExpressionNode *lhs,
                       ExpressionNode *object,
                       StatementNode *body);

    ExpressionNode *lhs() const;
    ExpressionNode *object() const;
    StatementNode *body() const;
};

//
// ForInVarStatement syntax element
//
class ForInVarStatementNode : public IterationStatementNode
{
  private:
    IdentifierNameToken name_;
    ExpressionNode *object_;
    StatementNode *body_;

  public:
    ForInVarStatementNode(const IdentifierNameToken &name,
                          ExpressionNode *object,
                          StatementNode *body);

    const IdentifierNameToken &name() const;
    ExpressionNode *object() const;
    StatementNode *body() const;
};

//
// ContinueStatement syntax element
//
class ContinueStatementNode : public StatementNode
{
  private:
    Maybe<IdentifierNameToken> label_;

  public:
    ContinueStatementNode();

    explicit ContinueStatementNode(const IdentifierNameToken &label);

    const Maybe<IdentifierNameToken> &label() const;
};

//
// BreakStatement syntax element
//
class BreakStatementNode : public StatementNode
{
  private:
    Maybe<IdentifierNameToken> label_;

  public:
    BreakStatementNode();

    explicit BreakStatementNode(const IdentifierNameToken &label);

    const Maybe<IdentifierNameToken> &label() const;
};

//
// ReturnStatement syntax element
//
class ReturnStatementNode : public StatementNode
{
  private:
    ExpressionNode *value_;

  public:
    explicit ReturnStatementNode(ExpressionNode *value);

    ExpressionNode *value() const;
};

//
// WithStatement syntax element
//
class WithStatementNode : public StatementNode
{
  private:
    ExpressionNode *value_;
    StatementNode *body_;

  public:
    WithStatementNode(ExpressionNode *value, StatementNode *body);

    ExpressionNode *value() const;
    StatementNode *body() const;
};

//
// SwitchStatement syntax element
//
class SwitchStatementNode : public StatementNode
{
  public:
    class CaseClause
    {
      private:
        ExpressionNode *expression_;
        StatementList statements_;

      public:
        CaseClause(ExpressionNode *expression, StatementList &&statements);
        CaseClause(const CaseClause &other);
        CaseClause(CaseClause &&other);

        ExpressionNode *expression() const;
        const StatementList &statements() const;
    };
    typedef List<CaseClause> CaseClauseList;

  private:
    ExpressionNode *value_;
    CaseClauseList caseClauses_;

  public:
    SwitchStatementNode(ExpressionNode *value, CaseClauseList &&caseClauses);

    ExpressionNode *value() const;
    const CaseClauseList &caseClauses() const;
};

//
// LabelledStatement syntax element
//
class LabelledStatementNode : public StatementNode
{
  private:
    IdentifierNameToken label_;
    StatementNode *statement_;

  public:
    LabelledStatementNode(const IdentifierNameToken &label,
                          StatementNode *statement);

    const IdentifierNameToken &label() const;
    StatementNode *statement() const;
};

//
// ThrowStatement syntax element
//
class ThrowStatementNode : public StatementNode
{
  private:
    ExpressionNode *value_;

  public:
    explicit ThrowStatementNode(ExpressionNode *value);

    ExpressionNode *value() const;
};


//
// Base helper class for all try/catch?/finally? statements.
//
class TryStatementNode : public StatementNode
{
  protected:
    TryStatementNode(NodeType type);
};


//
// TryCatchStatement syntax element
//
class TryCatchStatementNode : public TryStatementNode
{
  private:
    BlockNode *tryBlock_;
    IdentifierNameToken catchName_;
    BlockNode *catchBlock_;

  public:
    TryCatchStatementNode(BlockNode *tryBlock,
                          const IdentifierNameToken &catchName,
                          BlockNode *catchBlock);

    BlockNode *tryBlock() const;
    const IdentifierNameToken &catchName() const;
    BlockNode *catchBlock() const;
};

//
// TryFinallyStatement syntax element
//
class TryFinallyStatementNode : public TryStatementNode
{
  private:
    BlockNode *tryBlock_;
    BlockNode *finallyBlock_;

  public:
    TryFinallyStatementNode(BlockNode *tryBlock, BlockNode *finallyBlock);

    BlockNode *tryBlock() const;
    BlockNode *finallyBlock() const;
};

//
// TryCatchFinallyStatement syntax element
//
class TryCatchFinallyStatementNode : public TryStatementNode
{
  private:
    BlockNode *tryBlock_;
    IdentifierNameToken catchName_;
    BlockNode *catchBlock_;
    BlockNode *finallyBlock_;

  public:
    TryCatchFinallyStatementNode(BlockNode *tryBlock,
                                 const IdentifierNameToken &catchName,
                                 BlockNode *catchBlock,
                                 BlockNode *finallyBlock);

    BlockNode *tryBlock() const;
    const IdentifierNameToken &catchName() const;
    BlockNode *catchBlock() const;
    BlockNode *finallyBlock() const;
};

//
// DebuggerStatement syntax element
//
class DebuggerStatementNode : public StatementNode
{
  public:
    explicit DebuggerStatementNode();
};

/////////////////////////////
//                         //
//  Functions And Scripts  //
//                         //
/////////////////////////////

//
// FunctionDeclaration syntax element
//
class FunctionDeclarationNode : public SourceElementNode
{
  private:
    FunctionExpressionNode *func_;

  public:
    FunctionDeclarationNode(FunctionExpressionNode *func);

    FunctionExpressionNode *func() const;
};

//
// Program syntax element
//
class ProgramNode : public BaseNode
{
  private:
    SourceElementList sourceElements_;

  public:
    explicit ProgramNode(SourceElementList &&sourceElements);

    const SourceElementList &sourceElements() const;
};


} // namespace AST
} // namespace Whisper


#endif // WHISPER__PARSER__SYNTAX_TREE_HPP
