#ifndef WHISPER__PARSER__PACKED_READER_HPP
#define WHISPER__PARSER__PACKED_READER_HPP

#include <cstdio>
#include <unordered_map>
#include "allocators.hpp"
#include "runtime.hpp"
#include "fnv_hash.hpp"
#include "parser/code_source.hpp"
#include "parser/packed_syntax.hpp"
#include "vm/string.hpp"
#include "vm/box.hpp"
#include "vm/array.hpp"

namespace Whisper {
namespace AST {


class PackedReader;

class PackedVisitor
{
  protected:
    PackedVisitor() {}

  public:
    virtual ~PackedVisitor() {}

#define METHOD_(ntype) \
    virtual void visit##ntype(PackedReader const& reader, \
                              Packed##ntype##Node packedNode) \
    { \
        WH_UNREACHABLE("Abstract base method visit" #ntype); \
    }
    WHISPER_DEFN_SYNTAX_NODES(METHOD_)
#undef METHOD_
};

class PackedReader
{
    friend class TraceTraits<PackedReader>;

  private:
    // Packed syntax tree.
    StackField<VM::Array<uint32_t>*> text_;
    StackField<VM::Array<VM::Box>*> constPool_;

  public:
    PackedReader(VM::Array<uint32_t>* text, VM::Array<VM::Box>* constPool)
      : text_(text),
        constPool_(constPool)
    {}

    VM::Array<uint32_t>* text() const {
        return text_;
    }
    VM::Array<VM::Box>* constPool() const {
        return constPool_;
    }

    VM::Box constant(uint32_t idx) const {
        return constPool_->get(idx);
    }

    void visitNode(PackedBaseNode node, PackedVisitor* visitor) const;
    void visit(PackedVisitor* visitor) const {
        visitNode(PackedBaseNode(text_, 0), visitor);
    }
};

template <typename Printer>
class PrintingPackedVisitor : public PackedVisitor
{
    Printer& printer_;
    unsigned tabDepth_;

  public:
    PrintingPackedVisitor(Printer& printer)
      : printer_(printer),
        tabDepth_(0)
    {}

    virtual ~PrintingPackedVisitor() {}

    virtual void visitFile(PackedReader const& reader,
                           PackedFileNode file)
        override
    {
        for (uint32_t i = 0; i < file.numStatements(); i++)
            reader.visitNode(file.statement(i), this);
    }

    virtual void visitEmptyStmt(PackedReader const& reader,
                                PackedEmptyStmtNode emptyStmt)
        override
    {
        pr(";\n");
    }

    virtual void visitExprStmt(PackedReader const& reader,
                               PackedExprStmtNode exprStmt)
        override
    {
        // visit expression.
        reader.visitNode(exprStmt.expression(), this);
        pr(";\n");
    }

    virtual void visitReturnStmt(PackedReader const& reader,
                                 PackedReturnStmtNode returnStmt)
        override
    {
        pr("return");
        if (returnStmt.hasExpression()) {
            pr(" ");
            reader.visitNode(returnStmt.expression(), this);
        }
        pr(";\n");
    }

    virtual void visitIfStmt(PackedReader const& reader,
                             PackedIfStmtNode ifStmt)
        override
    {
        pr("if (");
        reader.visitNode(ifStmt.ifCond(), this);
        pr(") ");
        visitBlock(reader, ifStmt.ifBlock());

        for (uint32_t i = 0; i < ifStmt.numElsifs(); i++) {
            pr(" elsif (");
            reader.visitNode(ifStmt.elsifCond(i), this);
            pr(") ");
            visitBlock(reader, ifStmt.elsifBlock(i));
        }

        if (ifStmt.hasElse()) {
            pr(" else ");
            visitBlock(reader, ifStmt.elseBlock());
        }
        pr("\n");
    }

    virtual void visitDefStmt(PackedReader const& reader,
                              PackedDefStmtNode defStmt)
        override
    {
        pr("def ");
        visitIdentifier(reader, defStmt.nameCid());
        pr("(");

        for (uint32_t i = 0; i < defStmt.numParams(); i++) {
            if (i > 0)
                pr(", ");
            visitIdentifier(reader, defStmt.paramCid(i));
        }
        pr(") ");
        visitBlock(reader, defStmt.bodyBlock());
        pr("\n");
    }

    virtual void visitConstStmt(PackedReader const& reader,
                                PackedConstStmtNode constStmt)
        override
    {
        pr("const ");
        for (uint32_t i = 0; i < constStmt.numBindings(); i++) {
            if (i > 0)
                pr(", ");
            visitIdentifier(reader, constStmt.varnameCid(i));
            pr(" = ");
            reader.visitNode(constStmt.varexpr(i), this);
        }
        pr(";\n");
    }

    virtual void visitVarStmt(PackedReader const& reader,
                              PackedVarStmtNode varStmt)
        override
    {
        pr("var ");
        for (uint32_t i = 0; i < varStmt.numBindings(); i++) {
            if (i > 0)
                pr(", ");
            visitIdentifier(reader, varStmt.varnameCid(i));
            if (varStmt.hasVarexpr(i)) {
                pr(" = ");
                reader.visitNode(varStmt.varexpr(i), this);
            }
        }
        pr(";\n");
    }

    virtual void visitLoopStmt(PackedReader const& reader,
                               PackedLoopStmtNode loopStmt)
        override
    {
        pr("loop ");
        visitBlock(reader, loopStmt.bodyBlock());
        pr("\n");
    }

