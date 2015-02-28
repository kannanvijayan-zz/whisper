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

const char *NodeTypeString(NodeType nodeType);

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

class EmptyStmtNode;
class ExprStmtNode;
class ReturnStmtNode;
class IfStmtNode;
class DefStmtNode;

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

    virtual inline bool isExpression() const {
        return false;
    }
};

template <typename Printer>
void PrintNode(const CodeSource &source, const BaseNode *node, Printer printer,
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

  public:
    virtual inline bool isStatement() const override {
        return true;
    }
};

class Expression : public BaseNode
{
  protected:
    inline Expression(NodeType type) : BaseNode(type) {}

  public:
    virtual inline bool isExpression() const override {
        return true;
    }
};

class LiteralExpression : public Expression
{
  protected:
    LiteralExpression(NodeType type) : Expression(type) {}
};

typedef BaseNode::List<Expression *> ExpressionList;
typedef BaseNode::List<Statement *> StatementList;
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
    Expression *target_;
    IdentifierToken name_;

  public:
    PropertyExpression(NodeType type, Expression *target,
                       const IdentifierToken &name)
      : Expression(type),
        target_(target),
        name_(name)
    {
        WH_ASSERT_IF(type != NameExpr, target_);
    }

    bool hasTarget() const {
        return target_ != nullptr;
    }

    const Expression *target() const {
        WH_ASSERT(hasTarget());
        return target_;
    }

    const IdentifierToken &name() const {
        return name_;
    }
};

//
// ArrowExpr
//
class ArrowExprNode : public PropertyExpression
{
  public:
    ArrowExprNode(Expression *target, const IdentifierToken &name)
      : PropertyExpression(ArrowExpr, target, name)
    {}
};

//
// DotExpr
//
class DotExprNode : public PropertyExpression
{
  public:
    DotExprNode(Expression *target, const IdentifierToken &name)
      : PropertyExpression(DotExpr, target, name)
    {}
};

//
// CallExpr
//
class CallExprNode : public Expression
{
  private:
    PropertyExpression *receiver_;
    ExpressionList args_;

  public:
    CallExprNode(PropertyExpression *receiver, ExpressionList &&args)
      : Expression(CallExpr),
        receiver_(receiver),
        args_(args)
    {}

    const PropertyExpression *receiver() const {
        return receiver_;
    }

    const ExpressionList &args() const {
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
    Expression *lhs_;
    Expression *rhs_;

  public:
    BinaryExpression(Expression *lhs, Expression *rhs)
      : Expression(TYPE),
        lhs_(lhs),
        rhs_(rhs)
    {
        WH_ASSERT(lhs_);
        WH_ASSERT(rhs_);
    }

    const Expression *lhs() const {
        return lhs_;
    }

    const Expression *rhs() const {
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
    Expression *subexpr_;

  public:
    UnaryExpression(Expression *subexpr)
      : Expression(TYPE),
        subexpr_(subexpr)
    {
        WH_ASSERT(subexpr);
    }

    const Expression *subexpr() const {
        return subexpr_;
    }
};

//
// NameExpr
//
class NameExprNode : public PropertyExpression
{
  public:
    NameExprNode(const IdentifierToken &name)
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
    IntegerExprNode(const IntegerLiteralToken &token)
      : LiteralExpression(IntegerExpr),
        token_(token)
    {}

    const IntegerLiteralToken &token() const {
        return token_;
    }
};

//
// ParenExpr syntax element
//
class ParenExprNode : public Expression
{
  private:
    Expression *subexpr_;

  public:
    ParenExprNode(Expression *subexpr)
      : Expression(ParenExpr),
        subexpr_(subexpr)
    {
        WH_ASSERT(subexpr);
    }

    const Expression *subexpr() const {
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
// Block is a helper type to represent { ... } statement lists.
//
class Block
{
  private:
    StatementList statements_;

  public:
    inline Block(StatementList &&statements) : statements_(statements) {}

    const StatementList &statements() const {
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
    Expression *expr_;

  public:
    explicit inline ExprStmtNode(Expression *expr)
      : Statement(ExprStmt),
        expr_(expr)
    {
        WH_ASSERT(expr_);
    }

    inline Expression *expr() const {
        return expr_;
    }
};

//
// ReturnStmt syntax element
//
class ReturnStmtNode : public Statement
{
  private:
    Expression *expr_;

  public:
    explicit inline ReturnStmtNode(Expression *expr)
      : Statement(ReturnStmt),
        expr_(expr)
    {}

    inline bool hasExpr() const {
        return expr_ != nullptr;
    }

    inline Expression *expr() const {
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
    class CondPair {
      private:
        Expression *cond_;
        Block *block_;

      public:
        CondPair(Expression *cond, Block *block)
          : cond_(cond),
            block_(block)
        {
            WH_ASSERT(cond_);
            WH_ASSERT(block_);
        }

        const Expression *cond() const {
            return cond_;
        }
        const Block *block() const {
            return block_;
        }
    };
    typedef List<CondPair> CondPairList;

  private:
    CondPair ifPair_;
    CondPairList elsifPairs_;
    Block *elseBlock_;

  public:
    explicit IfStmtNode(const CondPair &ifPair,
                        CondPairList &&elsifPairs,
                        Block *elseBlock)
      : Statement(IfStmt),
        ifPair_(ifPair),
        elsifPairs_(elsifPairs),
        elseBlock_(elseBlock)
    {}

    const CondPair &ifPair() const {
        return ifPair_;
    }

    const CondPairList &elsifPairs() const {
        return elsifPairs_;
    }

    bool hasElseBlock() const {
        return elseBlock_ != nullptr;
    }

    Block *elseBlock() const {
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
    Block *bodyBlock_;

  public:
    explicit DefStmtNode(const IdentifierToken &name,
                         IdentifierList &&paramNames,
                         Block *bodyBlock)
      : Statement(DefStmt),
        name_(name),
        paramNames_(paramNames),
        bodyBlock_(bodyBlock)
    {
        WH_ASSERT(bodyBlock_);
    }

    const IdentifierToken &name() const {
        return name_;
    }

    const IdentifierList &paramNames() const {
        return paramNames_;
    }

    const Block *bodyBlock() const {
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
    explicit inline FileNode(StatementList &&statements)
      : BaseNode(File),
        statements_(statements)
    {}

    inline const StatementList &statements() const {
        return statements_;
    }
};


} // namespace AST
} // namespace Whisper


#endif // WHISPER__PARSER__SYNTAX_TREE_HPP
