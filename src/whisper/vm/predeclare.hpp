#ifndef WHISPER__VM__PREDECLARE_HPP
#define WHISPER__VM__PREDECLARE_HPP

#include "vm/core.hpp"

#define PREDECLARE_HEAP_OBJ_(name, fmtName, isVarSized) \
    namespace Whisper { \
      namespace VM { \
        class name; \
      } \
      template <> struct HeapTraits<VM::name> { \
        HeapTraits() = delete; \
        static constexpr bool Specialized = true; \
        static constexpr HeapFormat Format = HeapFormat::fmtName; \
        static constexpr bool VarSized = isVarSized; \
      }; \
    }

PREDECLARE_HEAP_OBJ_(LookupSeenObjects, LookupSeenObjects, true);
PREDECLARE_HEAP_OBJ_(LookupNode, LookupNode, false);
PREDECLARE_HEAP_OBJ_(LookupState, LookupState, false);
PREDECLARE_HEAP_OBJ_(PackedSyntaxTree, PackedSyntaxTree, false);
PREDECLARE_HEAP_OBJ_(SyntaxTreeFragment, SyntaxTreeFragment, false);
PREDECLARE_HEAP_OBJ_(PlainObject, PlainObject, false);
PREDECLARE_HEAP_OBJ_(PropertyDict, PropertyDict, true);
PREDECLARE_HEAP_OBJ_(SourceFile, SourceFile, false);
PREDECLARE_HEAP_OBJ_(String, String, true);

#undef PREDECLARE_HEAP_OBJ_

namespace Whisper {
namespace VM {

class Wobject;
class HashObject;

} // namespace VM

template <>
struct BaseHeapTypeTraits<VM::Wobject>
{
    BaseHeapTypeTraits() = delete;
    static constexpr bool Specialized = true;
};
template <>
struct BaseHeapTypeTraits<VM::HashObject>
{
    BaseHeapTypeTraits() = delete;
    static constexpr bool Specialized = true;
};

} // namespace Whisper


#endif // WHISPER__VM__PREDECLARE_HPP
