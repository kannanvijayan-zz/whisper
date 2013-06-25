#ifndef WHISPER__PARSER__SYNTAX_TREE_HPP
#define WHISPER__PARSER__SYNTAX_TREE_HPP

#include <list>
#include "parser/tokenizer.hpp"
#include "parser/syntax_defn.hpp"

namespace Whisper {
namespace AST {


//
// Base syntax element.
//
class BaseNode
{
  public:
    enum Type : uint8_t
    {
        INVALID,
#define DEF_ENUM_(node)   node,
        WHISPER_DEFN_SYNTAX_NODES(DEF_ENUM_)
#undef DEF_ENUM_
        LIMIT
    };

    template <typename T>
    using Allocator = STLBumpAllocator<T>;

    typedef Allocator<uint8_t> StdAllocator;

    template <typename T>
    using List = std::list<T, Allocator<T>>;

  protected:
    Type type_;

    BaseNode(Type type) : type_(type) {}

  public:
    inline Type type() const {
        return type_;
    }
};

///////////////////////////////////////
//                                   //
//  Intermediate and Helper Classes  //
//                                   //
///////////////////////////////////////

class SourceElementNode : public BaseNode
{
  protected:
    SourceElementNode(Type type) : BaseNode(type) {}
};

class StatementNode : public SourceElementNode
{
  protected:
    StatementNode(Type type) : SourceElementNode(type) {}
};

class IterationStatementNode : public StatementNode
{
  protected:
    IterationStatementNode(Type type) : StatementNode(type) {}
};

class ExpressionNode : public BaseNode
{
  protected:
    ExpressionNode(Type type) : BaseNode(type) {}
};

class LiteralExpressionNode : public ExpressionNode
{
  protected:
    LiteralExpressionNode(Type type) : ExpressionNode(type) {}
};

class VariableDeclaration
{
  public:
    IdentifierNameToken name_;
    StatementNode *initialiser_;

  public:
    VariableDeclaration(const IdentifierNameToken &name,
                        StatementNode *initialiser)
      : name_(name),
        initialiser_(initialiser)
    {}

    inline const IdentifierNameToken &name() const {
        return name_;
    }
    inline StatementNode *initaliser() const {
        return initialiser_;
    }
};

//
// FunctionBase provides base elements for both
// FunctionDeclaration and FunctionExpression syntax nodes.
//
class FunctionBase
{
  public:
    typedef BaseNode::List<IdentifierNameToken> FormalParameterList;
    typedef BaseNode::List<SourceElementNode *> SourceElementList;

  private:
    FormalParameterList formalParameters_;
    SourceElementList functionBody_;

  public:
    FunctionBase(FormalParameterList &&formalParameters,
                 SourceElementList &&functionBody)
      : formalParameters_(formalParameters),
        functionBody_(functionBody)
    {}

    inline const FormalParameterList &formalParameters() const {
        return formalParameters_;
    }

    inline const SourceElementList &functionBody() const {
        return functionBody_;
    }
};

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
    explicit ThisNode(const ThisKeywordToken &token)
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
    explicit IdentifierNode(const IdentifierNameToken &token)
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
    explicit NullLiteralNode(const NullLiteralToken &token)
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
    explicit BooleanLiteralNode(const FalseLiteralToken &token)
      : LiteralExpressionNode(BooleanLiteral),
        token_(token)
    {}

    explicit BooleanLiteralNode(const TrueLiteralToken &token)
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
class NumericLiteralNode : public LiteralExpressionNode
{
  private:
    NumericLiteralToken value_;

  public:
    explicit NumericLiteralNode(const NumericLiteralToken &value)
      : LiteralExpressionNode(NumericLiteral),
        value_(value)
    {}

