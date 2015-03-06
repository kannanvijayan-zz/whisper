
#include <algorithm>
#include "runtime_inlines.hpp"
#include "parser/packed_writer.hpp"

namespace Whisper {
namespace AST {


bool
PackedWriter::writeNode(const BaseNode *node)
{
    WH_ASSERT(!hasError());
    try {
        switch (node->type()) {
#define CASE_(ntype) \
          case ntype: \
            write(static_cast<uint32_t>(node->type())); \
            write##ntype(node->to##ntype()); \
            break;
    WHISPER_DEFN_SYNTAX_NODES(CASE_)
#undef CASE_
          default:
            WH_UNREACHABLE("Invalid node type.");
        }
    } catch (PackedWriterError err) {
        return false;

    } catch (BumpAllocatorError err) {
        error_ = "Memory allocation failed when writing packed syntax tree.";
        return false;
    }

    return true;
}

void
PackedWriter::expandBuffer()
{
    WH_ASSERT(!hasError());
    uint32_t oldSize = end_ - start_;
    uint32_t cursorOffset = cursor_ - start_;
    uint32_t newSize = oldSize ? oldSize * 2 : InitialBufferSize;

    uint32_t *data = allocator_.allocate(newSize);
    std::copy(start_, end_, data);
    if (start_)
        allocator_.deallocate(start_, oldSize);
    start_ = data;
    end_ = data + newSize;
    cursor_ = start_ + cursorOffset;
}

uint32_t
PackedWriter::addIdentifier(const IdentifierToken &ident)
{
    // Check identifier map.
    uint32_t bytes = ident.length();
    IdentifierKey key(ident.text(src_), bytes);
    IdentifierMap::const_iterator iter = identifierMap_.find(key);
    if (iter != identifierMap_.end())
        return iter->second;

    uint32_t stringSize = VM::String::CalculateSize(bytes);
    VM::String *str = acx_.createSized<VM::String>(stringSize, bytes,
                                                   ident.text(src_));
    if (!str)
        emitError("Could not allocate identifier.");

    return addToConstPool(GC::AllocThing::From(str));
}

uint32_t
PackedWriter::addToConstPool(GC::AllocThing *ptr)
{
    WH_ASSERT(constPoolSize_ <= constPoolCapacity_);
    if (constPoolSize_ == constPoolCapacity_)
        expandConstPool();
    uint32_t idx = constPoolSize_;
    constPool_[idx] = ptr;
    constPoolSize_++;
    return idx;
}

void
PackedWriter::expandConstPool()
{
    WH_ASSERT(!hasError());
    STLBumpAllocator<GC::AllocThing *> alloc(allocator_);
    uint32_t newCapacity = constPoolCapacity_ * 2;
    if (newCapacity == 0)
        newCapacity = InitialConstPoolSize;

    GC::AllocThing **newConstPool = alloc.allocate(newCapacity);
    std::copy(constPool_, constPool_ + constPoolSize_, newConstPool);
    if (constPool_)
        alloc.deallocate(constPool_, constPoolCapacity_);
    constPool_ = newConstPool;
    constPoolCapacity_ = newCapacity;
}

void
PackedWriter::writeParenExpr(const ParenExprNode *node)
{
    writeNode(node->subexpr());
}

void
PackedWriter::writeNameExpr(const NameExprNode *node)
{
    uint32_t identIdx = addIdentifier(node->name());
    write(identIdx);
}

void
PackedWriter::parseIntegerLiteral(const IntegerLiteralToken &token,
                                  int32_t *resultOut)
{
    WH_ASSERT(resultOut != nullptr);

    const uint8_t *text = token.text(src_);
    uint32_t len = token.length();
    if (token.hasFlag(Token::Int_BinPrefix)) {
        WH_ASSERT(len > 2);
        WH_ASSERT(text[0] == '0');
        WH_ASSERT(text[1] == 'b');
        unsigned bits = 0;
        int32_t result = 0;
        for (uint32_t i = 2; i < len; i++) {
            unic_t ch = text[i];
            WH_ASSERT((ch == '_') || Tokenizer::IsBinDigit(ch));

            if (ch == '_')
                continue;

            result <<= 1;
            result |= (ch - static_cast<uint8_t>('0'));

            bits++;
            if (bits > 31)
                emitError("Binary integer literal too large.");
        }
        *resultOut = result;
        return;
    }

    if (token.hasFlag(Token::Int_OctPrefix)) {
        WH_ASSERT(len > 2);
        WH_ASSERT(text[0] == '0');
        WH_ASSERT(text[1] == 'o');
        unsigned bits = 0;
        int32_t result = 0;
        for (uint32_t i = 2; i < len; i++) {
            uint8_t ch = text[i];
            WH_ASSERT((ch == '_') || Tokenizer::IsOctDigit(ch));

            if (ch == '_')
                continue;

            result <<= 3;
            result |= (ch - static_cast<uint8_t>('0'));

            bits += 3;
            if (bits > 31)
                emitError("Octal integer literal too large.");
        }
        *resultOut = result;
        return;
    }

    if (token.hasFlag(Token::Int_DecPrefix)) {
        WH_ASSERT(len > 2);
        WH_ASSERT(text[0] == '0');
        WH_ASSERT(text[1] == 'd');
        int32_t result = 0;
        for (uint32_t i = 2; i < len; i++) {
            uint8_t ch = text[i];
            WH_ASSERT((ch == '_') || Tokenizer::IsDecDigit(ch));

            if (ch == '_')
                continue;

            int digit = ch - static_cast<uint8_t>('0');
            if (result > ((std::numeric_limits<int32_t>::max() - digit) / 10))
                emitError("Decimal integer literal too large.");

            result *= 10;
            result += digit;
        }
        *resultOut = result;
        return;
    }

    if (token.hasFlag(Token::Int_HexPrefix)) {
        WH_ASSERT(len > 2);
        WH_ASSERT(text[0] == '0');
        WH_ASSERT(text[1] == 'x');
        unsigned bits = 0;
        int32_t result = 0;
        for (uint32_t i = 2; i < len; i++) {
            uint8_t ch = text[i];
            WH_ASSERT((ch == '_') || Tokenizer::IsHexDigit(ch));

            if (ch == '_')
                continue;

            int digit = ch - static_cast<uint8_t>('0');
            if (result > ((std::numeric_limits<int32_t>::max() - digit) / 10))

            result <<= 4;
            result |= digit;

            bits += 4;
            if (bits > 31)
                emitError("Hex integer literal too large.");
        }
        *resultOut = result;
        return;
    }
}

void
PackedWriter::writeIntegerExpr(const IntegerExprNode *node)
{
    int32_t val;
    parseIntegerLiteral(node->token(), &val);
    write(static_cast<uint32_t>(val));
}

void
PackedWriter::writeDotExpr(const DotExprNode *node)
{
    // Write name index, followed by target expression.
    uint32_t identIdx = addIdentifier(node->name());
    write(identIdx);
    writeNode(node->target());
}

void
PackedWriter::writeArrowExpr(const ArrowExprNode *node)
{
    // Write name index, followed by target expression.
    uint32_t identIdx = addIdentifier(node->name());
    write(identIdx);
    writeNode(node->target());
}

void
PackedWriter::writeCallExpr(const CallExprNode *node)
{
    // TypeExtra: nargs
    // Format [argOffset1, ..., argOffsetN, callee..., arg1..., ..., argN...]
    uint32_t nargs = node->args().size();
    if (nargs > MaxArgs)
        emitError("Too many arguments in call expression.");
    writeTypeExtra(nargs);

    Position argOffsetPos = position();
    for (uint32_t i = 0; i < nargs; i++)
        writeDummy();

    // Write out callee, then arguments.
    writeNode(node->callee());
    for (const Expression *arg : node->args()) {
        writeOffsetDistance(&argOffsetPos);
        writeNode(arg);
    }
}

void
PackedWriter::writePosExpr(const PosExprNode *node)
{
    writeNode(node->subexpr());
}

void
PackedWriter::writeNegExpr(const NegExprNode *node)
{
    writeNode(node->subexpr());
}

void
PackedWriter::writeBinaryExpr(const Expression *lhs, const Expression *rhs)
{
    Position rhsOffsetPos = position();
    writeNode(lhs);
    writeOffsetDistance(&rhsOffsetPos);
    writeNode(rhs);
}

void
PackedWriter::writeMulExpr(const MulExprNode *node)
{
    writeBinaryExpr(node->lhs(), node->rhs());
}

void
PackedWriter::writeDivExpr(const DivExprNode *node)
{
    writeBinaryExpr(node->lhs(), node->rhs());
}

void
PackedWriter::writeAddExpr(const AddExprNode *node)
{
    writeBinaryExpr(node->lhs(), node->rhs());
}

void
PackedWriter::writeSubExpr(const SubExprNode *node)
{
    writeBinaryExpr(node->lhs(), node->rhs());
}

void
PackedWriter::writeEmptyStmt(const EmptyStmtNode *node)
{
}

void
PackedWriter::writeExprStmt(const ExprStmtNode *node)
{
    writeNode(node->expr());
}

void
PackedWriter::writeReturnStmt(const ReturnStmtNode *node)
{
    if (node->hasExpr()) {
        writeTypeExtra(1);
        writeNode(node->expr());
    }
}

void
PackedWriter::writeBlock(const Block *block)
{
    WH_ASSERT(block->statements().size() <= MaxBlockStatements);
    Position offsetPosn = position();
    for (uint32_t i = 0; i < block->statements().size(); i++)
        writeDummy();

    for (const Statement *stmt : block->statements()) {
        writeOffsetDistance(&offsetPosn);
        writeNode(stmt);
    }
}

void
PackedWriter::writeSizedBlock(const Block *block)
{
    if (block->statements().size() > MaxBlockStatements)
        emitError("Too many block statements.");
    write(block->statements().size());
    writeBlock(block);
}

void
PackedWriter::writeIfStmt(const IfStmtNode *node)
{
    // TypeExtra contains:
    // (NumElsifClauses << 1) | HasElseClause

    // Format:
    //  [ offsetOfIfBlock,
    //    offsetOfElsifCond1, offsetOfElsifBlock1, ...
    //    offsetOfElseBlock ]

    uint32_t numElsifs = node->elsifPairs().size();
    bool hasElse = node->hasElseBlock();
    writeTypeExtra((numElsifs << 1) | (hasElse ? 1 : 0));

    // Save offset position for elsif clauses
    Position offsetPos = position();
    writeDummy();   // offsetOfIfBlock
    for (uint32_t i = 0; i < numElsifs; i++) {
        writeDummy();   // offsetOfElsifCond[i]
        writeDummy();   // offsetOfElsifBlock[i]
    }
    if (hasElse)
        writeDummy();   // offsetOfElseBlock

    writeNode(node->ifPair().cond());
    writeOffsetDistance(&offsetPos);
    writeSizedBlock(node->ifPair().block());

    for (const IfStmtNode::CondPair &elsifPair : node->elsifPairs()) {
        writeOffsetDistance(&offsetPos);
        writeNode(elsifPair.cond());
        writeOffsetDistance(&offsetPos);
        writeSizedBlock(elsifPair.block());
    }

    if (node->hasElseBlock()) {
        writeOffsetDistance(&offsetPos);
        writeSizedBlock(node->elseBlock());
    }
}

void
PackedWriter::writeDefStmt(const DefStmtNode *node)
{
    uint32_t numParams = node->paramNames().size();
    if (numParams > MaxParams)
        emitError("Too many params.");

    writeTypeExtra(numParams);

    uint32_t nameIdx = addIdentifier(node->name());
    write(nameIdx);

    for (const IdentifierToken &paramName : node->paramNames()) {
        uint32_t paramIdx = addIdentifier(paramName);
        write(paramIdx);
    }

    writeSizedBlock(node->bodyBlock());
}

void
PackedWriter::writeVarStmt(const VarStmtNode *node)
{
    uint32_t numBindings = node->bindings().size();
    if (numBindings > MaxBindings)
        emitError("Too many bindings in var statement.");

    writeTypeExtra(numBindings);

    Position offsetPos = position();
    for (uint32_t i = 0; i < numBindings; i++)
        writeDummy();

    for (const BindingStatement::Binding &binding : node->bindings()) {
        writeOffsetDistance(&offsetPos);

        uint32_t bindingIdx = addIdentifier(binding.name());
        bool hasValue = binding.hasValue();
        uint32_t idxAndFlag = bindingIdx;
        if (hasValue)
            idxAndFlag |= static_cast<uint32_t>(1) << 31;
        write(idxAndFlag);

        if (hasValue)
            writeNode(binding.value());
    }
}

void
PackedWriter::writeConstStmt(const ConstStmtNode *node)
{
    uint32_t numBindings = node->bindings().size();
    if (numBindings > MaxBindings)
        emitError("Too many bindings in const statement.");

    writeTypeExtra(numBindings);

    Position offsetPos = position();
    for (uint32_t i = 0; i < numBindings; i++)
        writeDummy();

    for (const BindingStatement::Binding &binding : node->bindings()) {
        writeOffsetDistance(&offsetPos);

        uint32_t bindingIdx = addIdentifier(binding.name());
        write(bindingIdx);
        writeNode(binding.value());
    }
}

void
PackedWriter::writeLoopStmt(const LoopStmtNode *node)
{
    writeTypeExtra(node->bodyBlock()->statements().size());
    writeBlock(node->bodyBlock());
}

void
PackedWriter::writeFile(const FileNode *node)
{
    uint32_t numStatements = node->statements().size();
    writeTypeExtra(numStatements);

    WH_ASSERT(numStatements <= MaxBlockStatements);
    Position offsetPosn = position();
    for (uint32_t i = 0; i < numStatements; i++)
        writeDummy();

    for (const Statement *stmt : node->statements()) {
        writeOffsetDistance(&offsetPosn);
        writeNode(stmt);
    }
}


} // namespace AST
} // namespace Whisper
