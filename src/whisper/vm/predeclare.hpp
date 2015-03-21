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

#define PREDECLARE_BASE_HEAP_TYPE_(name) \
    namespace Whisper { \
      namespace VM { \
        class name; \
      } \
      template <> struct BaseHeapTypeTraits<VM::name> { \
        BaseHeapTypeTraits() = delete; \
        static constexpr bool Specialized = true; \
      }; \
    }

PREDECLARE_HEAP_OBJ_(String, String, true);
PREDECLARE_HEAP_OBJ_(SourceFile, SourceFile, false);
PREDECLARE_HEAP_OBJ_(PackedSyntaxTree, PackedSyntaxTree, false);
PREDECLARE_HEAP_OBJ_(SyntaxTreeFragment, SyntaxTreeFragment, false);
PREDECLARE_HEAP_OBJ_(PropertyDict, PropertyDict, true);
PREDECLARE_HEAP_OBJ_(LookupSeenObjects, LookupSeenObjects, true);
PREDECLARE_HEAP_OBJ_(LookupNode, LookupNode, false);
PREDECLARE_HEAP_OBJ_(LookupState, LookupState, false);

PREDECLARE_HEAP_OBJ_(PlainObject, PlainObject, false);
PREDECLARE_HEAP_OBJ_(CallObject, CallObject, false);
PREDECLARE_HEAP_OBJ_(GlobalObject, GlobalObject, false);

PREDECLARE_HEAP_OBJ_(NativeFunction, NativeFunction, false);

PREDECLARE_BASE_HEAP_TYPE_(Wobject);
PREDECLARE_BASE_HEAP_TYPE_(HashObject);
PREDECLARE_BASE_HEAP_TYPE_(ScopeObject);
PREDECLARE_BASE_HEAP_TYPE_(Function);

#undef PREDECLARE_BASE_HEAP_TYPE_
#undef PREDECLARE_HEAP_OBJ_


#endif // WHISPER__VM__PREDECLARE_HPP