    inline const NumericLiteralToken value() const {
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
    explicit StringLiteralNode(const StringLiteralToken &value)
      : LiteralExpressionNode(StringLiteral),
        value_(value)
    {}

    inline const StringLiteralToken value() const {
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
    explicit RegularExpressionLiteralNode(
            const RegularExpressionLiteralToken &value)
      : LiteralExpressionNode(RegularExpressionLiteral),
        value_(value)
    {}

    inline const RegularExpressionLiteralToken value() const {
        return value_;
    }
};

//
// ArrayLiteralNode syntax element
//
class ArrayLiteralNode : public LiteralExpressionNode
{
  public:
    typedef List<ExpressionNode *> ExpressionList;

  private:
    ExpressionList elements_;

  public:
    explicit ArrayLiteralNode(ExpressionList &&elements)
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
    typedef OneOf<IdentifierNameToken, StringLiteralToken, NumericLiteralToken>
            PropertyName;

    class SlotDefinition
    {
      private:
        PropertyName name_;
        ExpressionNode *value_;

      public:
        SlotDefinition(const IdentifierNameToken &name, ExpressionNode *value)
          : name_(PropertyName::Make<0>(name)),
            value_(value)
        {}

        SlotDefinition(const StringLiteralToken &name, ExpressionNode *value)
          : name_(PropertyName::Make<1>(name)),
            value_(value)
        {}

        SlotDefinition(const NumericLiteralToken &name, ExpressionNode *value)
          : name_(PropertyName::Make<2>(name)),
            value_(value)
        {}

        inline bool hasIdentifierName() const {
            return name_.isNth(0);
        }

        inline bool hasStringName() const {
            return name_.isNth(1);
        }

        inline bool hasNumericName() const {
            return name_.isNth(2);
        }

        inline const IdentifierNameToken &identifierName() const {
            WH_ASSERT(hasIdentifierName());
            return name_.nthValue<0>();
        }

        inline const StringLiteralToken &stringName() const {
            WH_ASSERT(hasStringName());
            return name_.nthValue<1>();
        }

        inline const NumericLiteralToken &numericName() const {
            WH_ASSERT(hasNumericName());
            return name_.nthValue<2>();
        }

        inline ExpressionNode *value() const {
            return value_;
        }
    };

    class AccessorDefinition
    {
      protected:
        PropertyName name_;
        typedef BaseNode::List<SourceElementNode *> SourceElementList;
        SourceElementList body_;

      public:
        AccessorDefinition(const IdentifierNameToken &name,
                           SourceElementList &&body)
          : name_(PropertyName::Make<0>(name)),
            body_(body)
        {}

        AccessorDefinition(const StringLiteralToken &name,
                           SourceElementList &&body)
          : name_(PropertyName::Make<1>(name)),
            body_(body)
        {}

        AccessorDefinition(const NumericLiteralToken &name,
                           SourceElementList &&body)
          : name_(PropertyName::Make<2>(name)),
            body_(body)
        {}

        inline bool hasIdentifierName() const {
            return name_.isNth(0);
        }

        inline bool hasStringName() const {
            return name_.isNth(1);
        }

        inline bool hasNumericName() const {
            return name_.isNth(2);
        }

        inline const IdentifierNameToken &identifierName() const {
            WH_ASSERT(hasIdentifierName());
            return name_.nthValue<0>();
        }

        inline const StringLiteralToken &stringName() const {
            WH_ASSERT(hasStringName());
            return name_.nthValue<1>();
        }

        inline const NumericLiteralToken &numericName() const {
            WH_ASSERT(hasNumericName());
            return name_.nthValue<2>();
        }

        inline const SourceElementList &body() const {
            return body_;
        }
    };

    class GetterDefinition : public AccessorDefinition
    {
      public:
        GetterDefinition(const IdentifierNameToken &name,
                         SourceElementList &&body)
          : AccessorDefinition(name, std::move(body))
        {}

        GetterDefinition(const StringLiteralToken &name,
                         SourceElementList &&body)
          : AccessorDefinition(name, std::move(body))
        {}

        GetterDefinition(const NumericLiteralToken &name,
                         SourceElementList &&body)
          : AccessorDefinition(name, std::move(body))
        {}
    };

    class SetterDefinition : public AccessorDefinition
    {
      private:
        IdentifierNameToken parameter_;

      public:
        SetterDefinition(const IdentifierNameToken &name,
                         const IdentifierNameToken &parameter,
                         SourceElementList &&body)
          : AccessorDefinition(name, std::move(body)),
            parameter_(parameter)
        {}

        SetterDefinition(const StringLiteralToken &name,
                         const IdentifierNameToken &parameter,
                         SourceElementList &&body)
          : AccessorDefinition(name, std::move(body)),
            parameter_(parameter)
        {}

        SetterDefinition(const NumericLiteralToken &name,
                         const IdentifierNameToken &parameter,
                         SourceElementList &&body)
          : AccessorDefinition(name, std::move(body)),
            parameter_(parameter)
        {}
    };

    typedef OneOf<SlotDefinition, GetterDefinition, SetterDefinition>
            PropertyDefinition;

    typedef List<PropertyDefinition> PropertyDefinitionList;

  private:
    PropertyDefinitionList propertyDefinitions_;

  public:
    explicit ObjectLiteralNode(PropertyDefinitionList &&propertyDefinitions)
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
    explicit ParenthesizedExpressionNode(ExpressionNode *subexpression)
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
class FunctionExpressionNode : public ExpressionNode, public FunctionBase
{
  private:
    Maybe<IdentifierNameToken> name_;

  public:
    FunctionExpressionNode(FormalParameterList &&formalParameters,
                           SourceElementList &&functionBody)
      : ExpressionNode(FunctionExpression),
        FunctionBase(std::move(formalParameters), std::move(functionBody)),
        name_()
    {}

    FunctionExpressionNode(const IdentifierNameToken &name,
                           FormalParameterList &&formalParameters,
                           SourceElementList &&functionBody)
      : ExpressionNode(FunctionExpression),
        FunctionBase(std::move(formalParameters), std::move(functionBody)),
        name_(name)
    {}

    inline const Maybe<IdentifierNameToken> &name() const {
        return name_;
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
    GetElementExpressionNode(ExpressionNode *object,
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
    GetPropertyExpressionNode(ExpressionNode *object,
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
  public:
    typedef List<ExpressionNode *> ExpressionList;

  private:
    ExpressionNode *constructor_;
    ExpressionList arguments_;

  public:
    NewExpressionNode(ExpressionNode *constructor, ExpressionList &&arguments)
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
  public:
    typedef List<ExpressionNode *> ExpressionList;

  private:
    ExpressionNode *function_;
    ExpressionList arguments_;

  public:
    CallExpressionNode(ExpressionNode *function, ExpressionList &&arguments)
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
template <BaseNode::Type TYPE>
class UnaryExpressionNode : public ExpressionNode
{
    static_assert(TYPE == PostIncrementExpression ||
                  TYPE == PreIncrementExpression ||
                  TYPE == PostDecrementExpression ||
                  TYPE == PreDecrementExpression ||
                  TYPE == DeleteExpression ||
                  TYPE == VoidExpression ||
                  TYPE == PositiveExpression ||
                  TYPE == NegativeExpression ||
                  TYPE == ComplementExpression ||
                  TYPE == LogicalNotExpression,
                  "Invalid IncDecExpressionNode type.");
  private:
    ExpressionNode *subexpression_;

  public:
    explicit UnaryExpressionNode(ExpressionNode *subexpression)
      : ExpressionNode(TYPE),
        subexpression_(subexpression)
    {}

    inline ExpressionNode *subexpression() const {
        return subexpression_;
    }
};
typedef UnaryExpressionNode<BaseNode::PostIncrementExpression>
        PostIncrementExpressionNode;
typedef UnaryExpressionNode<BaseNode::PreIncrementExpression>
        PreIncrementExpressionNode;
typedef UnaryExpressionNode<BaseNode::PostDecrementExpression>
        PostDecrementExpressionNode;
typedef UnaryExpressionNode<BaseNode::PreDecrementExpression>
        PreDecrementExpressionNode;

typedef UnaryExpressionNode<BaseNode::DeleteExpression>
        DeleteExpressionNode;
typedef UnaryExpressionNode<BaseNode::VoidExpression>
        VoidExpressionNode;
typedef UnaryExpressionNode<BaseNode::PositiveExpression>
        PositiveExpressionNode;
typedef UnaryExpressionNode<BaseNode::NegativeExpression>
        NegativeExpressionNode;
typedef UnaryExpressionNode<BaseNode::ComplementExpression>
        ComplementExpressionNode;
typedef UnaryExpressionNode<BaseNode::LogicalNotExpression>
        LogicalNotExpressionNode;

//
// BinaryExpression syntax element
//
template <BaseNode::Type TYPE>
class BinaryExpressionNode : public ExpressionNode
{
    static_assert(TYPE == BaseNode::MultiplyExpression ||
                  TYPE == BaseNode::DivideExpression ||
                  TYPE == BaseNode::ModuloExpression ||
                  TYPE == BaseNode::AddExpression ||
                  TYPE == BaseNode::SubtractExpression ||
                  TYPE == BaseNode::LeftShiftExpression ||
                  TYPE == BaseNode::RightShiftExpression ||
                  TYPE == BaseNode::UnsignedRightShiftExpression ||
                  TYPE == BaseNode::LessThanExpression ||
                  TYPE == BaseNode::GreaterThanExpression ||
                  TYPE == BaseNode::LessEqualExpression ||
                  TYPE == BaseNode::GreaterEqualExpression ||
                  TYPE == BaseNode::InstanceofExpression ||
                  TYPE == BaseNode::InExpression ||
                  TYPE == BaseNode::EqualExpression ||
                  TYPE == BaseNode::NotEqualExpression ||
                  TYPE == BaseNode::StrictEqualExpression ||
                  TYPE == BaseNode::StrictNotEqualExpression ||
                  TYPE == BaseNode::BitAndExpression ||
                  TYPE == BaseNode::BitXorExpression ||
                  TYPE == BaseNode::BitOrExpression ||
                  TYPE == BaseNode::LogicalAndExpression ||
                  TYPE == BaseNode::LogicalOrExpression ||
                  TYPE == BaseNode::CommaExpression,
                  "Invalid IncDecExpressionNode type.");
  private:
    ExpressionNode *lhs_;
    ExpressionNode *rhs_;

  public:
    explicit BinaryExpressionNode(ExpressionNode *lhs, ExpressionNode *rhs)
      : ExpressionNode(TYPE),
        lhs_(lhs),
        rhs_(rhs)
    {}

    inline ExpressionNode *lhs() const {
        return lhs_;
    }

    inline ExpressionNode *rhs() const {
        return rhs_;
    }
};

typedef BinaryExpressionNode<BaseNode::MultiplyExpression>
        MultiplyExpressionNode;
typedef BinaryExpressionNode<BaseNode::DivideExpression>
        DivideExpressionNode;
typedef BinaryExpressionNode<BaseNode::ModuloExpression>
        ModuloExpressionNode;
typedef BinaryExpressionNode<BaseNode::AddExpression>
        AddExpressionNode;
typedef BinaryExpressionNode<BaseNode::SubtractExpression>
        SubtractExpressionNode;
typedef BinaryExpressionNode<BaseNode::LeftShiftExpression>
        LeftShiftExpressionNode;
typedef BinaryExpressionNode<BaseNode::RightShiftExpression>
        RightShiftExpressionNode;
typedef BinaryExpressionNode<BaseNode::UnsignedRightShiftExpression>
        UnsignedRightShiftExpressionNode;
typedef BinaryExpressionNode<BaseNode::LessThanExpression>
        LessThanExpressionNode;
typedef BinaryExpressionNode<BaseNode::GreaterThanExpression>
        GreaterThanExpressionNode;
typedef BinaryExpressionNode<BaseNode::LessEqualExpression>
        LessEqualExpressionNode;
typedef BinaryExpressionNode<BaseNode::GreaterEqualExpression>
        GreaterEqualExpressionNode;
typedef BinaryExpressionNode<BaseNode::InstanceofExpression>
        InstanceofExpressionNode;
typedef BinaryExpressionNode<BaseNode::InExpression>
        InExpressionNode;
typedef BinaryExpressionNode<BaseNode::EqualExpression>
        EqualExpressionNode;
typedef BinaryExpressionNode<BaseNode::NotEqualExpression>
        NotEqualExpressionNode;
typedef BinaryExpressionNode<BaseNode::StrictEqualExpression>
        StrictEqualExpressionNode;
typedef BinaryExpressionNode<BaseNode::StrictNotEqualExpression>
        StrictNotEqualExpressionNode;
typedef BinaryExpressionNode<BaseNode::BitAndExpression>
        BitAndExpressionNode;
typedef BinaryExpressionNode<BaseNode::BitXorExpression>
        BitXorExpressionNode;
typedef BinaryExpressionNode<BaseNode::BitOrExpression>
        BitOrExpressionNode;
typedef BinaryExpressionNode<BaseNode::LogicalAndExpression>
        LogicalAndExpressionNode;
typedef BinaryExpressionNode<BaseNode::LogicalOrExpression>
        LogicalOrExpressionNode;
typedef BinaryExpressionNode<BaseNode::CommaExpression>
        CommaExpressionNode;

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
template <BaseNode::Type TYPE>
class AssignmentExpressionNode : public ExpressionNode
{
    static_assert(TYPE == BaseNode::AssignExpression ||
                  TYPE == BaseNode::PlusAssignExpression ||
                  TYPE == BaseNode::MinusAssignExpression ||
                  TYPE == BaseNode::StarAssignExpression ||
                  TYPE == BaseNode::PercentAssignExpression ||
                  TYPE == BaseNode::ShiftLeftAssignExpression ||
                  TYPE == BaseNode::ShiftRightAssignExpression ||
                  TYPE == BaseNode::ShiftUnsignedRightAssignExpression ||
                  TYPE == BaseNode::BitAndAssignExpression ||
                  TYPE == BaseNode::BitOrAssignExpression ||
                  TYPE == BaseNode::BitXorAssignExpression ||
                  TYPE == BaseNode::DivideAssignExpression,
                  "Invalid IncDecExpressionNode type.");
  private:
    ExpressionNode *lhs_;
    ExpressionNode *rhs_;

  public:
    AssignmentExpressionNode(ExpressionNode *lhs,
                             ExpressionNode *rhs)
      : ExpressionNode(TYPE),
        lhs_(lhs),
        rhs_(rhs)
    {}

    inline ExpressionNode *lhs() const {
        return lhs_;
    }

    inline ExpressionNode *rhs() const {
        return rhs_;
    }
};
typedef AssignmentExpressionNode<BaseNode::AssignExpression>
        AssignExpressionNode;
typedef AssignmentExpressionNode<BaseNode::PlusAssignExpression>
        PlusAssignExpressionNode;
typedef AssignmentExpressionNode<BaseNode::MinusAssignExpression>
        MinusAssignExpressionNode;
typedef AssignmentExpressionNode<BaseNode::StarAssignExpression>
        StarAssignExpressionNode;
typedef AssignmentExpressionNode<BaseNode::PercentAssignExpression>
        PercentAssignExpressionNode;
typedef AssignmentExpressionNode<BaseNode::ShiftLeftAssignExpression>
        ShiftLeftAssignExpressionNode;
typedef AssignmentExpressionNode<BaseNode::ShiftRightAssignExpression>
        ShiftRightAssignExpressionNode;
typedef AssignmentExpressionNode<BaseNode::ShiftUnsignedRightAssignExpression>
        ShiftUnsignedRightAssignExpressionNode;
typedef AssignmentExpressionNode<BaseNode::BitAndAssignExpression>
        BitAndAssignExpressionNode;
typedef AssignmentExpressionNode<BaseNode::BitOrAssignExpression>
        BitOrAssignExpressionNode;
typedef AssignmentExpressionNode<BaseNode::BitXorAssignExpression>
        BitXorAssignExpressionNode;
typedef AssignmentExpressionNode<BaseNode::DivideAssignExpression>
        DivideAssignExpressionNode;


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
  public:
    typedef List<SourceElementNode *> SourceElementList;

  private:
    SourceElementList sourceElements_;

  public:
    explicit BlockNode(SourceElementList &&sourceElements)
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
  public:
    typedef List<VariableDeclaration> DeclarationList;

  private:
    DeclarationList declarations_;

  public:
    explicit VariableStatementNode(DeclarationList &&declarations)
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
    EmptyStatementNode() : StatementNode(EmptyStatement) {}
};

//
// ExpressionStatement syntax element
//
class ExpressionStatementNode : public StatementNode
{
  private:
    ExpressionNode *expression_;

  public:
    explicit ExpressionStatementNode(ExpressionNode *expression)
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
    IfStatementNode(ExpressionNode *condition,
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
// DoWhileStatement syntax element
//
class DoWhileStatementNode : public IterationStatementNode
{
  private:
    StatementNode *body_;
    ExpressionNode *condition_;

  public:
    DoWhileStatementNode(StatementNode *body,
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
    WhileStatementNode(ExpressionNode *condition,
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
    ForLoopStatementNode(ExpressionNode *initial,
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
    typedef List<VariableDeclaration> DeclarationList;
    DeclarationList initial_;
    ExpressionNode *condition_;
    ExpressionNode *update_;
    StatementNode *body_;

  public:
    ForLoopVarStatementNode(DeclarationList &&initial,
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
    ExpressionNode *var_;
    ExpressionNode *object_;
    StatementNode *body_;

  public:
    ForInStatementNode(ExpressionNode *var,
                       ExpressionNode *object,
                       StatementNode *body)
      : IterationStatementNode(ForInStatement),
        var_(var),
        object_(object),
        body_(body)
    {}

    inline ExpressionNode *var() const {
        return var_;
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
    VariableDeclaration var_;
    ExpressionNode *object_;
    StatementNode *body_;

  public:
    ForInVarStatementNode(const VariableDeclaration &var,
                          ExpressionNode *object,
                          StatementNode *body)
      : IterationStatementNode(ForInVarStatement),
        var_(var),
        object_(object),
        body_(body)
    {}

    inline const VariableDeclaration &var() const {
        return var_;
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
    ContinueStatementNode()
      : StatementNode(ContinueStatement),
        label_()
    {}

    explicit ContinueStatementNode(const IdentifierNameToken &label)
      : StatementNode(ContinueStatement),
        label_(label)
    {}

    inline const Maybe<IdentifierNameToken> &label() {
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
    BreakStatementNode()
      : StatementNode(BreakStatement),
        label_()
    {}

    explicit BreakStatementNode(const IdentifierNameToken &label)
      : StatementNode(BreakStatement),
        label_(label)
    {}

    inline const Maybe<IdentifierNameToken> &label() {
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
    explicit ReturnStatementNode(ExpressionNode *value)
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
    WithStatementNode(ExpressionNode *value,
                      StatementNode *body)
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
    typedef List<StatementNode *> StatementList;

    class CaseClause
    {
      private:
        ExpressionNode *expression_;
        StatementList statements_;

      public:
        CaseClause(ExpressionNode *expression,
                   StatementList &&statements)
          : expression_(expression),
            statements_(statements)
        {}

        CaseClause(const CaseClause &other)
          : expression_(other.expression_),
            statements_(other.statements_)
        {}

        CaseClause(CaseClause &&other)
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
    SwitchStatementNode(ExpressionNode *value,
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
    LabelledStatementNode(const IdentifierNameToken &label,
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
    explicit ThrowStatementNode(ExpressionNode *value)
      : StatementNode(ThrowStatement),
        value_(value)
    {}

    inline ExpressionNode *value() const {
        return value_;
    }
};

//
// TryCatchStatement syntax element
//
class TryCatchStatementNode : public StatementNode
{
  private:
    BlockNode *tryBlock_;
    IdentifierNameToken catchName_;
    BlockNode *catchBlock_;

  public:
    TryCatchStatementNode(BlockNode *tryBlock,
                          const IdentifierNameToken &catchName,
                          BlockNode *catchBlock)
      : StatementNode(TryCatchStatement),
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
class TryFinallyStatementNode : public StatementNode
{
  private:
    BlockNode *tryBlock_;
    BlockNode *finallyBlock_;

  public:
    TryFinallyStatementNode(BlockNode *tryBlock,
                            BlockNode *finallyBlock)
      : StatementNode(TryFinallyStatement),
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
class TryCatchFinallyStatementNode : public StatementNode
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
                                 BlockNode *finallyBlock)
      : StatementNode(TryCatchFinallyStatement),
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
    explicit DebuggerStatementNode()
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
class FunctionDeclarationNode : public SourceElementNode, public FunctionBase
{
  private:
    IdentifierNameToken name_;

  public:
    FunctionDeclarationNode(const IdentifierNameToken &name,
                            FormalParameterList &&formalParameters,
                            SourceElementList &&functionBody)
      : SourceElementNode(FunctionDeclaration),
        FunctionBase(std::move(formalParameters), std::move(functionBody)),
        name_(name)
    {}

    inline const IdentifierNameToken &name() const {
        return name_;
    }
};

//
// Program syntax element
//
class ProgramNode : public BaseNode
{
  public:
    typedef List<SourceElementNode *> SourceElementList;

  private:
    SourceElementList sourceElements_;

  public:
    explicit ProgramNode(SourceElementList &&sourceElements)
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
