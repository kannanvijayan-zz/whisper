#ifndef WHISPER__PARSER__SYNTAX_TREE_HPP
#define WHISPER__PARSER__SYNTAX_TREE_HPP

#include <list>
#include "parser/tokenizer.hpp"
#include "parser/syntax_defn.hpp"

namespace Whisper {
namespace AST {

//
// Annotation forward declarations.
//
class NumericLiteralAnnotation;

//
// Mixin class for syntax tree nodes which are annotated.
//
template <typename Annot>
class Annotated
{
  protected:
    Annot *annot_ = nullptr;

  public:
    inline Annotated() {}

    inline bool hasAnnotation() const {
        return annot_ != nullptr;
    }

    inline Annot *annotation() const {
        WH_ASSERT(hasAnnotation());
        return annot_;
    }

    inline void setAnnotation(Annot *annot) {
        WH_ASSERT(!annot_);
        annot_ = annot;
    }
};


//
// The syntax nodes are used to build a syntax tree of parsed script
// or function body.
//
// They also hold annotation fields which are filled by a pre-codegen
// pass, containing information relevant for code-generation.
//

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

    inline BaseNode(NodeType type) : type_(type) {}

  public:
    inline NodeType type() const {
        return type_;
    }

    inline const char *typeString() const {
        return NodeTypeString(type_);
    }

#define METHODS_(node) \
    inline bool is##node() const { \
        return type_ == node; \
    } \
    inline const node##Node *to##node() const { \
        WH_ASSERT(is##node()); \
        return reinterpret_cast<const node##Node *>(this); \
    } \
    inline node##Node *to##node() { \
        WH_ASSERT(is##node()); \
        return reinterpret_cast<node##Node *>(this); \
    }
    WHISPER_DEFN_SYNTAX_NODES(METHODS_)
#undef METHODS_

    virtual inline bool isStatement() const {
        return false;
    }
    virtual inline bool isBinaryExpression() const {
        return false;
    }
    inline const BaseBinaryExpressionNode *toBinaryExpression() const {
        WH_ASSERT(isBinaryExpression());
        return reinterpret_cast<const BaseBinaryExpressionNode *>(this);
    }
    inline BaseBinaryExpressionNode *toBinaryExpression() {
        WH_ASSERT(isBinaryExpression());
        return reinterpret_cast<BaseBinaryExpressionNode *>(this);
    }

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
    inline SourceElementNode(NodeType type) : BaseNode(type) {}
};

class StatementNode : public SourceElementNode
{
  protected:
    inline StatementNode(NodeType type) : SourceElementNode(type) {}

  public:
    virtual inline bool isStatement() const override {
        return true;
    }

    FunctionExpressionNode *statementToNamedFunction();
};


class ExpressionNode : public BaseNode
{
  protected:
    inline ExpressionNode(NodeType type) : BaseNode(type) {}

  public:
    bool isNamedFunction();
};

class LiteralExpressionNode : public ExpressionNode
{
  protected:
    LiteralExpressionNode(NodeType type) : ExpressionNode(type) {}
};

class VariableDeclaration
{
  public:
    IdentifierNameToken name_;
    ExpressionNode *initialiser_;

  public:
    inline VariableDeclaration(const IdentifierNameToken &name,
                               ExpressionNode *initialiser)
      : name_(name),
        initialiser_(initialiser)
    {}

    inline const IdentifierNameToken &name() const {
        return name_;
    }
    inline ExpressionNode *initialiser() const {
        return initialiser_;
    }
};

typedef BaseNode::List<ExpressionNode *> ExpressionList;
typedef BaseNode::List<StatementNode *> StatementList;
typedef BaseNode::List<SourceElementNode *> SourceElementList;
typedef BaseNode::List<VariableDeclaration> DeclarationList;

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
    explicit inline ThisNode(const ThisKeywordToken &token)
      : ExpressionNode(This),
        token_(token)
    {}

    inline const ThisKeywordToken &token() const {
        return token_;
    }
};

