#ifndef WHISPER__PARSER__SYNTAX_TREE_HPP
#define WHISPER__PARSER__SYNTAX_TREE_HPP

#include <list>
#include "parser/tokenizer.hpp"
#include "parser/syntax_defn.hpp"

namespace Whisper {
namespace AST {


//
// The syntax nodes are used to build a syntax tree of parsed script
// or function body.
//

enum NodeType : uint8_t
{
    INVALID,
#define DEF_ENUM_(node)   node,
    WHISPER_DEFN_SYNTAX_NODES(DEF_ENUM_)
#undef DEF_ENUM_
    LIMIT
};

char const* NodeTypeString(NodeType nodeType);

//
// Pre-declarations.
//

// Abstract pre-declarations.

class Statement;
class Expression;
class PropertyExpression;
class LiteralExpression;

template <NodeType TYPE> class UnaryExpression;
template <NodeType TYPE> class BinaryExpression;

// Concrete pre-declarations.

class FileNode;
class BlockNode;

class EmptyStmtNode;
class ExprStmtNode;
class ReturnStmtNode;
class IfStmtNode;
class DefStmtNode;
class ConstStmtNode;
class VarStmtNode;
class LoopStmtNode;

class CallExprNode;
class DotExprNode;
class ArrowExprNode;

typedef UnaryExpression<NegExpr> NegExprNode;
typedef UnaryExpression<PosExpr> PosExprNode;

typedef BinaryExpression<MulExpr> MulExprNode;
typedef BinaryExpression<DivExpr> DivExprNode;
typedef BinaryExpression<AddExpr> AddExprNode;
typedef BinaryExpression<SubExpr> SubExprNode;

class NameExprNode;
class IntegerExprNode;

class ParenExprNode;

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

    inline char const* typeString() const {
        return NodeTypeString(type_);
    }

#define METHODS_(node) \
    inline bool is##node() const { \
        return type_ == node; \
    } \
    inline node##Node const* to##node() const { \
        WH_ASSERT(is##node()); \
        return reinterpret_cast<node##Node const*>(this); \
    } \
    inline node##Node* to##node() { \
        WH_ASSERT(is##node()); \
        return reinterpret_cast<node##Node*>(this); \
    }
    WHISPER_DEFN_SYNTAX_NODES(METHODS_)
#undef METHODS_
};

template <typename Writer>
void WritePacked(CodeSource const& src, BaseNode const* node, Writer& wr);

template <typename Printer>
void PrintNode(CodeSource const& src, BaseNode const* node, Printer pr,
               int tabDepth);

///////////////////////////////////////
//                                   //
//  Intermediate and Helper Classes  //
//                                   //
///////////////////////////////////////

class Statement : public BaseNode
{
  protected:
    inline Statement(NodeType type) : BaseNode(type) {}
};

class Expression : public BaseNode
{
  protected:
    inline Expression(NodeType type) : BaseNode(type) {}
};

class LiteralExpression : public Expression
{
  protected:
    LiteralExpression(NodeType type) : Expression(type) {}
};

typedef BaseNode::List<Expression*> ExpressionList;
typedef BaseNode::List<Statement*> StatementList;
typedef BaseNode::List<IdentifierToken> IdentifierList;


///////////////////
//               //
//  Expressions  //
//               //
///////////////////

//
// PropertyExpression
//
class PropertyExpression : public Expression
{
  private:
    Expression* target_;
    IdentifierToken name_;

  public:
    PropertyExpression(NodeType type, Expression* target,
                       IdentifierToken const& name)
      : Expression(type),
        target_(target),
        name_(name)
    {
        WH_ASSERT_IF(type != NameExpr, target_);
    }

    bool hasTarget() const {
        return target_ != nullptr;
    }

    Expression const* target() const {
        WH_ASSERT(hasTarget());
        return target_;
    }

    IdentifierToken const& name() const {
        return name_;
    }
};

//
// ArrowExpr
//
class ArrowExprNode : public PropertyExpression
{
  public:
    ArrowExprNode(Expression* target, IdentifierToken const& name)
      : PropertyExpression(ArrowExpr, target, name)
    {}
};

