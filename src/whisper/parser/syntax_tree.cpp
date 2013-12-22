
#include "parser/syntax_tree.hpp"
#include "parser/syntax_tree_inlines.hpp"

namespace Whisper {
namespace AST {


const char *
NodeTypeString(NodeType nodeType)
{
    switch (nodeType) {
      case INVALID:
        return "INVALID";
    #define DEF_CASE_(node)   case node: return #node;
      WHISPER_DEFN_SYNTAX_NODES(DEF_CASE_)
    #undef DEF_CASE_
      case LIMIT:
        return "LIMIT";
      default:
        return "UNKNOWN";
    }
}


} // namespace AST
} // namespace Whisper