//
// IdentifierNode syntax element
//
class IdentifierNode : public ExpressionNode
{
  private:
    IdentifierNameToken token_;

  public:
    explicit inline IdentifierNode(const IdentifierNameToken &token)
      : ExpressionNode(Identifier),
        token_(token)
    {}

    inline const IdentifierNameToken &token() const {
        return token_;
    }
};

//
// NullLiteralNode syntax element
//
class NullLiteralNode : public LiteralExpressionNode
{
  private:
    NullLiteralToken token_;

  public:
    explicit inline NullLiteralNode(const NullLiteralToken &token)
      : LiteralExpressionNode(NullLiteral),
        token_(token)
    {}

    inline const NullLiteralToken &token() const {
        return token_;
    }
};

//
// BooleanLiteralNode syntax element
//
class BooleanLiteralNode : public LiteralExpressionNode
{
  private:
    Either<FalseLiteralToken, TrueLiteralToken> token_;

  public:
    explicit inline BooleanLiteralNode(const FalseLiteralToken &token)
      : LiteralExpressionNode(BooleanLiteral),
        token_(token)
    {}

    explicit inline BooleanLiteralNode(const TrueLiteralToken &token)
      : LiteralExpressionNode(BooleanLiteral),
        token_(token)
    {}

    inline bool isFalse() const {
        return token_.hasFirst();
    }

    inline bool isTrue() const {
        return token_.hasSecond();
    }
};

//
// NumericLiteralNode syntax element
//
class NumericLiteralNode : public LiteralExpressionNode,
                           public Annotated<NumericLiteralAnnotation>
{
  private:
    NumericLiteralToken value_;

  public:
    explicit inline NumericLiteralNode(const NumericLiteralToken &value)
      : LiteralExpressionNode(NumericLiteral),
        value_(value)
    {}

    inline const NumericLiteralToken &value() const {
        return value_;
    }
};

//
// StringLiteralNode syntax element
//
class StringLiteralNode : public LiteralExpressionNode
{
  private:
    StringLiteralToken value_;

  public:
    explicit inline StringLiteralNode(const StringLiteralToken &value)
      : LiteralExpressionNode(StringLiteral),
        value_(value)
    {}

    inline const StringLiteralToken &value() const {
        return value_;
    }
};

//
// RegularExpressionLiteralNode syntax element
//
class RegularExpressionLiteralNode : public LiteralExpressionNode
{
  private:
    RegularExpressionLiteralToken value_;

  public:
    explicit inline RegularExpressionLiteralNode(
            const RegularExpressionLiteralToken &value)
      : LiteralExpressionNode(RegularExpressionLiteral),
        value_(value)
    {}

    inline const RegularExpressionLiteralToken &value() const {
        return value_;
    }
};

//
// ArrayLiteralNode syntax element
//
class ArrayLiteralNode : public LiteralExpressionNode
{
  private:
    ExpressionList elements_;

  public:
    explicit inline ArrayLiteralNode(ExpressionList &&elements)
      : LiteralExpressionNode(ArrayLiteral),
        elements_(elements)
    {}

    inline const ExpressionList &elements() const {
        return elements_;
    }
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
        inline PropertyDefinition(SlotKind kind, const Token &name)
          : kind_(kind), name_(name)
        {
            WH_ASSERT(name_.isIdentifierName() ||
                      name_.isStringLiteral() ||
                      name_.isNumericLiteral());
        }

        inline SlotKind kind() const {
            return kind_;
        }

        inline bool isValueSlot() const {
            return kind_ == Value;
        }

        inline const ValueDefinition *toValueSlot() const {
            WH_ASSERT(isValueSlot());
            return reinterpret_cast<const ValueDefinition *>(this);
        }

        inline bool isGetterSlot() const {
            return kind_ == Getter;
        }

        inline const GetterDefinition *toGetterSlot() const {
            WH_ASSERT(isGetterSlot());
            return reinterpret_cast<const GetterDefinition *>(this);
        }

