#ifndef WHISPER__PARSER__PACKED_READER_HPP
#define WHISPER__PARSER__PACKED_READER_HPP

#include <cstdio>
#include <unordered_map>
#include "allocators.hpp"
#include "runtime.hpp"
#include "fnv_hash.hpp"
#include "parser/code_source.hpp"
#include "parser/syntax_tree.hpp"
#include "vm/string.hpp"

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
                              const uint32_t *posn, \
                              uint32_t typeExtra) \
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
    GC::AllocThing *const *constPool_;
    uint32_t constPoolSize_;

  public:
    PackedReader(const uint32_t *buffer,
                 uint32_t bufferSize,
                 GC::AllocThing *const *constPool,
                 uint32_t constPoolSize)
      : buffer_(buffer),
        bufferSize_(bufferSize),
        constPool_(constPool),
        constPoolSize_(constPoolSize)
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

    GC::AllocThing *constant(uint32_t idx) const {
        WH_ASSERT(idx < constPoolSize_);
        return constPool_[idx];
    }

    void visitAt(const uint32_t *position, PackedVisitor *visitor) const;
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
                           const uint32_t *posn,
                           uint32_t typeExtra)
        override
    {
        uint32_t numStatements = typeExtra;
        fprintf(stderr, "visitFile: numStatements = %d (%08x)\n", (int)numStatements, *posn);
        const uint32_t *offsetPosn = posn + 1;
        for (uint32_t i = 0; i < numStatements; i++) {
            // Visit i'th statement.
            reader.visitAt(readIncrOffsetPosn(&offsetPosn), this);
        }
    }

    virtual void visitEmptyStmt(const PackedReader &reader,
                                const uint32_t *posn,
                                uint32_t typeExtra)
        override
    {
        pr(";\n");
    }

    virtual void visitExprStmt(const PackedReader &reader,
                               const uint32_t *posn,
                               uint32_t typeExtra)
        override
    {
        // visit expression.
        reader.visitAt(posn + 1, this);
        pr(";\n");
    }

    virtual void visitReturnStmt(const PackedReader &reader,
                                 const uint32_t *posn,
                                 uint32_t typeExtra)
        override
    {
        pr("return");
        if (typeExtra) {
            // visit return expression.
            pr(" ");
            reader.visitAt(posn + 1, this);
        }
        pr(";\n");
    }

    virtual void visitIfStmt(const PackedReader &reader,
                             const uint32_t *posn,
                             uint32_t typeExtra)
        override
    {
        bool hasElse = typeExtra & 1;
        uint32_t numElsifs = typeExtra >> 1;

        const uint32_t *offsetPosn = posn + 1;
        pr("if (");
        const uint32_t *ifCondPosn = offsetPosn + numElsifs + (hasElse ? 1 : 0);
        reader.visitAt(ifCondPosn, this);
        pr(") ");
        const uint32_t *ifBlockPosn = offsetPosn + *offsetPosn;
        visitSizedBlock(reader, ifBlockPosn);
        offsetPosn++;

        for (uint32_t i = 0; i < numElsifs; i++) {
            pr(" elsif (");
            reader.visitAt(readIncrOffsetPosn(&offsetPosn), this);
            pr(") ");
            visitSizedBlock(reader, readIncrOffsetPosn(&offsetPosn));
        }

        if (hasElse) {
            pr(" else ");
            visitSizedBlock(reader, readIncrOffsetPosn(&offsetPosn));
            offsetPosn++;
        }
        pr("\n");
    }

    virtual void visitDefStmt(const PackedReader &reader,
                              const uint32_t *posn,
                              uint32_t typeExtra)
        override
    {
        uint32_t numParams = typeExtra;

        pr("def ");
        visitIdentifier(reader, posn[1]);
        pr("(");

        for (uint32_t i = 0; i < numParams; i++) {
            if (i > 0)
                pr(", ");
            visitIdentifier(reader, posn[2 + i]);
        }
        pr(") ");
        visitSizedBlock(reader, posn + 2 + numParams);
        pr("\n");
    }

    virtual void visitConstStmt(const PackedReader &reader,
                                const uint32_t *posn,
                                uint32_t typeExtra)
        override
    {
        uint32_t numBindings = typeExtra;

        const uint32_t *offsetPosn = posn + 1;
        pr("const ");

        for (uint32_t i = 0; i < numBindings; i++) {
            if (i > 0)
                pr(", ");
            const uint32_t *bindingPosn = readIncrOffsetPosn(&offsetPosn);
            uint32_t bindingIdx = *bindingPosn;
            visitIdentifier(reader, bindingIdx);
            pr(" = ");
            reader.visitAt(bindingPosn + 1, this);
        }
        pr(";\n");
    }

    virtual void visitVarStmt(const PackedReader &reader,
                              const uint32_t *posn,
                              uint32_t typeExtra)
        override
    {
        uint32_t numBindings = typeExtra;

        const uint32_t *offsetPosn = posn + 1;
        pr("var ");

        for (uint32_t i = 0; i < numBindings; i++) {
            if (i > 0)
                pr(", ");
            const uint32_t *bindingPosn = readIncrOffsetPosn(&offsetPosn);
            uint32_t hasValueMask = static_cast<uint32_t>(1) << 31;
            bool hasValue = !!(*bindingPosn & hasValueMask);
            uint32_t bindingIdx = *bindingPosn & ~hasValueMask;
            visitIdentifier(reader, bindingIdx);
            if (hasValue) {
                pr(" = ");
                reader.visitAt(bindingPosn + 1, this);
            }
        }
        pr(";\n");
    }

    virtual void visitLoopStmt(const PackedReader &reader,
                               const uint32_t *posn,
                               uint32_t typeExtra)
        override
    {
        uint32_t blockSize = typeExtra;
        pr("loop ");
        visitBlock(reader, blockSize, posn + 1);
        pr("\n");
    }

    virtual void visitCallExpr(const PackedReader &reader,
                               const uint32_t *posn,
                               uint32_t typeExtra)
        override
    {
        uint32_t numArgs = typeExtra;

        const uint32_t *offsetPosn = posn + 1;

        // print callee expr
        reader.visitAt(offsetPosn + numArgs, this);
        pr("(");
        for (uint32_t i = 0; i < typeExtra; i++) {
            if (i > 0)
                pr(", ");
            reader.visitAt(readIncrOffsetPosn(&offsetPosn), this);
        }
        pr(")");
    }

    virtual void visitDotExpr(const PackedReader &reader,
                              const uint32_t *posn,
                              uint32_t typeExtra)
        override
    {
        reader.visitAt(posn + 2, this);
        pr(".");
        visitIdentifier(reader, posn[1]);
    }

    virtual void visitArrowExpr(const PackedReader &reader,
                                const uint32_t *posn,
                                uint32_t typeExtra)
        override
    {
        reader.visitAt(posn + 2, this);
        pr("->");
        visitIdentifier(reader, posn[1]);
    }

    virtual void visitPosExpr(const PackedReader &reader,
                              const uint32_t *posn,
                              uint32_t typeExtra)
        override
    {
        visitUnaryExpr(reader, posn, "+");
    }

    virtual void visitNegExpr(const PackedReader &reader,
                              const uint32_t *posn,
                              uint32_t typeExtra)
        override
    {
        visitUnaryExpr(reader, posn, "+");
    }

    virtual void visitAddExpr(const PackedReader &reader,
                              const uint32_t *posn,
                              uint32_t typeExtra)
        override
    {
        visitBinaryExpr(reader, posn, "+");
    }

    virtual void visitSubExpr(const PackedReader &reader,
                              const uint32_t *posn,
                              uint32_t typeExtra)
        override
    {
        visitBinaryExpr(reader, posn, "-");
    }

    virtual void visitMulExpr(const PackedReader &reader,
                              const uint32_t *posn,
                              uint32_t typeExtra)
        override
    {
        visitBinaryExpr(reader, posn, "*");
    }

    virtual void visitDivExpr(const PackedReader &reader,
                              const uint32_t *posn,
                              uint32_t typeExtra)
        override
    {
        visitBinaryExpr(reader, posn, "/");
    }

    virtual void visitParenExpr(const PackedReader &reader,
                                const uint32_t *posn,
                                uint32_t typeExtra)
        override
    {
        pr("(");
        reader.visitAt(posn + 1, this);
        pr(")");
    }

    virtual void visitNameExpr(const PackedReader &reader,
                               const uint32_t *posn,
                               uint32_t typeExtra)
        override
    {
        visitIdentifier(reader, posn[1]);
    }

    virtual void visitIntegerExpr(const PackedReader &reader,
                                  const uint32_t *posn,
                                  uint32_t typeExtra)
        override
    {
        char buf[64];
        snprintf(buf, 64, "%d", static_cast<int32_t>(posn[1]));
        pr(buf);
    }

  private:
    const uint32_t *readIncrOffsetPosn(const uint32_t **offsetPosnPtr) {
        const uint32_t *offsetPosn = *offsetPosnPtr;
        ++*offsetPosnPtr;
        return offsetPosn + *offsetPosn;
    }

    void tab(uint32_t depth) {
        for (unsigned i = 0; i < depth; i++)
            pr("  ");
    }
    void tab() { tab(tabDepth_); }

    void pr(const char *text) {
        printer_(text);
    }

    void visitUnaryExpr(const PackedReader &reader,
                        const uint32_t *posn,
                        const char *op)
    {
        pr(op);
        reader.visitAt(posn + 1, this);
    }

    void visitBinaryExpr(const PackedReader &reader,
                         const uint32_t *posn,
                         const char *op)
    {
        const uint32_t *offsetPosn = posn + 1;
        pr("@(");
        reader.visitAt(posn + 2, this);
        pr(op);
        reader.visitAt(readIncrOffsetPosn(&offsetPosn), this);
        pr(")@");
    }

    void visitIdentifier(const PackedReader &reader,
                         uint32_t idx)
    {
        GC::AllocThing *thing = reader.constant(idx);
        WH_ASSERT(thing->header().isFormat_String());
        VM::String *string = reinterpret_cast<VM::String *>(thing);
        for (uint32_t i = 0; i < string->byteLength(); i++) {
            char cs[2];
            cs[0] = string->bytes()[i];
            cs[1] = '\0';
            pr(cs);
        }
    }

    void visitSizedBlock(const PackedReader &reader, const uint32_t *block)
    {
        visitBlock(reader, *block, block + 1);
    }

    void visitBlock(const PackedReader &reader, uint32_t size,
                    const uint32_t *block)
    {
        const uint32_t *offsetPosn = block;

        pr("{\n");
        tabDepth_++;
        for (uint32_t i = 0; i < size; i++) {
            tab();
            reader.visitAt(readIncrOffsetPosn(&offsetPosn), this);
        }
        tabDepth_--;
        tab();
        pr("}");
    }
};


} // namespace AST
} // namespace Whisper

#endif // WHISPER__PARSER__PACKED_READER_HPP
