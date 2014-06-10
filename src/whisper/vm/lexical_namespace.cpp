
#include "vm/lexical_namespace.hpp"

namespace Whisper {
namespace VM {


LexicalNamespace::LexicalNamespace(
        LexicalNamespace *parent,
        BindingArray *bindings)
  : parent_(parent), bindings_(bindings)
{
    WH_ASSERT(bindings != nullptr);
}


} // namespace VM
} // namespace Whisper