        inline bool isSetterSlot() const {
            return kind_ == Setter;
        }

        inline const SetterDefinition *toSetterSlot() const {
            WH_ASSERT(isSetterSlot());
            return reinterpret_cast<const SetterDefinition *>(this);
        }

        inline bool hasIdentifierName() const {
            return name_.isIdentifierName();
        }

        inline bool hasStringName() const {
            return name_.isStringLiteral();
        }

        inline bool hasNumericName() const {
            return name_.isNumericLiteral();
        }

        inline const IdentifierNameToken &identifierName() const {
            WH_ASSERT(hasIdentifierName());
            return reinterpret_cast<const IdentifierNameToken &>(name_);
        }

        inline const StringLiteralToken &stringName() const {
            WH_ASSERT(hasStringName());
            return reinterpret_cast<const StringLiteralToken &>(name_);
        }

        inline const NumericLiteralToken &numericName() const {
            WH_ASSERT(hasNumericName());
            return reinterpret_cast<const NumericLiteralToken &>(name_);
        }

        inline const Token &name() const {
            return name_;
        }
    };

    class ValueDefinition : public PropertyDefinition
    {
      private:
        ExpressionNode *value_;

      public:
        inline ValueDefinition(const Token &name, ExpressionNode *value)
          : PropertyDefinition(Value, name), value_(value)
        {
        }

        inline ExpressionNode *value() const {
            return value_;
        }
    };

    class AccessorDefinition : public PropertyDefinition
    {
      protected:
        SourceElementList body_;

      public:
        inline AccessorDefinition(SlotKind kind, const Token &name,
                                  SourceElementList &&body)
          : PropertyDefinition(kind, name), body_(body)
        {}

        inline const SourceElementList &body() const {
            return body_;
        }
    };

    class GetterDefinition : public AccessorDefinition
    {
      public:
        inline GetterDefinition(const Token &name, SourceElementList &&body)
          : AccessorDefinition(Getter, name, std::move(body))
        {}
    };

    class SetterDefinition : public AccessorDefinition
    {
      private:
        IdentifierNameToken parameter_;

      public:
        inline SetterDefinition(const Token &name,
                                const IdentifierNameToken &parameter,
                                SourceElementList &&body)
          : AccessorDefinition(Setter, name, std::move(body)),
            parameter_(parameter)
        {}

        const IdentifierNameToken &parameter() const {
            return parameter_;
        }
    };

    typedef List<PropertyDefinition *> PropertyDefinitionList;

  private:
    PropertyDefinitionList propertyDefinitions_;

  public:
    explicit inline ObjectLiteralNode(
            PropertyDefinitionList &&propertyDefinitions)
      : LiteralExpressionNode(ObjectLiteral),
        propertyDefinitions_(propertyDefinitions)
    {}

    inline const PropertyDefinitionList &propertyDefinitions() const {
        return propertyDefinitions_;
    }
};

//
// ParenthesizedExpressionNode syntax element
//
class ParenthesizedExpressionNode : public ExpressionNode
{
  private:
    ExpressionNode *subexpression_;

  public:
    explicit inline ParenthesizedExpressionNode(ExpressionNode *subexpression)
      : ExpressionNode(ParenthesizedExpression),
        subexpression_(subexpression)
    {}

    inline ExpressionNode *subexpression() const {
        return subexpression_;
    }
};

//
// FunctionExpression syntax element
//
class FunctionExpressionNode : public ExpressionNode
{
  public:
    typedef BaseNode::List<IdentifierNameToken> FormalParameterList;

  private:
    Maybe<IdentifierNameToken> name_;
    FormalParameterList formalParameters_;
    SourceElementList functionBody_;

  public:
    inline FunctionExpressionNode(FormalParameterList &&formalParameters,
                                  SourceElementList &&functionBody)
      : ExpressionNode(FunctionExpression),
        name_(),
        formalParameters_(formalParameters),
        functionBody_(functionBody)
    {}

