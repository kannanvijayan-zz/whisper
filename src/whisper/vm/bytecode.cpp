
#include "vm/bytecode.hpp"
#include "vm/heap_thing_inlines.hpp"

#include <algorithm>

namespace Whisper {
namespace VM {


Bytecode::Bytecode()
{}

const uint8_t *
Bytecode::data() const
{
    return recastThis<uint8_t>();
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


} // namespace VM
} // namespace Whisper