    virtual void visitCallExpr(PackedReader const& reader,
                               PackedCallExprNode callExpr)
        override
    {
        reader.visitNode(callExpr.callee(), this);
        pr("(");
        for (uint32_t i = 0; i < callExpr.numArgs(); i++) {
            if (i > 0)
                pr(", ");
            reader.visitNode(callExpr.arg(i), this);
        }
        pr(")");
    }

    virtual void visitDotExpr(PackedReader const& reader,
                              PackedDotExprNode dotExpr)
        override
    {
        reader.visitNode(dotExpr.target(), this);
        pr(".");
        visitIdentifier(reader, dotExpr.nameCid());
    }

    virtual void visitArrowExpr(PackedReader const& reader,
                                PackedArrowExprNode arrowExpr)
        override
    {
        reader.visitNode(arrowExpr.target(), this);
        pr("->");
        visitIdentifier(reader, arrowExpr.nameCid());
    }

    virtual void visitPosExpr(PackedReader const& reader,
                              PackedPosExprNode posExpr)
        override
    {
        visitUnaryExpr(reader, posExpr.subexpr(), "+");
    }

    virtual void visitNegExpr(PackedReader const& reader,
                              PackedNegExprNode negExpr)
        override
    {
        visitUnaryExpr(reader, negExpr.subexpr(), "-");
    }

    virtual void visitAddExpr(PackedReader const& reader,
                              PackedAddExprNode addExpr)
        override
    {
        visitBinaryExpr(reader, addExpr.lhs(), addExpr.rhs(), "+");
    }

    virtual void visitSubExpr(PackedReader const& reader,
                              PackedSubExprNode subExpr)
        override
    {
        visitBinaryExpr(reader, subExpr.lhs(), subExpr.rhs(), "-");
    }

    virtual void visitMulExpr(PackedReader const& reader,
                              PackedMulExprNode mulExpr)
        override
    {
        visitBinaryExpr(reader, mulExpr.lhs(), mulExpr.rhs(), "*");
    }

    virtual void visitDivExpr(PackedReader const& reader,
                              PackedDivExprNode divExpr)
        override
    {
        visitBinaryExpr(reader, divExpr.lhs(), divExpr.rhs(), "/");
    }

    virtual void visitParenExpr(PackedReader const& reader,
                                PackedParenExprNode parenExpr)
        override
    {
        pr("(");
        reader.visitNode(parenExpr.subexpr(), this);
        pr(")");
    }

    virtual void visitNameExpr(PackedReader const& reader,
                               PackedNameExprNode nameExpr)
        override
    {
        visitIdentifier(reader, nameExpr.nameCid());
    }

    virtual void visitIntegerExpr(PackedReader const& reader,
                                  PackedIntegerExprNode integerExpr)
        override
    {
        char buf[64];
        snprintf(buf, 64, "%d", integerExpr.value());
        pr(buf);
    }

  private:

    void tab(uint32_t depth) {
        for (unsigned i = 0; i < depth; i++)
            pr("  ");
    }
    void tab() { tab(tabDepth_); }

    void pr(char const* text) {
        printer_(text);
    }

    void visitUnaryExpr(PackedReader const& reader,
                        PackedBaseNode node,
                        char const* op)
    {
        pr("(| ");
        pr(op);
        pr(" ");
        reader.visitNode(node, this);
        pr(" |)");
    }

    void visitBinaryExpr(PackedReader const& reader,
                         PackedBaseNode lhs,
                         PackedBaseNode rhs,
                         char const* op)
    {
        pr("(| ");
        reader.visitNode(lhs, this);
        pr(op);
        reader.visitNode(rhs, this);
        pr(" |)");
    }

    void visitIdentifier(PackedReader const& reader,
                         uint32_t idx)
    {
        VM::Box box = reader.constant(idx);
        WH_ASSERT(box.isPointer());
        WH_ASSERT(box.pointer<HeapThing>()->isString());
        VM::String* string = box.pointer<VM::String>();
        for (uint32_t i = 0; i < string->byteLength(); i++) {
            char cs[2];
            cs[0] = string->bytes()[i];
            cs[1] = '\0';
            pr(cs);
        }
    }

    void visitBlock(PackedReader const& reader, PackedBlockNode block)
    {
        pr("{\n");
        tabDepth_++;
        for (uint32_t i = 0; i < block.numStatements(); i++) {
            tab();
            reader.visitNode(block.statement(i), this);
        }
        tabDepth_--;
        tab();
        pr("}");
    }
};


} // namespace AST


// GC-specialize PackedReader as a stack type.
template <>
struct StackTraits<AST::PackedReader>
{
    StackTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr StackFormat Format = StackFormat::PackedReader;
};

template <>
struct StackFormatTraits<StackFormat::PackedReader>
{
    StackFormatTraits() = delete;
    typedef AST::PackedReader Type;
};

template <>
struct TraceTraits<AST::PackedReader>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner& scanner, AST::PackedReader const& pr,
                     void const* start, void const* end)
    {
        pr.text_.scan(scanner, start, end);
        pr.constPool_.scan(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater& updater, AST::PackedReader& pr,
                       void const* start, void const* end)
    {
        pr.text_.update(updater, start, end);
        pr.constPool_.update(updater, start, end);
    }
};


} // namespace Whisper

#endif // WHISPER__PARSER__PACKED_READER_HPP
