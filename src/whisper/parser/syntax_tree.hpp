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

class FileDeclarationNode;
class BlockElementNode;
class StatementNode;
class ExpressionNode;
class LiteralExpressionNode;
class TypenameNode;

template <typename BASE, NodeType TYPE, typename TOKTYPE>
class SingleTokenNode;

// Concrete pre-declarations.

class FileNode;

class ModuleDeclNode;
class ImportDeclNode;
class FuncDeclNode;
class BlockNode;

class ReturnStmtNode;
class EmptyStmtNode;
class ExprStmtNode;

typedef SingleTokenNode<ExpressionNode, IdentifierExpr, IdentifierToken>
        IdentifierExprNode;

typedef SingleTokenNode<LiteralExpressionNode, IntegerLiteralExpr,
                        IntegerLiteralToken>
        IntegerLiteralExprNode;

typedef SingleTokenNode<TypenameNode, IntType, IntKeywordToken>
        IntTypeNode;

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

class FileDeclarationNode : public BaseNode
{
  protected:
    inline FileDeclarationNode(NodeType type) : BaseNode(type) {}
};

class BlockElementNode : public BaseNode
{
  protected:
    inline BlockElementNode(NodeType type) : BaseNode(type) {}
};

class StatementNode : public BlockElementNode
{
  protected:
    inline StatementNode(NodeType type) : BlockElementNode(type) {}

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

class TypenameNode : public BaseNode
{
  protected:
    TypenameNode(NodeType type) : BaseNode(type) {}
};

typedef BaseNode::List<IdentifierToken> IdentifierTokenList;
typedef BaseNode::List<ExpressionNode *> ExpressionList;
typedef BaseNode::List<StatementNode *> StatementList;
typedef BaseNode::List<BlockElementNode *> BlockElementList;

//
// SingleTokenNode is a template base class for syntax nodes whose
// contents are a single token.
// 
template <typename BASE, NodeType TYPE, typename TOKTYPE>
class SingleTokenNode : public BASE
{
  private:
    TOKTYPE token_;

  public:
    inline SingleTokenNode(const TOKTYPE &token)
      : BASE(TYPE),
        token_(token)
    {
        WH_ASSERT(token_.isValid());
    }

    inline const TOKTYPE &token() const {
        return token_;
    }
};

/////////////
//         //
//  Types  //
//         //
/////////////

//
// IntTypeNode syntax element
//
// see typedef for IntTypeNode in pre-declarations


///////////////////
//               //
//  Expressions  //
//               //
///////////////////

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
// Block syntax element
//
class BlockNode : public StatementNode
{
  private:
    BlockElementList elements_;

  public:
    explicit inline BlockNode(BlockElementList &&elements)
      : StatementNode(Block),
        elements_(elements)
    {}

    inline const BlockElementList &elements() const {
        return elements_;
    }
};

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

//
// ReturnStmt syntax element
//
class ReturnStmtNode : public StatementNode
{
  private:
    ExpressionNode *value_;

  public:
    explicit inline ReturnStmtNode(ExpressionNode *value)
      : StatementNode(ReturnStmt),
        value_(value)
    {}

    inline bool hasValue() const {
        return value_;
    }

    inline ExpressionNode *value() const {
        WH_ASSERT(hasValue());
        return value_;
    }
};


/////////////////
//             //
//  Functions  //
//             //
/////////////////

//
// FuncDecl syntax element
//
class FuncDeclNode : public FileDeclarationNode
{
  public:
    class Param {
      private:
        TypenameNode *type_;
        IdentifierToken name_;

      public:
        Param(TypenameNode *type, const IdentifierToken &name)
          : type_(type),
            name_(name)
        {
            WH_ASSERT(type_);
        }

        const TypenameNode *type() const {
            return type_;
        }

        const IdentifierToken &name() const {
            return name_;
        }
    };

    typedef List<Param> ParamList;