    inline FunctionExpressionNode(const IdentifierNameToken &name,
                                  FormalParameterList &&formalParameters,
                                  SourceElementList &&functionBody)
      : ExpressionNode(FunctionExpression),
        name_(name),
        formalParameters_(formalParameters),
        functionBody_(functionBody)
    {}

    inline const Maybe<IdentifierNameToken> &name() const {
        return name_;
    }

    inline const FormalParameterList &formalParameters() const {
        return formalParameters_;
    }

    inline const SourceElementList &functionBody() const {
        return functionBody_;
    }
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
    inline GetElementExpressionNode(ExpressionNode *object,
                                    ExpressionNode *element)
      : ExpressionNode(GetElementExpression),
        object_(object),
        element_(element)
    {}

    inline ExpressionNode *object() const {
        return object_;
    }

    inline ExpressionNode *element() const {
        return element_;
    }
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
    inline GetPropertyExpressionNode(ExpressionNode *object,
                                     const IdentifierNameToken &property)
      : ExpressionNode(GetPropertyExpression),
        object_(object),
        property_(property)
    {}

    inline ExpressionNode *object() const {
        return object_;
    }

    inline const IdentifierNameToken &property() const {
        return property_;
    }
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
    inline NewExpressionNode(ExpressionNode *constructor,
                             ExpressionList &&arguments)
      : ExpressionNode(NewExpression),
        constructor_(constructor),
        arguments_(arguments)
    {}

    inline ExpressionNode *constructor() const {
        return constructor_;
    }

    inline const ExpressionList &arguments() const {
        return arguments_;
    }
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
    inline CallExpressionNode(ExpressionNode *function,
                              ExpressionList &&arguments)
      : ExpressionNode(CallExpression),
        function_(function),
        arguments_(arguments)
    {}

    inline ExpressionNode *function() const {
        return function_;
    }

    inline const ExpressionList &arguments() const {
        return arguments_;
    }
};

//
// UnaryExpression syntax element
//
class BaseUnaryExpressionNode : public ExpressionNode
{
  protected:
    ExpressionNode *subexpression_;

  public:
    explicit inline BaseUnaryExpressionNode(
            NodeType type, ExpressionNode *subexpression)
      : ExpressionNode(type),
        subexpression_(subexpression)
    {}

    inline ExpressionNode *subexpression() const {
        return subexpression_;
    }
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
    explicit inline UnaryExpressionNode(ExpressionNode *subexpression)
      : BaseUnaryExpressionNode(TYPE, subexpression)
    {}
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
    inline BaseBinaryExpressionNode(NodeType type,
                                    ExpressionNode *lhs,
                                    ExpressionNode *rhs)
      : ExpressionNode(type),
        lhs_(lhs),
        rhs_(rhs)
    {}

    inline ExpressionNode *lhs() const {
        return lhs_;
    }

    inline ExpressionNode *rhs() const {
        return rhs_;
    }

    virtual inline bool isBinaryExpression() const override {
        return true;
    }
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
    explicit inline BinaryExpressionNode(ExpressionNode *lhs,
                                         ExpressionNode *rhs)
      : BaseBinaryExpressionNode(TYPE, lhs, rhs)
    {}
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
    inline ConditionalExpressionNode(ExpressionNode *condition,
                                     ExpressionNode *trueExpression,
                                     ExpressionNode *falseExpression)
      : ExpressionNode(ConditionalExpression),
        condition_(condition),
        trueExpression_(trueExpression),
        falseExpression_(falseExpression)
    {}

    inline ExpressionNode *condition() const {
        return condition_;
    }

    inline ExpressionNode *trueExpression() const {
        return trueExpression_;
    }

    inline ExpressionNode *falseExpression() const {
        return falseExpression_;
    }
};

