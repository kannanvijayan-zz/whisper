#ifndef WHISPER__GC__FORMATS_HPP
#define WHISPER__GC__FORMATS_HPP


#define WHISPER_DEFN_GC_STACK_FORMATS(_) \
    _(UntracedPrimitive)        \
    _(NativeCallInfo)           \
    _(SyntaxTreeRef)            \
    _(SyntaxNodeRef)            \
    _(SyntaxBlockRef)           \
    _(PropertyName)             \
    _(PropertyDescriptor)       \
    _(Box)                      \
    _(ValBox)                   \
    _(PackedSyntaxElement)      \
    _(PackedWriter)             \
    _(PackedReader)             \
    _(HeapPointer)              \
    _(EvalResult)               \
    _(CallResult)               \
    _(StepResult)


#define WHISPER_DEFN_GC_HEAP_FORMATS(_) \
    _(UInt8Array)               \
    _(UInt16Array)              \
    _(UInt32Array)              \
    _(UInt64Array)              \
    _(Int8Array)                \
    _(Int16Array)               \
    _(Int32Array)               \
    _(Int64Array)               \
    _(FloatArray)               \
    _(DoubleArray)              \
    _(HeapPointerArray)         \
    _(String)                   \
    _(BoxArray)                 \
    _(ValBoxArray)              \
    _(UInt8Slist)               \
    _(UInt16Slist)              \
    _(UInt32Slist)              \
    _(UInt64Slist)              \
    _(Int8Slist)                \
    _(Int16Slist)               \
    _(Int32Slist)               \
    _(Int64Slist)               \
    _(FloatSlist)               \
    _(DoubleSlist)              \
    _(HeapPointerSlist)         \
    _(BoxSlist)                 \
    _(ValBoxSlist)              \
    _(Error)                    \
    _(RuntimeState)             \
    _(ThreadState)              \
    _(PackedSyntaxTree)         \
    _(SyntaxNode)               \
    _(SyntaxBlock)              \
    _(SourceFile)               \
    _(PropertyDict)             \
    _(PlainObject)              \
    _(CallScope)                \
    _(BlockScope)               \
    _(ModuleScope)              \
    _(GlobalScope)              \
    _(LookupSeenObjects)        \
    _(LookupNode)               \
    _(LookupState)              \
    _(NativeFunction)           \
    _(ScriptedFunction)         \
    _(FunctionObject)           \
    _(TerminalFrame)            \
    _(EntryFrame)               \
    _(SyntaxNameLookupFrame)    \
    _(InvokeSyntaxFrame)        \
    _(FileSyntaxFrame)          \
    _(CallExprSyntaxFrame)      \
    _(InvokeApplicativeFrame)   \
    _(InvokeOperativeFrame)


#endif // WHISPER__GC__FORMATS_HPP