  private:
    VisibilityToken visibility_;
    TypenameNode *returnType_;
    IdentifierToken name_;
    ParamList params_;
    BlockNode *body_;

  public:
    inline FuncDeclNode(const VisibilityToken &visibility,
                        TypenameNode *returnType,
                        const IdentifierToken &name,
                        ParamList &&params,
                        BlockNode *body)
      : FileDeclarationNode(FuncDecl),
        visibility_(visibility),
        returnType_(returnType),
        name_(name),
        params_(params),
        body_(body)
    {
        WH_ASSERT(returnType_);
        WH_ASSERT(name_.isValid());
        WH_ASSERT(body_);
    }

    inline bool hasVisibility() const {
        return !visibility_.isINVALID();
    }

    inline const VisibilityToken &visibility() const {
        WH_ASSERT(hasVisibility());
        return visibility_;
    }

    inline const TypenameNode *returnType() const {
        return returnType_;
    }

    inline const IdentifierToken &name() const {
        return name_;
    }

    inline const ParamList &params() const {
        return params_;
    }

    inline const BlockNode *body() const {
        return body_;
    }
};


/////////////////
//             //
//  Top Level  //
//             //
/////////////////

//
// Import declaration
//
class ImportDeclNode : public BaseNode
{
  public:
    class Member {
      private:
        IdentifierToken name_;
        IdentifierToken asName_;

      public:
        Member(const IdentifierToken &name)
          : name_(name),
            asName_()
        {
            WH_ASSERT(name_.isValid());
        }

        Member(const IdentifierToken &name, const IdentifierToken &asName)
          : name_(name),
            asName_(asName)
        {
            WH_ASSERT(name_.isValid());
            WH_ASSERT(asName_.isValid());
        }

        const IdentifierToken &name() const {
            return name_;
        }

        bool hasAsName() const {
            return !asName_.isINVALID();
        }
        const IdentifierToken &asName() const {
            return asName_;
        }
    };
    typedef List<Member> MemberList;

  private:
    IdentifierTokenList path_;
    IdentifierToken asName_;
    MemberList members_;

  public:
    explicit inline ImportDeclNode(IdentifierTokenList &&path,
                                   const IdentifierToken &asName,
                                   MemberList &&members)
      : BaseNode(ImportDecl),
        path_(path),
        asName_(asName),
        members_(members)
    {}

    inline const IdentifierTokenList &path() const {
        return path_;
    }

    inline bool hasAsName() const {
        return !asName_.isINVALID();
    }

    inline const IdentifierToken &asName() const {
        WH_ASSERT(hasAsName());
        return asName_;
    }

    inline const MemberList &members() const {
        return members_;
    }
};

//
// Module declaration
//
class ModuleDeclNode : public BaseNode
{
  private:
    IdentifierTokenList path_;

  public:
    explicit inline ModuleDeclNode(IdentifierTokenList &&path)
      : BaseNode(ModuleDecl),
        path_(path)
    {}

    inline const IdentifierTokenList &path() const {
        return path_;
    }
};

//
// File
//
class FileNode : public BaseNode
{
  public:
    typedef List<ImportDeclNode *> ImportDeclList;
    typedef List<FileDeclarationNode *> FileDeclarationList;

  private:
    ModuleDeclNode *module_;
    ImportDeclList imports_;
    FileDeclarationList contents_;

  public:
    explicit inline FileNode(ModuleDeclNode *module,
                             ImportDeclList &&imports,
                             FileDeclarationList &&contents)
      : BaseNode(File),
        module_(module),
        imports_(imports),
        contents_(contents)
    {}

    inline bool hasModule() const {
        return module_;
    }

    inline const ModuleDeclNode *module() const {
        WH_ASSERT(hasModule());
        return module_;
    }

    inline const ImportDeclList &imports() const {
        return imports_;
    }

    inline const FileDeclarationList &contents() const {
        return contents_;
    }
};


} // namespace AST
} // namespace Whisper


#endif // WHISPER__PARSER__SYNTAX_TREE_HPP
