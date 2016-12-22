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

#define PREDECLARE_STACK_OBJ_FMT_(ns, name, fmtName) \
    namespace Whisper { \
      namespace ns { \
        class name; \
      } \
      template <> struct StackTraits<ns::name> { \
        StackTraits() = delete; \
        static constexpr bool Specialized = true; \
        static constexpr StackFormat Format = StackFormat::fmtName; \
      }; \
      template <> struct StackFormatTraits<StackFormat::fmtName> { \
        StackFormatTraits() = delete; \
        typedef ns::name Type; \
      }; \
    }

#define PREDECLARE_STACK_OBJ_(name) \
    PREDECLARE_STACK_OBJ_FMT_(VM, name, name)

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

PREDECLARE_STACK_OBJ_(Box);
PREDECLARE_STACK_OBJ_(ValBox);
PREDECLARE_STACK_OBJ_(EvalResult);
PREDECLARE_STACK_OBJ_(CallResult);
PREDECLARE_STACK_OBJ_(StepResult);

PREDECLARE_VARSIZED_HEAP_OBJ_(BaseSelfTraced);

PREDECLARE_VARSIZED_HEAP_OBJ_(String);
PREDECLARE_FIXSIZED_HEAP_OBJ_(SourceFile);

PREDECLARE_FIXSIZED_HEAP_OBJ_(Error);

PREDECLARE_FIXSIZED_HEAP_OBJ_(PackedSyntaxTree);
PREDECLARE_BASE_HEAP_TYPE_(SyntaxTreeFragment);
PREDECLARE_FIXSIZED_HEAP_OBJ_(SyntaxNode);
PREDECLARE_FIXSIZED_HEAP_OBJ_(SyntaxBlock);
PREDECLARE_STACK_OBJ_(SyntaxTreeRef);
PREDECLARE_STACK_OBJ_(SyntaxNodeRef);
PREDECLARE_STACK_OBJ_(SyntaxBlockRef);

PREDECLARE_STACK_OBJ_(PropertyName);
PREDECLARE_STACK_OBJ_(PropertyDescriptor);
PREDECLARE_STACK_OBJ_(PropertyLookupResult);

PREDECLARE_FIXSIZED_HEAP_OBJ_(RuntimeState);
PREDECLARE_FIXSIZED_HEAP_OBJ_(ThreadState);
PREDECLARE_VARSIZED_HEAP_OBJ_(PropertyDict);
PREDECLARE_VARSIZED_HEAP_OBJ_(LookupSeenObjects);
PREDECLARE_FIXSIZED_HEAP_OBJ_(LookupNode);
PREDECLARE_FIXSIZED_HEAP_OBJ_(LookupState);

PREDECLARE_BASE_HEAP_TYPE_(Exception);
PREDECLARE_VARSIZED_HEAP_OBJ_(InternalException);
PREDECLARE_FIXSIZED_HEAP_OBJ_(NameLookupFailedException);
PREDECLARE_FIXSIZED_HEAP_OBJ_(FunctionNotOperativeException);

PREDECLARE_BASE_HEAP_TYPE_(Wobject);
PREDECLARE_BASE_HEAP_TYPE_(HashObject);
PREDECLARE_FIXSIZED_HEAP_OBJ_(PlainObject);

PREDECLARE_BASE_HEAP_TYPE_(ScopeObject);
PREDECLARE_FIXSIZED_HEAP_OBJ_(CallScope);
PREDECLARE_FIXSIZED_HEAP_OBJ_(BlockScope);
PREDECLARE_FIXSIZED_HEAP_OBJ_(ModuleScope);
PREDECLARE_FIXSIZED_HEAP_OBJ_(GlobalScope);

PREDECLARE_BASE_HEAP_TYPE_(Function);
PREDECLARE_FIXSIZED_HEAP_OBJ_(NativeFunction);
PREDECLARE_FIXSIZED_HEAP_OBJ_(ScriptedFunction);
PREDECLARE_FIXSIZED_HEAP_OBJ_(FunctionObject);
PREDECLARE_STACK_OBJ_(NativeCallInfo);

PREDECLARE_BASE_HEAP_TYPE_(Frame);
PREDECLARE_FIXSIZED_HEAP_OBJ_(TerminalFrame);
PREDECLARE_FIXSIZED_HEAP_OBJ_(EntryFrame);
PREDECLARE_BASE_HEAP_TYPE_(SyntaxFrame);
PREDECLARE_FIXSIZED_HEAP_OBJ_(InvokeSyntaxNodeFrame);
PREDECLARE_FIXSIZED_HEAP_OBJ_(FileSyntaxFrame);
PREDECLARE_FIXSIZED_HEAP_OBJ_(BlockSyntaxFrame);
PREDECLARE_FIXSIZED_HEAP_OBJ_(CallExprSyntaxFrame);
PREDECLARE_FIXSIZED_HEAP_OBJ_(InvokeApplicativeFrame);
PREDECLARE_FIXSIZED_HEAP_OBJ_(InvokeOperativeFrame);
PREDECLARE_FIXSIZED_HEAP_OBJ_(NativeCallResumeFrame);

#undef PREDECLARE_BASE_HEAP_TYPE_
#undef PREDECLARE_FIXSIZED_HEAP_OBJ_
#undef PREDECLARE_VARSIZED_HEAP_OBJ_
#undef PREDECLARE_STACK_OBJ_
#undef PREDECLARE_STACK_OBJ_FMT_
#undef PREDECLARE_HEAP_OBJ_


#endif // WHISPER__VM__PREDECLARE_HPP
