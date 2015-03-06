
#include <algorithm>
#include "parser/packed_reader.hpp"

namespace Whisper {
namespace AST {


void
PackedReader::visitAt(const uint32_t *position, PackedVisitor *visitor) const
{
    WH_ASSERT(position >= buffer());
    WH_ASSERT(position < bufferEnd());

    // First word contains type.
    uint32_t typeAndExtra = *position;
    NodeType type = static_cast<NodeType>(typeAndExtra & 0xfff);
    uint32_t extra = typeAndExtra >> 12;

    switch (type) {
#define CASE_(ntype) \
      case ntype: \
        visitor->visit##ntype(*this, position, extra); \
        break;
    WHISPER_DEFN_SYNTAX_NODES(CASE_)
#undef CASE_
      default:
        WH_UNREACHABLE("Unknown reader type.");
    }
}


} // namespace AST
} // namespace Whisper
