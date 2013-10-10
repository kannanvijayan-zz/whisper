
#include "vm/bytecode.hpp"
#include "vm/heap_thing_inlines.hpp"
#include "interp/bytecode_ops.hpp"
#include "spew.hpp"

#include <algorithm>
#include <stdio.h>

namespace Whisper {
namespace VM {


Bytecode::Bytecode()
{}

const uint8_t *
Bytecode::data() const
{
    return recastThis<uint8_t>();
}

const uint8_t *
Bytecode::dataAt(uint32_t pcOffset) const
{
    WH_ASSERT(pcOffset < length());
    return data() + pcOffset;
}

const uint8_t *
Bytecode::dataEnd() const
{
    return data() + length();
}

uint8_t *
Bytecode::writableData()
{
    return recastThis<uint8_t>();
}

uint32_t
Bytecode::length() const
{
    return objectSize();
}

void
SpewBytecodeObject(Bytecode *bc)
{
    if (ChannelSpewLevel(SpewChannel::Bytecode) > SpewLevel::Note)
        return;

    const uint8_t *data = bc->data();
    uint32_t length = bc->length();
    const uint8_t *end = data + length;

    constexpr uint32_t BufSize = 1024;
    char buf[BufSize];

    // Deparse and spew each bytecode.
    const uint8_t *cur = data;

    SpewBytecodeNote("Object %p", bc);
    SpewBytecodeNote("  Data %p-%p, len=%d", data, end, (int)length);
    while (cur < end) {
        // Parse the opcode.
        Interp::Opcode op;
        uint32_t instrBytes = Interp::ReadOpcode(cur, end, &op);
        Interp::OpcodeFormat fmt = Interp::GetOpcodeFormat(op);

        // Write opcode string into buffer.
        uint32_t bufIdx = 0;
        bufIdx += snprintf(buf, BufSize, "%s", Interp::GetOpcodeName(op));

        // Get operand count.
        uint8_t operandCount = Interp::GetOpcodeOperandCount(fmt);
        for (uint8_t i = 0; i < operandCount; i++) {
            Interp::OperandLocation location;
            uint32_t operandBytes = Interp::ReadOperandLocation(
                                cur + instrBytes, end, fmt, i, &location);
            WH_ASSERT(operandBytes > 0);
            instrBytes += operandBytes;

            const char *space = Interp::OperandSpaceString(location.space());
            int32_t val = 0;
            if (location.isImmediate()) {
                if (location.isSigned())
                    val = location.signedValue();
                else
                    val = location.unsignedValue();
            } else {
                val = location.anyIndex();
            }

            bufIdx += snprintf(buf + bufIdx, BufSize - bufIdx, "%s[%s %d]",
                               (i == 0) ? " " : ", ", space, (int) val);
            WH_ASSERT(bufIdx < BufSize);
        }

        SpewBytecodeNote("    %s", buf);

        // Go to next instruction.
        cur += instrBytes;
    }
}


} // namespace VM
} // namespace Whisper
