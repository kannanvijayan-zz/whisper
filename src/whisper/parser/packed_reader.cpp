
#include <algorithm>
#include "parser/packed_reader.hpp"

namespace Whisper {
namespace AST {


void
PackedReader::visitNode(PackedBaseNode node, PackedVisitor *visitor) const
{
    WH_ASSERT(node.text() >= buffer());
    WH_ASSERT(node.textEnd() == bufferEnd());

    switch (node.type()) {
#define CASE_(ntype) \
      case ntype: \
        visitor->visit##ntype(*this, node.as##ntype()); \
        break;
    WHISPER_DEFN_SYNTAX_NODES(CASE_)
#undef CASE_
      default:
        WH_UNREACHABLE("Unknown reader type.");
    }
}


} // namespace AST
} // namespace Whisper