//
// AssignmentExpression syntax element
//
class BaseAssignmentExpressionNode : public ExpressionNode
{
  protected:
    ExpressionNode *lhs_;
    ExpressionNode *rhs_;

    inline BaseAssignmentExpressionNode(NodeType type,
                                        ExpressionNode *lhs,
                                        ExpressionNode *rhs)
      : ExpressionNode(type),
        lhs_(lhs),
        rhs_(rhs)
    {}

  public:
    inline ExpressionNode *lhs() const {
        return lhs_;
    }

    inline ExpressionNode *rhs() const {
        return rhs_;
    }
};

template <NodeType TYPE>
class AssignmentExpressionNode : public BaseAssignmentExpressionNode
{
    static_assert(IsValidAssignmentExpressionType(TYPE),
                  "Invalid AssignmentExpressionNode type.");

  public:
    inline AssignmentExpressionNode(ExpressionNode *lhs, ExpressionNode *rhs)
      : BaseAssignmentExpressionNode(TYPE, lhs, rhs)
    {}
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
    explicit inline BlockNode(SourceElementList &&sourceElements)
      : StatementNode(Block),
        sourceElements_(sourceElements)
    {}

    inline const SourceElementList &sourceElements() const {
        return sourceElements_;
    }
};

//
// VariableStatement syntax element
//
class VariableStatementNode : public StatementNode
{
  private:
    DeclarationList declarations_;

  public:
    explicit inline VariableStatementNode(DeclarationList &&declarations)
      : StatementNode(VariableStatement),
        declarations_(declarations)
    {}

    inline const DeclarationList &declarations() const {
        return declarations_;
    }
};

//
// EmptyStatement syntax element
//
class EmptyStatementNode : public StatementNode
{
  public:
    inline EmptyStatementNode() : StatementNode(EmptyStatement) {}
};

//
// ExpressionStatement syntax element
//
class ExpressionStatementNode : public StatementNode
{
  private:
    ExpressionNode *expression_;

  public:
    explicit inline ExpressionStatementNode(ExpressionNode *expression)
      : StatementNode(ExpressionStatement),
        expression_(expression)
    {}

    inline ExpressionNode *expression() const {
        return expression_;
    }
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
    inline IfStatementNode(ExpressionNode *condition,
                           StatementNode *trueBody,
                           StatementNode *falseBody)
      : StatementNode(IfStatement),
        condition_(condition),
        trueBody_(trueBody),
        falseBody_(falseBody)
    {}

    inline ExpressionNode *condition() const {
        return condition_;
    }

    inline StatementNode *trueBody() const {
        return trueBody_;
    }

    inline StatementNode *falseBody() const {
        return falseBody_;
    }
};

//
// Base class for all iteration statements.
//
class IterationStatementNode : public StatementNode
{
  protected:
    inline IterationStatementNode(NodeType type) : StatementNode(type) {}
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
    inline DoWhileStatementNode(StatementNode *body,
                                ExpressionNode *condition)
      : IterationStatementNode(DoWhileStatement),
        body_(body),
        condition_(condition)
    {}

    inline StatementNode *body() const {
        return body_;
    }

    inline ExpressionNode *condition() const {
        return condition_;
    }
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
    inline WhileStatementNode(ExpressionNode *condition,
                              StatementNode *body)
      : IterationStatementNode(WhileStatement),
        condition_(condition),
        body_(body)
    {}

    inline ExpressionNode *condition() const {
        return condition_;
    }

    inline StatementNode *body() const {
        return body_;
    }
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
    inline ForLoopStatementNode(ExpressionNode *initial,
                                ExpressionNode *condition,
                                ExpressionNode *update,
                                StatementNode *body)
      : IterationStatementNode(ForLoopStatement),
        initial_(initial),
        condition_(condition),
        update_(update),
        body_(body)
    {}

    inline ExpressionNode *initial() const {
        return initial_;
    }

    inline ExpressionNode *condition() const {
        return condition_;
    }

    inline ExpressionNode *update() const {
        return update_;
    }

