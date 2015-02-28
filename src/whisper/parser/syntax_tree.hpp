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

class StatementNode;
class ExpressionNode;
class PropertyExpressionNode;
class LiteralExpressionNode;

template <NodeType TYPE> class UnaryExpressionNode;
template <NodeType TYPE> class BinaryExpressionNode;

// Concrete pre-declarations.

class FileNode;

class EmptyStmtNode;
class ExprStmtNode;

class CallExprNode;
class DotExprNode;
class ArrowExprNode;

typedef UnaryExpressionNode<NegExpr> NegExprNode;
typedef UnaryExpressionNode<PosExpr> PosExprNode;
typedef BinaryExpressionNode<MulExpr> MulExprNode;
typedef BinaryExpressionNode<DivExpr> DivExprNode;
typedef BinaryExpressionNode<AddExpr> AddExprNode;
typedef BinaryExpressionNode<SubExpr> SubExprNode;

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

class StatementNode : public BaseNode
{
  protected:
    inline StatementNode(NodeType type) : BaseNode(type) {}

  public:
    virtual inline bool isStatement() const override {
        return true;
    }
};

class ExpressionNode : public BaseNode
{
  protected:
    inline ExpressionNode(NodeType type) : BaseNode(type) {}

  public:
    virtual inline bool isExpression() const override {
        return true;
    }
};

class LiteralExpressionNode : public ExpressionNode
{
  protected:
    LiteralExpressionNode(NodeType type) : ExpressionNode(type) {}
};

typedef BaseNode::List<ExpressionNode *> ExpressionList;
typedef BaseNode::List<StatementNode *> StatementList;


///////////////////
//               //
//  Expressions  //
//               //
///////////////////

//
// PropertyExpressionNode
//
class PropertyExpressionNode : public ExpressionNode
{
  private:
    ExpressionNode *target_;
    IdentifierToken name_;

  public:
    PropertyExpressionNode(NodeType type, ExpressionNode *target,
                           const IdentifierToken &name)
      : ExpressionNode(type),
        target_(target),
        name_(name)
    {
        WH_ASSERT_IF(type != NameExpr, target_);
    }

    bool hasTarget() const {
        return target_ != nullptr;
    }

    const ExpressionNode *target() const {
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
class ArrowExprNode : public PropertyExpressionNode
{
  public:
    ArrowExprNode(ExpressionNode *target, const IdentifierToken &name)
      : PropertyExpressionNode(ArrowExpr, target, name)
    {}
};

//
// DotExpr
//
class DotExprNode : public PropertyExpressionNode
{
  public:
    DotExprNode(ExpressionNode *target, const IdentifierToken &name)
      : PropertyExpressionNode(DotExpr, target, name)
    {}
};

//
// CallExpr
//
class CallExprNode : public ExpressionNode
{
  private:
    PropertyExpressionNode *receiver_;
    ExpressionList args_;

  public:
    CallExprNode(PropertyExpressionNode *receiver, ExpressionList &&args)
      : ExpressionNode(CallExpr),
        receiver_(receiver),
        args_(args)
    {}

    const PropertyExpressionNode *receiver() const {
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
class BinaryExpressionNode : public ExpressionNode
{
  private:
    ExpressionNode *lhs_;
    ExpressionNode *rhs_;

  public:
    BinaryExpressionNode(ExpressionNode *lhs, ExpressionNode *rhs)
      : ExpressionNode(TYPE),
        lhs_(lhs),
        rhs_(rhs)
    {
        WH_ASSERT(lhs_);
        WH_ASSERT(rhs_);
    }

    const ExpressionNode *lhs() const {
        return lhs_;
    }

    const ExpressionNode *rhs() const {
        return rhs_;
    }
};

//
// UnaryExpr template class.
//
template <NodeType TYPE>
class UnaryExpressionNode : public ExpressionNode
{
  private:
    ExpressionNode *subexpr_;

  public:
    UnaryExpressionNode(ExpressionNode *subexpr)
      : ExpressionNode(TYPE),
        subexpr_(subexpr)
    {
        WH_ASSERT(subexpr);
    }

    const ExpressionNode *subexpr() const {
        return subexpr_;
    }
};

//
// NameExpr
//
class NameExprNode : public PropertyExpressionNode
{
  public:
    NameExprNode(const IdentifierToken &name)
      : PropertyExpressionNode(NameExpr, nullptr, name)
    {}
};

//
// IntegerExpr
//
class IntegerExprNode : public LiteralExpressionNode
{
  private:
    IntegerLiteralToken token_;

  public:
    IntegerExprNode(const IntegerLiteralToken &token)
      : LiteralExpressionNode(IntegerExpr),
        token_(token)
    {}

    const IntegerLiteralToken &token() const {
        return token_;
    }
};

//
// ParenExpr syntax element
//
class ParenExprNode : public ExpressionNode
{
  private:
    ExpressionNode *subexpr_;

  public:
    ParenExprNode(ExpressionNode *subexpr)
      : ExpressionNode(ParenExpr),
        subexpr_(subexpr)
    {
        WH_ASSERT(subexpr);
    }

    const ExpressionNode *subexpr() const {
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
// EmptyStmt syntax element
//
class EmptyStmtNode : public StatementNode
{
  public:
    inline EmptyStmtNode() : StatementNode(EmptyStmt) {}
};

//
// ExprStmt syntax element
//
class ExprStmtNode : public StatementNode
{
  private:
    ExpressionNode *expr_;

  public:
    explicit inline ExprStmtNode(ExpressionNode *expr)
      : StatementNode(ExprStmt),
        expr_(expr)
    {
        WH_ASSERT(expr_);
    }

    inline ExpressionNode *expr() const {
        return expr_;
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
  public:
    typedef List<StatementNode *> StatementList;

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
