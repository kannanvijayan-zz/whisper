#ifndef WHISPER__GC__FORMATS_HPP
#define WHISPER__GC__FORMATS_HPP


#define WHISPER_DEFN_GC_STACK_FORMATS(_) \
    _(PropertyName)             \
    _(PropertyDescriptor)       \
    _(Box)                      \
    _(PackedSyntaxElement)      \
    _(PackedWriter)             \
    _(PackedReader)             \
    _(HeapPointer)


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
    _(String)                   \
    _(BoxArray)                 \
    _(RuntimeState)             \
    _(PackedSyntaxTree)         \
    _(SyntaxTreeFragment)       \
    _(HeapPointerArray)         \
    _(SourceFile)               \
    _(PropertyDict)             \
    _(PlainObject)              \
    _(CallObject)               \
    _(ModuleObject)             \
    _(GlobalObject)             \
    _(LookupSeenObjects)        \
    _(LookupNode)               \
    _(LookupState)              \
    _(NativeFunction)           \
    _(ScriptedFunction)         \
    _(FunctionObject)           \
    _(Frame)


#endif // WHISPER__GC__FORMATS_HPP