    inline StatementNode *body() const {
        return body_;
    }
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
    inline ForLoopVarStatementNode(DeclarationList &&initial,
                                   ExpressionNode *condition,
                                   ExpressionNode *update,
                                   StatementNode *body)
      : IterationStatementNode(ForLoopVarStatement),
        initial_(initial),
        condition_(condition),
        update_(update),
        body_(body)
    {}

    inline const DeclarationList &initial() const {
        return initial_;
    }
    inline DeclarationList &initial() {
        return initial_;
    }

    inline ExpressionNode *condition() const {
        return condition_;
    }

    inline ExpressionNode *update() const {
        return update_;
    }

    inline StatementNode *body() const {
        return body_;
    }
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
    inline ForInStatementNode(ExpressionNode *lhs,
                              ExpressionNode *object,
                              StatementNode *body)
      : IterationStatementNode(ForInStatement),
        lhs_(lhs),
        object_(object),
        body_(body)
    {}

    inline ExpressionNode *lhs() const {
        return lhs_;
    }

    inline ExpressionNode *object() const {
        return object_;
    }

    inline StatementNode *body() const {
        return body_;
    }
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
    inline ForInVarStatementNode(const IdentifierNameToken &name,
                                 ExpressionNode *object,
                                 StatementNode *body)
      : IterationStatementNode(ForInVarStatement),
        name_(name),
        object_(object),
        body_(body)
    {}

    inline const IdentifierNameToken &name() const {
        return name_;
    }

    inline ExpressionNode *object() const {
        return object_;
    }

    inline StatementNode *body() const {
        return body_;
    }
};

//
// ContinueStatement syntax element
//
class ContinueStatementNode : public StatementNode
{
  private:
    Maybe<IdentifierNameToken> label_;

  public:
    inline ContinueStatementNode()
      : StatementNode(ContinueStatement),
        label_()
    {}

    explicit inline ContinueStatementNode(const IdentifierNameToken &label)
      : StatementNode(ContinueStatement),
        label_(label)
    {}

    inline const Maybe<IdentifierNameToken> &label() const {
        return label_;
    }
};

//
// BreakStatement syntax element
//
class BreakStatementNode : public StatementNode
{
  private:
    Maybe<IdentifierNameToken> label_;

  public:
    inline BreakStatementNode()
      : StatementNode(BreakStatement),
        label_()
    {}

    explicit inline BreakStatementNode(const IdentifierNameToken &label)
      : StatementNode(BreakStatement),
        label_(label)
    {}

    inline const Maybe<IdentifierNameToken> &label() const {
        return label_;
    }
};

//
// ReturnStatement syntax element
//
class ReturnStatementNode : public StatementNode
{
  private:
    ExpressionNode *value_;

  public:
    explicit inline ReturnStatementNode(ExpressionNode *value)
      : StatementNode(ReturnStatement),
        value_(value)
    {}

    inline ExpressionNode *value() const {
        return value_;
    }
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
    inline WithStatementNode(ExpressionNode *value, StatementNode *body)
      : StatementNode(WithStatement),
        value_(value),
        body_(body)
    {}

    inline ExpressionNode *value() const {
        return value_;
    }

    inline StatementNode *body() const {
        return body_;
    }
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
        inline CaseClause(ExpressionNode *expression,
                          StatementList &&statements)
          : expression_(expression),
            statements_(statements)
        {}

        inline CaseClause(const CaseClause &other)
          : expression_(other.expression_),
            statements_(other.statements_)
        {}

        inline CaseClause(CaseClause &&other)
          : expression_(other.expression_),
            statements_(std::move(other.statements_))
        {}

        inline ExpressionNode *expression() const {
            return expression_;
        }

