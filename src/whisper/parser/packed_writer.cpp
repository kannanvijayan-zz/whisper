
#include <algorithm>
#include "runtime_inlines.hpp"
#include "parser/packed_writer.hpp"

namespace Whisper {
namespace AST {


bool
PackedWriter::writeNode(BaseNode const* node)
{
    WH_ASSERT(!hasError());
    try {
        switch (node->type()) {
#define CASE_(ntype) \
          case ntype: \
            /*fprintf(stderr, "Writing NodeType %s at %d\n", */\
            /*        NodeTypeString(node->type()), */\
            /*        int(cursor_ - start_)); */\
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
    if (oldSize == MaxBufferSize)
        emitError("Exceeded maximum buffer size.");

    uint32_t cursorOffset = cursor_ - start_;
    uint32_t newSize = oldSize ? oldSize * 2 : InitialBufferSize;
    if (newSize > MaxBufferSize)
        newSize = MaxBufferSize;

    uint32_t* data = allocator_.allocate(newSize);
    std::copy(start_, end_, data);
    if (start_)
        allocator_.deallocate(start_, oldSize);
    start_ = data;
    end_ = data + newSize;
    cursor_ = start_ + cursorOffset;
}

uint32_t
PackedWriter::addIdentifier(IdentifierToken const& ident)
{
    // Check identifier map.
    uint32_t bytes = ident.length();
    IdentifierKey key(ident.text(src_), bytes);
    IdentifierMap::const_iterator iter = identifierMap_.find(key);
    if (iter != identifierMap_.end())
        return iter->second;

    // Ensure capacity in const pool.
    if (constPoolSize_ == constPoolCapacity_)
        expandConstPool();

    Result<VM::String*> str =
        VM::String::Create(acx_, bytes, ident.text(src_));
    if (str.isError())
        emitError("Could not allocate identifier.");

    WH_ASSERT(constPoolSize_ < constPoolCapacity_);
    uint32_t idx = addToConstPool(VM::Box::Pointer(str.value()));
    identifierMap_.insert(IdentifierMap::value_type(key, idx));
    return idx;
}

uint32_t
PackedWriter::addToConstPool(VM::Box thing)
{
    WH_ASSERT(constPoolSize_ < constPoolCapacity_);
    uint32_t idx = constPoolSize_;
    constPool_[idx] = thing;
    constPoolSize_++;
    return idx;
}

void
PackedWriter::expandConstPool()
{
    WH_ASSERT(!hasError());
    if (constPoolCapacity_ == MaxConstPoolSize)
        emitError("Exceeded maximum const pool size size.");

    STLBumpAllocator<VM::Box> alloc(allocator_);
    uint32_t newCapacity = constPoolCapacity_ * 2;
    if (newCapacity == 0)
        newCapacity = InitialConstPoolSize;
    else if (newCapacity > MaxConstPoolSize)
        newCapacity = MaxConstPoolSize;

    VM::Box* newConstPool = alloc.allocate(newCapacity);
    std::copy(constPool_, constPool_ + constPoolSize_, newConstPool);
    if (constPool_)
        alloc.deallocate(constPool_, constPoolCapacity_);
    constPool_ = newConstPool;
    constPoolCapacity_ = newCapacity;
}

void
PackedWriter::writeParenExpr(ParenExprNode const* node)
{
    writeNode(node->subexpr());
}

void
PackedWriter::writeNameExpr(NameExprNode const* node)
{
    uint32_t identIdx = addIdentifier(node->name());
    write(identIdx);
}

void
PackedWriter::parseInteger(IntegerLiteralToken const& token,
                           int32_t* resultOut)
{
    WH_ASSERT(resultOut != nullptr);

    uint8_t const* text = token.text(src_);
    uint32_t len = token.length();
    if (token.hasFlag(Token::Int_BinPrefix)) {
        parseBinInteger(token, resultOut);

    } else if (token.hasFlag(Token::Int_OctPrefix)) {
        parseOctInteger(token, resultOut);

    } else if (token.hasFlag(Token::Int_DecPrefix)) {
        parseDecInteger(token, resultOut);

    } else if (token.hasFlag(Token::Int_HexPrefix)) {
        parseHexInteger(token, resultOut);

    } else {
        int32_t result = 0;
        for (uint32_t i = 0; i < len; i++) {
            unic_t ch = text[i];
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
    }
}

void
PackedWriter::parseBinInteger(IntegerLiteralToken const& token,
                              int32_t* resultOut)
{
    uint8_t const* text = token.text(src_);
    uint32_t len = token.length();
    WH_ASSERT(len > 2);
    WH_ASSERT(text[0] == '0');
    WH_ASSERT(text[1] == 'b');

    int32_t result = 0;
    for (uint32_t i = 2; i < len; i++) {
        unic_t ch = text[i];
        WH_ASSERT((ch == '_') || Tokenizer::IsBinDigit(ch));

        if (ch == '_')
            continue;

        if (result > (std::numeric_limits<int32_t>::max() >> 1))
            emitError("Binary integer literal too large.");

        result <<= 1;
        result |= (ch - static_cast<uint8_t>('0'));
    }
    *resultOut = result;
}

void
PackedWriter::parseOctInteger(IntegerLiteralToken const& token,
                              int32_t* resultOut)
{
    uint8_t const* text = token.text(src_);
    uint32_t len = token.length();
    WH_ASSERT(len > 2);
    WH_ASSERT(text[0] == '0');
    WH_ASSERT(text[1] == 'o');

    int32_t result = 0;
    for (uint32_t i = 2; i < len; i++) {
        uint8_t ch = text[i];
        WH_ASSERT((ch == '_') || Tokenizer::IsOctDigit(ch));

        if (ch == '_')
            continue;

        if (result > (std::numeric_limits<int32_t>::max() >> 3))
            emitError("Octal integer literal too large.");

        result <<= 3;
        result |= (ch - static_cast<uint8_t>('0'));
    }
    *resultOut = result;
}

void
PackedWriter::parseDecInteger(IntegerLiteralToken const& token,
                              int32_t* resultOut)
{
    uint8_t const* text = token.text(src_);
    uint32_t len = token.length();
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
}

void
PackedWriter::parseHexInteger(IntegerLiteralToken const& token,
                              int32_t* resultOut)
{
    uint8_t const* text = token.text(src_);
    uint32_t len = token.length();
    WH_ASSERT(len > 2);
    WH_ASSERT(text[0] == '0');
    WH_ASSERT(text[1] == 'x');

    int32_t result = 0;
    for (uint32_t i = 2; i < len; i++) {
        uint8_t ch = text[i];
        WH_ASSERT((ch == '_') || Tokenizer::IsHexDigit(ch));

        if (ch == '_')
            continue;

        if (result > (std::numeric_limits<int32_t>::max() >> 3))
            emitError("Hexadecimal integer literal too large.");

        int digit = ch - static_cast<uint8_t>('0');
        if (result > ((std::numeric_limits<int32_t>::max() - digit) / 10))

        result <<= 4;
        result |= digit;
    }
    *resultOut = result;
}

void
PackedWriter::writeIntegerExpr(IntegerExprNode const* node)
{
    int32_t val;
    parseInteger(node->token(), &val);
    write(static_cast<uint32_t>(val));
}

void
PackedWriter::writeDotExpr(DotExprNode const* node)
{
    // Write name index, followed by target expression.
    uint32_t identIdx = addIdentifier(node->name());
    write(identIdx);
    writeNode(node->target());
}

void
PackedWriter::writeArrowExpr(ArrowExprNode const* node)
{
    // Write name index, followed by target expression.
    uint32_t identIdx = addIdentifier(node->name());
    write(identIdx);
    writeNode(node->target());
}

void
PackedWriter::writeCallExpr(CallExprNode const* node)
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
    for (Expression const* arg : node->args()) {
        writeOffsetDistance(&argOffsetPos);
        writeNode(arg);
    }
}

void
PackedWriter::writePosExpr(PosExprNode const* node)
{
    writeNode(node->subexpr());
}

void
PackedWriter::writeNegExpr(NegExprNode const* node)
{
    writeNode(node->subexpr());
}

void
PackedWriter::writeBinaryExpr(Expression const* lhs, Expression const* rhs)
{
    Position rhsOffsetPos = position();
    writeDummy();
    writeNode(lhs);
    writeOffsetDistance(&rhsOffsetPos);
    writeNode(rhs);
}

void
PackedWriter::writeMulExpr(MulExprNode const* node)
{
    writeBinaryExpr(node->lhs(), node->rhs());
}

void
PackedWriter::writeDivExpr(DivExprNode const* node)
{
    writeBinaryExpr(node->lhs(), node->rhs());
}

void
PackedWriter::writeAddExpr(AddExprNode const* node)
{
    writeBinaryExpr(node->lhs(), node->rhs());
}

void
PackedWriter::writeSubExpr(SubExprNode const* node)
{
    writeBinaryExpr(node->lhs(), node->rhs());
}

void
PackedWriter::writeEmptyStmt(EmptyStmtNode const* node)
{
}

void
PackedWriter::writeExprStmt(ExprStmtNode const* node)
{
    writeNode(node->expr());
}

void
PackedWriter::writeReturnStmt(ReturnStmtNode const* node)
{
    if (node->hasExpr()) {
        writeTypeExtra(1);
        writeNode(node->expr());
    }
}

void
PackedWriter::writeBlock(Block const* block)
{
    WH_ASSERT(block->statements().size() <= MaxBlockStatements);
    Position offsetPosn = position();
    // First offset is not written.
    for (uint32_t i = 1; i < block->statements().size(); i++)
        writeDummy();

    uint32_t i = 0;
    for (Statement const* stmt : block->statements()) {
        if (i > 0)
            writeOffsetDistance(&offsetPosn);
        writeNode(stmt);
        i++;
    }
}

void
PackedWriter::writeSizedBlock(Block const* block)
{
    if (block->statements().size() > MaxBlockStatements)
        emitError("Too many block statements.");
    write(block->statements().size());
    writeBlock(block);
}

void
PackedWriter::writeIfStmt(IfStmtNode const* node)
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

    for (IfStmtNode::CondPair const& elsifPair : node->elsifPairs()) {
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
PackedWriter::writeDefStmt(DefStmtNode const* node)
{
    uint32_t numParams = node->paramNames().size();
    if (numParams > MaxParams)
        emitError("Too many params.");

    writeTypeExtra(numParams);

    uint32_t nameIdx = addIdentifier(node->name());
    write(nameIdx);

    for (IdentifierToken const& paramName : node->paramNames()) {
        uint32_t paramIdx = addIdentifier(paramName);
        write(paramIdx);
    }

    writeSizedBlock(node->bodyBlock());
}

void
PackedWriter::writeVarStmt(VarStmtNode const* node)
{
    uint32_t numBindings = node->bindings().size();
    if (numBindings > MaxBindings)
        emitError("Too many bindings in var statement.");

    writeTypeExtra(numBindings);

    Position offsetPos = position();
    for (BindingStatement::Binding const& binding : node->bindings()) {
        write(addIdentifier(binding.name()));
        writeDummy();
    }

    for (BindingStatement::Binding const& binding : node->bindings()) {
        offsetPos++;
        if (binding.hasValue()) {
            writeOffsetDistance(&offsetPos);
            writeNode(binding.value());
        } else {
            writeAt(offsetPos++, 0);
        }
    }
}

void
PackedWriter::writeConstStmt(ConstStmtNode const* node)
{
    uint32_t numBindings = node->bindings().size();
    if (numBindings > MaxBindings)
        emitError("Too many bindings in const statement.");

    writeTypeExtra(numBindings);

    Position offsetPos = position();
    for (BindingStatement::Binding const& binding : node->bindings()) {
        write(addIdentifier(binding.name()));
        writeDummy();
    }

    for (BindingStatement::Binding const& binding : node->bindings()) {
        WH_ASSERT(binding.hasValue());
        offsetPos++;
        writeOffsetDistance(&offsetPos);
        writeNode(binding.value());
    }
}

void
PackedWriter::writeLoopStmt(LoopStmtNode const* node)
{
    writeTypeExtra(node->bodyBlock()->statements().size());
    writeBlock(node->bodyBlock());
}

void
PackedWriter::writeFile(FileNode const* node)
{
    uint32_t numStatements = node->statements().size();
    writeTypeExtra(numStatements);

    WH_ASSERT(numStatements <= MaxBlockStatements);
    Position offsetPosn = position();
    for (uint32_t i = 1; i < numStatements; i++)
        writeDummy();

    int32_t i = 0;
    for (Statement const* stmt : node->statements()) {
        if (i > 0)
            writeOffsetDistance(&offsetPosn);
        writeNode(stmt);
        i++;
    }
}


} // namespace AST
} // namespace Whisper