//
// DotExpr
//
class DotExprNode : public PropertyExpression
{
  public:
    DotExprNode(Expression* target, IdentifierToken const& name)
      : PropertyExpression(DotExpr, target, name)
    {}
};

//
// CallExpr
//
class CallExprNode : public Expression
{
  private:
    PropertyExpression* callee_;
    ExpressionList args_;

  public:
    CallExprNode(PropertyExpression* callee, ExpressionList&& args)
      : Expression(CallExpr),
        callee_(callee),
        args_(std::move(args))
    {}

    PropertyExpression const* callee() const {
        return callee_;
    }

    ExpressionList const& args() const {
        return args_;
    }
};

//
// BinaryExpr template class.
//
template <NodeType TYPE>
class BinaryExpression : public Expression
{
  private:
    Expression* lhs_;
    Expression* rhs_;

  public:
    BinaryExpression(Expression* lhs, Expression* rhs)
      : Expression(TYPE),
        lhs_(lhs),
        rhs_(rhs)
    {
        WH_ASSERT(lhs_);
        WH_ASSERT(rhs_);
    }

    Expression const* lhs() const {
        return lhs_;
    }

    Expression const* rhs() const {
        return rhs_;
    }
};

//
// UnaryExpr template class.
//
template <NodeType TYPE>
class UnaryExpression : public Expression
{
  private:
    Expression* subexpr_;

  public:
    UnaryExpression(Expression* subexpr)
      : Expression(TYPE),
        subexpr_(subexpr)
    {
        WH_ASSERT(subexpr);
    }

    Expression const* subexpr() const {
        return subexpr_;
    }
};

//
// NameExpr
//
class NameExprNode : public PropertyExpression
{
  public:
    NameExprNode(IdentifierToken const& name)
      : PropertyExpression(NameExpr, nullptr, name)
    {}
};

//
// IntegerExpr
//
class IntegerExprNode : public LiteralExpression
{
  private:
    IntegerLiteralToken token_;

  public:
    IntegerExprNode(IntegerLiteralToken const& token)
      : LiteralExpression(IntegerExpr),
        token_(token)
    {}

    IntegerLiteralToken const& token() const {
        return token_;
    }
};

//
// ParenExpr syntax element
//
class ParenExprNode : public Expression
{
  private:
    Expression* subexpr_;

  public:
    ParenExprNode(Expression* subexpr)
      : Expression(ParenExpr),
        subexpr_(subexpr)
    {
        WH_ASSERT(subexpr);
    }

    Expression const* subexpr() const {
        return subexpr_;
    }
};

//
// IdentifierExpr syntax element
//
// See typedef for IdentifierExprNode in pre-declarations.

//
// IntegerLiteralExpr syntax element
//
// See typedef for IntegerLiteralExprNode in pre-declarations.


//////////////////
//              //
//  Statements  //
//              //
//////////////////

//
// BindingStatement
//
class BindingStatement : public Statement
{
  public:
    class Binding
    {
      private:
        IdentifierToken name_;
        Expression* value_;

      public:
        Binding(IdentifierToken const& name, Expression* value)
          : name_(name),
            value_(value)
        {}

        IdentifierToken const& name() const {
            return name_;
        }

        bool hasValue() const {
            return value_ != nullptr;
        }

        Expression const* value() const {
            WH_ASSERT(hasValue());
            return value_;
        }
    };

    typedef List<Binding> BindingList;

  private:
    BindingList bindings_;

  public:
    explicit BindingStatement(NodeType type, BindingList&& bindings)
      : Statement(type),
        bindings_(std::move(bindings))
    {}

    BindingList const& bindings() const {
        return bindings_;
    }
};

//
// BlockNode represents { ... } statement lists.
//
class BlockNode : public Statement
{
  private:
    StatementList statements_;

  public:
    inline BlockNode(StatementList&& statements)
      : Statement(Block),
        statements_(std::move(statements))
    {}

    StatementList const& statements() const {
        return statements_;
    }
};

//
// EmptyStmt syntax element
//
class EmptyStmtNode : public Statement
{
  public:
    inline EmptyStmtNode() : Statement(EmptyStmt) {}
};

//
// ExprStmt syntax element
//
class ExprStmtNode : public Statement
{
  private:
    Expression* expr_;

