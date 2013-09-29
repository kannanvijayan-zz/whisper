
#include "interp/bytecode_generator.hpp"

#include "spew.hpp"
#include "runtime_inlines.hpp"
#include "rooting_inlines.hpp"
#include "vm/heap_thing_inlines.hpp"

namespace Whisper {
namespace Interp {


BytecodeGenerator::BytecodeGenerator(
        RunContext *cx,
        const STLBumpAllocator<uint8_t> &allocator,
        AST::ProgramNode *node,
        AST::SyntaxAnnotator &annotator,
        bool strict)
  : cx_(cx),
    allocator_(allocator),
    node_(node),
    annotator_(annotator),
    strict_(strict),
    bytecode_(cx_)
{
    WH_ASSERT(node_);
}

bool
BytecodeGenerator::hasError() const
{
    return error_ != nullptr;
}

const char *
BytecodeGenerator::error() const
{
    WH_ASSERT(hasError());
    return error_;
}

VM::Bytecode *
BytecodeGenerator::generateBytecode()
{
    // Scan bytecode initially to calculate info and check for errors.
    try {
        calculateStackDepth_ = true;
        generate();
    } catch (BytecodeGeneratorError &exc) {
        WH_ASSERT(hasError());
        return nullptr;
    }

    SpewBytecodeNote("Got max stack depth: %d", (int) maxStackDepth_);
    SpewBytecodeNote("Final stack depth: %d", (int) currentStackDepth_);

    WH_ASSERT(currentBytecodeSize_ > 0);
    bytecodeSize_ = currentBytecodeSize_;
    currentBytecodeSize_ = 0;

    // Bytecode scanning worked out.  Allocate a bytecode object.
    bytecode_ = cx_->createSized<VM::Bytecode>(true, bytecodeSize_);
    WH_ASSERT(bytecode_);

    // Fill the bytecode object.
    calculateStackDepth_ = false;
    generate();

    return bytecode_;
}

void
BytecodeGenerator::generate()
{
    for (AST::SourceElementNode *elem : node_->sourceElements()) {
        if (elem->isFunctionDeclaration())
            emitError("Cannot handle function declarations yet.");

        // Otherwise, it must be a statement.
        WH_ASSERT(elem->isStatement());
        if (elem->isExpressionStatement()) {
            generateExpressionStatement(elem->toExpressionStatement());
            continue;
        }

        SpewBytecodeError("Cannot handle syntax node: %s", elem->typeString());
        emitError("Cannot handle this syntax node yet.");
    }
}

void
BytecodeGenerator::generateExpressionStatement(
            AST::ExpressionStatementNode *exprStmt)
{
    OperandLocation outputLocation = OperandLocation::StackTop();

    // Generate the expression.
    generateExpression(exprStmt->expression(), outputLocation);

    // Pop the value left on the stack by the expression.
    emitPop();
}

void
BytecodeGenerator::generateExpression(AST::ExpressionNode *expr,
                                      const OperandLocation &outputLocation)
{
    // Handle binary expression.
    if (expr->isBinaryExpression()) {
        AST::BaseBinaryExpressionNode *binExpr = expr->toBinaryExpression();

        // See if LHS and RHS are addressable.
        OperandLocation lhsLocation = OperandLocation::StackTop();
        OperandLocation rhsLocation = OperandLocation::StackTop();

        // If lhs is not directly addressable, generate the code for it
        // and deposit onto stack top.
        if (!getAddressableLocation(binExpr->lhs(), lhsLocation)) {
            WH_ASSERT(lhsLocation.isStackTop());
            generateExpression(binExpr->lhs(), lhsLocation);
        }

        // If rhs is not directly addressable, generate the code for it
        // and deposit onto stack top.
        if (!getAddressableLocation(binExpr->rhs(), rhsLocation)) {
            WH_ASSERT(rhsLocation.isStackTop());
            generateExpression(binExpr->rhs(), rhsLocation);
        }

        // Now generate the binary expression.
        emitBinaryOp(binExpr, lhsLocation, rhsLocation, outputLocation);

    // Handle unary expression.
    } else if (expr->isUnaryExpression()) {
        AST::BaseUnaryExpressionNode *unExpr = expr->toUnaryExpression();

        // See if input is addressable.
        OperandLocation inputLocation = OperandLocation::StackTop();

        // If input is not directly addressable, generate the code for it
        // and deposit onto stack top.
        if (!getAddressableLocation(unExpr->subexpression(), inputLocation)) {
            WH_ASSERT(inputLocation.isStackTop());
            generateExpression(unExpr->subexpression(), inputLocation);
        }

        // Now generate the unary expression.
        emitUnaryOp(unExpr, inputLocation, outputLocation);

    // Handle numeric literals.
    } else if (expr->isNumericLiteral()) {
        AST::NumericLiteralNode *lit = expr->toNumericLiteral();
        WH_ASSERT(lit->hasAnnotation());
        AST::NumericLiteralAnnotation *annot = lit->annotation();

        if (!annot->isInt32()) {
            SpewBytecodeError("Cannot handle non-int NumericLiterals.");
            emitError("Cannot handle expression.");
        }

        // Int32s are just always emitted inline.
        emitPushInt32(annot->int32Value());

    // Handle parenthesized expressions.
    } else if (expr->isParenthesizedExpression()) {
        return generateExpression(
                    expr->toParenthesizedExpression()->subexpression(),
                    outputLocation);
    } else {
        SpewBytecodeError("Cannot handle expr node: %s", expr->typeString());
        emitError("Cannot handle expression");
    }
}

bool
BytecodeGenerator::getAddressableLocation(AST::ExpressionNode *expr,
                                          OperandLocation &location)
{
    // Numeric integer literals within the appropriate range
    // are addressable.
    if (expr->isNumericLiteral()) {
        AST::NumericLiteralNode *lit = expr->toNumericLiteral();
        WH_ASSERT(lit->hasAnnotation());
        AST::NumericLiteralAnnotation *annot = lit->annotation();

        // Nont-int32 immediates cannot be encoded.
        if (!annot->isInt32())
            return false;

        // Ensure the value is within the immediate range.
        int32_t val = annot->int32Value();
        if (val < OperandMinSignedValue || val > OperandMaxSignedValue)
            return false;

        location = OperandLocation::Immediate(annot->int32Value());
        return true;
    }

    // Handle parenthesized expressions.
    if (expr->isParenthesizedExpression()) {
        auto subExpr = expr->toParenthesizedExpression()->subexpression();
        return getAddressableLocation(subExpr, location);
    }

    // Handle negative literals.
    if (expr->isNegativeExpression()) {
        auto subExpr = expr->toNegativeExpression()->subexpression();
        if (!subExpr->isNumericLiteral())
            return false;

        if (!getAddressableLocation(subExpr, location))
            return false;

        if (!location.isImmediate())
            return false;

        if (location.signedValue() < 0)
            return false;

        location = OperandLocation::Immediate(-location.signedValue());
        return true;
    }

    // TODO: Handle other cases.

    return false;
}

void
BytecodeGenerator::emitPushInt32(int32_t value)
{
    if (value >= INT8_MIN && value <= INT8_MAX) {
        emitOp(Opcode::PushInt8);
        emitByte(value);
        return;
    }

    if (value >= INT16_MIN && value <= INT16_MAX) {
        emitOp(Opcode::PushInt16);
        emitByte(value & 0xFF);
        emitByte((value >> 8));
    }

    constexpr int32_t INT24_MAX = 0xFFFFFFl;
    constexpr int32_t INT24_MIN = -INT24_MAX - 1;
    if (value >= INT24_MIN && value <= INT24_MAX) {
        emitOp(Opcode::PushInt24);
        emitByte(value & 0xFF);
        emitByte((value >> 8) & 0xFF);
        emitByte(value >> 16);
    }

    emitOp(Opcode::PushInt32);
    emitByte(value & 0xFF);
    emitByte((value >> 8) & 0xFF);
    emitByte((value >> 16) & 0xFF);
    emitByte(value >> 24);
}

void
BytecodeGenerator::emitUnaryOp(AST::BaseUnaryExpressionNode *expr,
                               const OperandLocation &inputLocation,
                               const OperandLocation &outputLocation)
{
    // Calculate base opcode for this binary operation.
    Opcode opcode = Opcode::INVALID;
    switch (expr->type()) {
      case AST::NegativeExpression:
        opcode = Opcode::Neg_SS;
        break;
      default:
        break;
    }
    if (opcode == Opcode::INVALID)
        emitError("Unhandled unary op.");

    // Calculate offset of actual opcode from the 'SSS' opcode variant.
    uint8_t formatOffset = 0;
    if (!inputLocation.isStackTop())
        formatOffset |= (1 << 1);
    if (!outputLocation.isStackTop())
        formatOffset |= (1 << 0);

    // Calculate actual opcode.
    opcode = static_cast<Opcode>(static_cast<uint16_t>(opcode) + formatOffset);

    // Emit op and operand locations.
    emitOp(opcode);
    emitOperandLocation(inputLocation);
    emitOperandLocation(outputLocation);
}

void
BytecodeGenerator::emitBinaryOp(AST::BaseBinaryExpressionNode *expr,
                                const OperandLocation &lhsLocation,
                                const OperandLocation &rhsLocation,
                                const OperandLocation &outputLocation)
{
    // Calculate base opcode for this binary operation.
    Opcode opcode = Opcode::INVALID;
    switch (expr->type()) {
      case AST::AddExpression:
        opcode = Opcode::Add_SSS;
        break;
      case AST::SubtractExpression:
        opcode = Opcode::Sub_SSS;
        break;
      case AST::MultiplyExpression:
        opcode = Opcode::Mul_SSS;
        break;
      case AST::DivideExpression:
        opcode = Opcode::Div_SSS;
        break;
      case AST::ModuloExpression:
        opcode = Opcode::Mod_SSS;
        break;
      default:
        break;
    }
    if (opcode == Opcode::INVALID)
        emitError("Unhandled binary op.");

    // Calculate offset of actual opcode from the 'SSS' opcode variant.
    uint8_t formatOffset = 0;
    if (!lhsLocation.isStackTop())
        formatOffset |= (1 << 2);
    if (!rhsLocation.isStackTop())
        formatOffset |= (1 << 1);
    if (!outputLocation.isStackTop())
        formatOffset |= (1 << 0);

    // Calculate actual opcode.
    opcode = static_cast<Opcode>(static_cast<uint16_t>(opcode) + formatOffset);

    // Emit op and operand locations.
    emitOp(opcode);
    emitOperandLocation(lhsLocation);
    emitOperandLocation(rhsLocation);
    emitOperandLocation(outputLocation);
}

void
BytecodeGenerator::emitPop(uint16_t num)
{
    for (uint16_t i = 0; i < num; i++)
        emitOp(Opcode::Pop);
}

void
BytecodeGenerator::emitOperandLocation(const OperandLocation &location)
{
    switch (location.space()) {
      case OperandSpace::Constant:
        emitConstantOperand(location.index());
        break;
      case OperandSpace::Argument:
        emitArgumentOperand(location.index());
        break;
      case OperandSpace::Local:
        emitLocalOperand(location.index());
        break;
      case OperandSpace::Stack:
        emitStackOperand(location.index());
        break;
      case OperandSpace::Immediate:
        if (location.isUnsigned())
            emitImmediateUnsignedOperand(location.unsignedValue());
        else
            emitImmediateSignedOperand(location.signedValue());
        break;
      case OperandSpace::StackTop:
        break;
      default:
        WH_UNREACHABLE("Invalid operand space.");
    }
}

void
BytecodeGenerator::emitOp(Opcode op)
{
    WH_ASSERT(IsValidOpcode(op));
    // Ops in section 0 get emitted without prefix.
    // Ops in other sections have section prefix.
    WH_ASSERT(GetOpcodeSection(op) == 0);
    emitByte(static_cast<uint8_t>(op));

    // Adjust stack depth calculations.  Only do this when doing
    // bytecode generation, not scanning.
    if (calculateStackDepth_) {
        WH_ASSERT(GetOpcodePopped(op) <= currentStackDepth_);

        currentStackDepth_ -= GetOpcodePopped(op);
        currentStackDepth_ += GetOpcodePushed(op);
        if (currentStackDepth_ > maxStackDepth_)
            maxStackDepth_ = currentStackDepth_;
    }
}

void
BytecodeGenerator::emitConstantOperand(uint32_t idx)
{
    emitIndexedOperand(OperandSpace::Constant, idx);
}

void
BytecodeGenerator::emitArgumentOperand(uint32_t idx)
{
    emitIndexedOperand(OperandSpace::Argument, idx);
}

void
BytecodeGenerator::emitLocalOperand(uint32_t idx)
{
    emitIndexedOperand(OperandSpace::Local, idx);
}

void
BytecodeGenerator::emitStackOperand(uint32_t idx)
{
    emitIndexedOperand(OperandSpace::Stack, idx);
}

void
BytecodeGenerator::emitImmediateUnsignedOperand(uint32_t val)
{
    if (val <= 0xFu) {
        // Single-byte encoding of operand.
        uint8_t byte = (0 << 2) | 3;
        byte |= val << 4;
        emitByte(byte);
        return;
    }

    if (val <= 0xFFFu) {
        // Two-byte encoding of operand.
        uint8_t byte = (1 << 2) | 3;
        byte |= (val & 0xFu) << 4;
        emitByte(byte);
        emitByte(val >> 4);
        return;
    }

    if (val <= 0xFFFFFu) {
        // Three-byte encoding of operand.
        uint8_t byte = (2 << 2) | 3;
        byte |= (val & 0xFu) << 4;
        emitByte(byte);
        emitByte((val >> 4) & 0xFFu);
        emitByte(val >> 12);
        return;
    }

    WH_ASSERT(val <= 0x0FFFFFFFu);

    // Four-byte encoding of operand.
    uint8_t byte = (3 << 2) | 3;
    byte |= (val & 0xFu) << 4;
    emitByte(byte);
    emitByte((val >> 4) & 0xFFu);
    emitByte((val >> 12) & 0xFFu);
    emitByte(val >> 20);
    return;
}

void
BytecodeGenerator::emitImmediateSignedOperand(int32_t val)
{
    constexpr int32_t Max4 = 0x7l;
    constexpr int32_t Min4 = -Max4 - 1;
    if (val >= Min4 && val <= Max4) {
        // Single-byte encoding of operand.
        uint8_t byte = (0 << 2) | 3;
        byte |= val << 4;
        emitByte(byte);
        return;
    }

    constexpr int32_t Max12 = 0x7FFl;
    constexpr int32_t Min12 = -Max12 - 1;
    if (val >= Min12 && val <= Max12) {
        // Two-byte encoding of operand.
        uint8_t byte = (1 << 2) | 3;
        byte |= (val & 0xFu) << 4;
        emitByte(byte);
        emitByte(val >> 4);
        return;
    }

    constexpr int32_t Max20 = 0x7FFFFl;
    constexpr int32_t Min20 = -Max20 - 1;
    if (val >= Min20 && val <= Max20) {
        // Three-byte encoding of operand.
        uint8_t byte = (2 << 2) | 3;
        byte |= (val & 0xFu) << 4;
        emitByte(byte);
        emitByte((val >> 4) & 0xFFu);
        emitByte(val >> 12);
        return;
    }

    constexpr int32_t Max28 = 0x7FFFFFFl;
    constexpr int32_t Min28 = -Max28 - 1;
    WH_ASSERT(val >= Min28 && val <= Max28);

    // Four-byte encoding of operand.
    uint8_t byte = (3 << 2) | 3;
    byte |= (val & 0xFu) << 4;
    emitByte(byte);
    emitByte((val >> 4) & 0xFFu);
    emitByte((val >> 12) & 0xFFu);
    emitByte(val >> 20);
    return;
}

void
BytecodeGenerator::emitIndexedOperand(OperandSpace space, uint32_t idx)
{
    WH_ASSERT(static_cast<uint8_t>(space) <= 0x3);

    if (idx <= 0xFu) {
        // Single-byte encoding of operand.
        // Low bits hold size (0)
        uint8_t byte = 0;
        byte |= (static_cast<uint8_t>(space) << 2);
        byte |= idx << 4;
        emitByte(byte);
        return;
    }

    if (idx <= 0xFFFu) {
        // Two-byte encoding of operand.
        // Low bits hold size (1)
        uint8_t byte = 1;
        byte |= (static_cast<uint8_t>(space) << 2);
        byte |= (idx & 0xFu) << 4;
        emitByte(byte);
        emitByte(idx >> 4);
        return;
    }

    WH_ASSERT(idx <= 0x0FFFFFFFu);

    // Four-byte encoding of operand.
    // Low bits hold size (2)
    uint8_t byte = 2;
    byte |= (static_cast<uint8_t>(space) << 2);
    byte |= (idx & 0xFu) << 4;
    emitByte(byte);
    emitByte((idx >> 4) & 0xFFu);
    emitByte((idx >> 12) & 0xFFu);
    emitByte(idx >> 20);
    return;
}

void
BytecodeGenerator::emitByte(uint8_t byte)
{
    WH_ASSERT_IF(bytecode_, currentBytecodeSize_ < bytecodeSize_);

    if (bytecode_)
        bytecode_->writableData()[currentBytecodeSize_] = byte;

    currentBytecodeSize_++;
}

void
BytecodeGenerator::emitError(const char *msg)
{
    WH_ASSERT(!hasError());
    error_ = msg;
    throw BytecodeGeneratorError();
}


} // namespace Interp
} // namespace Whisper
