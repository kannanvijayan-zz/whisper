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
    virtual void visit##ntype(const PackedReader &reader, \
                              Packed##ntype##Node packedNode) \
    { \
        WH_UNREACHABLE("Abstract base method visit" #ntype); \
    }
    WHISPER_DEFN_SYNTAX_NODES(METHOD_)
#undef METHOD_
};

class PackedReader
{
  private:
    // Packed syntax tree.
    const uint32_t *buffer_;
    uint32_t bufferSize_;

    // Constant pool.
    const VM::Box *constPool_;
    uint32_t constPoolSize_;

  public:
    PackedReader(ArrayHandle<uint32_t> buffer,
                 ArrayHandle<VM::Box> constPool)
      : buffer_(buffer.ptr()),
        bufferSize_(buffer.length()),
        constPool_(constPool.ptr()),
        constPoolSize_(constPool.length())
    {}

    const uint32_t *buffer() const {
        return buffer_;
    }
    const uint32_t *bufferEnd() const {
        return buffer_ + bufferSize_;
    }
    uint32_t bufferSize() const {
        return bufferSize_;
    }

    VM::Box constant(uint32_t idx) const {
        WH_ASSERT(idx < constPoolSize_);
        return constPool_[idx];
    }

    void visitNode(PackedBaseNode node, PackedVisitor *visitor) const;
    void visit(PackedVisitor *visitor) const {
        visitNode(PackedBaseNode(buffer_, bufferSize_), visitor);
    }
};

template <typename Printer>
class PrintingPackedVisitor : public PackedVisitor
{
    Printer &printer_;
    unsigned tabDepth_;

  public:
    PrintingPackedVisitor(Printer &printer)
      : printer_(printer),
        tabDepth_(0)
    {}

    virtual ~PrintingPackedVisitor() {}

    virtual void visitFile(const PackedReader &reader,
                           PackedFileNode file)
        override
    {
        for (uint32_t i = 0; i < file.numStatements(); i++)
            reader.visitNode(file.statement(i), this);
    }

    virtual void visitEmptyStmt(const PackedReader &reader,
                                PackedEmptyStmtNode emptyStmt)
        override
    {
        pr(";\n");
    }

    virtual void visitExprStmt(const PackedReader &reader,
                               PackedExprStmtNode exprStmt)
        override
    {
        // visit expression.
        reader.visitNode(exprStmt.expression(), this);
        pr(";\n");
    }

    virtual void visitReturnStmt(const PackedReader &reader,
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

    virtual void visitIfStmt(const PackedReader &reader,
                             PackedIfStmtNode ifStmt)
        override
    {
        pr("if (");
        reader.visitNode(ifStmt.ifCond(), this);
        pr(") ");
        visitSizedBlock(reader, ifStmt.ifBlock());

        for (uint32_t i = 0; i < ifStmt.numElsifs(); i++) {
            pr(" elsif (");
            reader.visitNode(ifStmt.elsifCond(i), this);
            pr(") ");
            visitSizedBlock(reader, ifStmt.elsifBlock(i));
        }

        if (ifStmt.hasElse()) {
            pr(" else ");
            visitSizedBlock(reader, ifStmt.elseBlock());
        }
        pr("\n");
    }

    virtual void visitDefStmt(const PackedReader &reader,
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
        visitSizedBlock(reader, defStmt.bodyBlock());
        pr("\n");
    }

    virtual void visitConstStmt(const PackedReader &reader,
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

    virtual void visitVarStmt(const PackedReader &reader,
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

    virtual void visitLoopStmt(const PackedReader &reader,
                               PackedLoopStmtNode loopStmt)
        override
    {
        pr("loop ");
        visitBlock(reader, loopStmt.bodyBlock());
        pr("\n");
    }

    virtual void visitCallExpr(const PackedReader &reader,
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

    virtual void visitDotExpr(const PackedReader &reader,
                              PackedDotExprNode dotExpr)
        override
    {
        reader.visitNode(dotExpr.target(), this);
        pr(".");
        visitIdentifier(reader, dotExpr.nameCid());
    }

    virtual void visitArrowExpr(const PackedReader &reader,
                                PackedArrowExprNode arrowExpr)
        override
    {
        reader.visitNode(arrowExpr.target(), this);
        pr("->");
        visitIdentifier(reader, arrowExpr.nameCid());
    }

    virtual void visitPosExpr(const PackedReader &reader,
                              PackedPosExprNode posExpr)
        override
    {
        visitUnaryExpr(reader, posExpr.subexpr(), "+");
    }

    virtual void visitNegExpr(const PackedReader &reader,
                              PackedNegExprNode negExpr)
        override
    {
        visitUnaryExpr(reader, negExpr.subexpr(), "-");
    }

    virtual void visitAddExpr(const PackedReader &reader,
                              PackedAddExprNode addExpr)
        override
    {
        visitBinaryExpr(reader, addExpr.lhs(), addExpr.rhs(), "+");
    }

    virtual void visitSubExpr(const PackedReader &reader,
                              PackedSubExprNode subExpr)
        override
    {
        visitBinaryExpr(reader, subExpr.lhs(), subExpr.rhs(), "-");
    }

    virtual void visitMulExpr(const PackedReader &reader,
                              PackedMulExprNode mulExpr)
        override
    {
        visitBinaryExpr(reader, mulExpr.lhs(), mulExpr.rhs(), "*");
    }

    virtual void visitDivExpr(const PackedReader &reader,
                              PackedDivExprNode divExpr)
        override
    {
        visitBinaryExpr(reader, divExpr.lhs(), divExpr.rhs(), "/");
    }

    virtual void visitParenExpr(const PackedReader &reader,
                                PackedParenExprNode parenExpr)
        override
    {
        pr("(");
        reader.visitNode(parenExpr.subexpr(), this);
        pr(")");
    }

    virtual void visitNameExpr(const PackedReader &reader,
                               PackedNameExprNode nameExpr)
        override
    {
        visitIdentifier(reader, nameExpr.nameCid());
    }

    virtual void visitIntegerExpr(const PackedReader &reader,
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

    void pr(const char *text) {
        printer_(text);
    }

    void visitUnaryExpr(const PackedReader &reader,
                        PackedBaseNode node,
                        const char *op)
    {
        pr("(| ");
        pr(op);
        pr(" ");
        reader.visitNode(node, this);
        pr(" |)");
    }

    void visitBinaryExpr(const PackedReader &reader,
                         PackedBaseNode lhs,
                         PackedBaseNode rhs,
                         const char *op)
    {
        pr("(| ");
        reader.visitNode(lhs, this);
        pr(op);
        reader.visitNode(rhs, this);
        pr(" |)");
    }

    void visitIdentifier(const PackedReader &reader,
                         uint32_t idx)
    {
        VM::Box box = reader.constant(idx);
        WH_ASSERT(box.isPointer());
        WH_ASSERT(box.pointer<HeapThing>()->isString());
        VM::String *string = box.pointer<VM::String>();
        for (uint32_t i = 0; i < string->byteLength(); i++) {
            char cs[2];
            cs[0] = string->bytes()[i];
            cs[1] = '\0';
            pr(cs);
        }
    }

    void visitSizedBlock(const PackedReader &reader, PackedSizedBlock block)
    {
        visitBlock(reader, block.unsizedBlock());
    }

    void visitBlock(const PackedReader &reader, PackedBlock block)
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
} // namespace Whisper

#endif // WHISPER__PARSER__PACKED_READER_HPP