  public:
    explicit inline ExprStmtNode(Expression* expr)
      : Statement(ExprStmt),
        expr_(expr)
    {
        WH_ASSERT(expr_);
    }

    inline Expression* expr() const {
        return expr_;
    }
};

//
// ReturnStmt syntax element
//
class ReturnStmtNode : public Statement
{
  private:
    Expression* expr_;

  public:
    explicit inline ReturnStmtNode(Expression* expr)
      : Statement(ReturnStmt),
        expr_(expr)
    {}

    inline bool hasExpr() const {
        return expr_ != nullptr;
    }

    inline Expression* expr() const {
        WH_ASSERT(hasExpr());
        return expr_;
    }
};

//
// IfStmt syntax element
//
class IfStmtNode : public Statement
{
  public:
    class CondPair
    {
      private:
        Expression* cond_;
        BlockNode* block_;

      public:
        CondPair(Expression* cond, BlockNode* block)
          : cond_(cond),
            block_(block)
        {
            WH_ASSERT(cond_);
            WH_ASSERT(block_);
        }

        Expression const* cond() const {
            return cond_;
        }
        BlockNode const* block() const {
            return block_;
        }
    };
    typedef List<CondPair> CondPairList;

  private:
    CondPair ifPair_;
    CondPairList elsifPairs_;
    BlockNode* elseBlock_;

  public:
    explicit IfStmtNode(CondPair const& ifPair,
                        CondPairList&& elsifPairs,
                        BlockNode* elseBlock)
      : Statement(IfStmt),
        ifPair_(ifPair),
        elsifPairs_(std::move(elsifPairs)),
        elseBlock_(elseBlock)
    {}

    CondPair const& ifPair() const {
        return ifPair_;
    }

    CondPairList const& elsifPairs() const {
        return elsifPairs_;
    }

    bool hasElseBlock() const {
        return elseBlock_ != nullptr;
    }

    BlockNode* elseBlock() const {
        WH_ASSERT(hasElseBlock());
        return elseBlock_;
    }
};

//
// DefStmt
//
class DefStmtNode : public Statement
{
  private:
    IdentifierToken name_;
    IdentifierList paramNames_;
    BlockNode* bodyBlock_;

  public:
    explicit DefStmtNode(IdentifierToken const& name,
                         IdentifierList&& paramNames,
                         BlockNode* bodyBlock)
      : Statement(DefStmt),
        name_(name),
        paramNames_(std::move(paramNames)),
        bodyBlock_(bodyBlock)
    {
        WH_ASSERT(bodyBlock_);
    }

    IdentifierToken const& name() const {
        return name_;
    }

    IdentifierList const& paramNames() const {
        return paramNames_;
    }

    BlockNode const* bodyBlock() const {
        return bodyBlock_;
    }
};

//
// ConstStmt
//
class ConstStmtNode : public BindingStatement
{
  public:
    explicit ConstStmtNode(BindingList&& bindings)
      : BindingStatement(ConstStmt, std::move(bindings))
    {}
};

//
// VarStmt
//
class VarStmtNode : public BindingStatement
{
  public:
    explicit VarStmtNode(BindingList&& bindings)
      : BindingStatement(VarStmt, std::move(bindings))
    {}
};

//
// LoopStmt
//
class LoopStmtNode : public Statement
{
  private:
    BlockNode* bodyBlock_;

  public:
    explicit LoopStmtNode(BlockNode* bodyBlock)
      : Statement(LoopStmt),
        bodyBlock_(bodyBlock)
    {
        WH_ASSERT(bodyBlock_);
    }

    BlockNode const* bodyBlock() const {
        return bodyBlock_;
    }
};


/////////////////
//             //
//  Top Level  //
//             //
/////////////////

//
// File
//
class FileNode : public BaseNode
{
  private:
    StatementList statements_;

  public:
    explicit inline FileNode(StatementList&& statements)
      : BaseNode(File),
        statements_(std::move(statements))
    {}

    inline StatementList const& statements() const {
        return statements_;
    }
};


} // namespace AST
} // namespace Whisper


#endif // WHISPER__PARSER__SYNTAX_TREE_HPP