        inline const StatementList &statements() const {
            return statements_;
        }
    };
    typedef List<CaseClause> CaseClauseList;

  private:
    ExpressionNode *value_;
    CaseClauseList caseClauses_;

  public:
    inline SwitchStatementNode(ExpressionNode *value,
                               CaseClauseList &&caseClauses)
      : StatementNode(SwitchStatement),
        value_(value),
        caseClauses_(caseClauses)
    {}

    inline ExpressionNode *value() const {
        return value_;
    }

    inline const CaseClauseList &caseClauses() const {
        return caseClauses_;
    }
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
    inline LabelledStatementNode(const IdentifierNameToken &label,
                                 StatementNode *statement)
      : StatementNode(LabelledStatement),
        label_(label),
        statement_(statement)
    {}

    inline const IdentifierNameToken &label() const {
        return label_;
    }

    inline StatementNode *statement() const {
        return statement_;
    }
};

//
// ThrowStatement syntax element
//
class ThrowStatementNode : public StatementNode
{
  private:
    ExpressionNode *value_;

  public:
    explicit inline ThrowStatementNode(ExpressionNode *value)
      : StatementNode(ThrowStatement),
        value_(value)
    {}

    inline ExpressionNode *value() const {
        return value_;
    }
};


//
// Base helper class for all try/catch?/finally? statements.
//
class TryStatementNode : public StatementNode
{
  protected:
    inline TryStatementNode(NodeType type) : StatementNode(type) {}
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
    inline TryCatchStatementNode(BlockNode *tryBlock,
                                 const IdentifierNameToken &catchName,
                                 BlockNode *catchBlock)
      : TryStatementNode(TryCatchStatement),
        tryBlock_(tryBlock),
        catchName_(catchName),
        catchBlock_(catchBlock)
    {}

    inline BlockNode *tryBlock() const {
        return tryBlock_;
    }

    inline const IdentifierNameToken &catchName() const {
        return catchName_;
    }

    inline BlockNode *catchBlock() const {
        return catchBlock_;
    }
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
    inline TryFinallyStatementNode(BlockNode *tryBlock,
                                   BlockNode *finallyBlock)
      : TryStatementNode(TryFinallyStatement),
        tryBlock_(tryBlock),
        finallyBlock_(finallyBlock)
    {}

    inline BlockNode *tryBlock() const {
        return tryBlock_;
    }

    inline BlockNode *finallyBlock() const {
        return finallyBlock_;
    }
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
    inline TryCatchFinallyStatementNode(BlockNode *tryBlock,
                                        const IdentifierNameToken &catchName,
                                        BlockNode *catchBlock,
                                        BlockNode *finallyBlock)
      : TryStatementNode(TryCatchFinallyStatement),
        tryBlock_(tryBlock),
        catchName_(catchName),
        catchBlock_(catchBlock),
        finallyBlock_(finallyBlock)
    {}

    inline BlockNode *tryBlock() const {
        return tryBlock_;
    }

    inline const IdentifierNameToken &catchName() const {
        return catchName_;
    }

    inline BlockNode *catchBlock() const {
        return catchBlock_;
    }

    inline BlockNode *finallyBlock() const {
        return finallyBlock_;
    }
};

//
// DebuggerStatement syntax element
//
class DebuggerStatementNode : public StatementNode
{
  public:
    explicit inline DebuggerStatementNode()
      : StatementNode(DebuggerStatement)
    {}
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
    inline FunctionDeclarationNode(FunctionExpressionNode *func)
      : SourceElementNode(FunctionDeclaration),
        func_(func)
    {
        WH_ASSERT(func->name());
    }

    inline FunctionExpressionNode *func() const {
        return func_;
    }
};

//
// Program syntax element
//
class ProgramNode : public BaseNode
{
  private:
    SourceElementList sourceElements_;

  public:
    explicit inline ProgramNode(SourceElementList &&sourceElements)
      : BaseNode(Program),
        sourceElements_(sourceElements)
    {}

    inline const SourceElementList &sourceElements() const {
        return sourceElements_;
    }
};


} // namespace AST
} // namespace Whisper


#endif // WHISPER__PARSER__SYNTAX_TREE_HPP
