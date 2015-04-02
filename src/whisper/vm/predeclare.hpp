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
      template <> struct HeapFormatTraits<HeapFormat::fmtName> { \
        HeapFormatTraits() = delete; \
        typedef VM::name Type; \
      }; \
    }

#define PREDECLARE_FIXSIZED_HEAP_OBJ_(name) \
    PREDECLARE_HEAP_OBJ_(name, name, false)

#define PREDECLARE_VARSIZED_HEAP_OBJ_(name) \
    PREDECLARE_HEAP_OBJ_(name, name, true)

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

PREDECLARE_VARSIZED_HEAP_OBJ_(String);
PREDECLARE_FIXSIZED_HEAP_OBJ_(SourceFile);
PREDECLARE_FIXSIZED_HEAP_OBJ_(PackedSyntaxTree);
PREDECLARE_FIXSIZED_HEAP_OBJ_(SyntaxTreeFragment);
PREDECLARE_FIXSIZED_HEAP_OBJ_(RuntimeState);
PREDECLARE_VARSIZED_HEAP_OBJ_(PropertyDict);
PREDECLARE_VARSIZED_HEAP_OBJ_(LookupSeenObjects);
PREDECLARE_FIXSIZED_HEAP_OBJ_(LookupNode);
PREDECLARE_FIXSIZED_HEAP_OBJ_(LookupState);

PREDECLARE_BASE_HEAP_TYPE_(Wobject);
PREDECLARE_BASE_HEAP_TYPE_(HashObject);
PREDECLARE_BASE_HEAP_TYPE_(ScopeObject);
PREDECLARE_FIXSIZED_HEAP_OBJ_(PlainObject);
PREDECLARE_FIXSIZED_HEAP_OBJ_(CallObject);
PREDECLARE_FIXSIZED_HEAP_OBJ_(GlobalObject);

PREDECLARE_BASE_HEAP_TYPE_(Function);
PREDECLARE_FIXSIZED_HEAP_OBJ_(NativeFunction);
PREDECLARE_FIXSIZED_HEAP_OBJ_(ScriptedFunction);
PREDECLARE_FIXSIZED_HEAP_OBJ_(FunctionObject);

PREDECLARE_VARSIZED_HEAP_OBJ_(Frame);

#undef PREDECLARE_BASE_HEAP_TYPE_
#undef PREDECLARE_FIXSIZED_HEAP_OBJ_
#undef PREDECLARE_VARSIZED_HEAP_OBJ_
#undef PREDECLARE_HEAP_OBJ_


#endif // WHISPER__VM__PREDECLARE_HPP
