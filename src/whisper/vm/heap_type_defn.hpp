#ifndef WHISPER__VM__HEAP_TYPE_DEFN_HPP
#define WHISPER__VM__HEAP_TYPE_DEFN_HPP


// Macro iterating over all heap types.
#define WHISPER_DEFN_HEAP_TYPES(_)                              \
    /* Name                             Traced */               \
    \
    _(HeapDouble,                       false)                  \
    _(HeapString,                       false)                  \
    _(Bytecode,                         false)                  \
    \
    _(Tuple,                            true)                   \
    \
    _(ShapeTree,                        true)                   \
    _(ShapeTreeChild,                   true)                   \
    _(Shape,                            true)                   \
    \
    _(Script,                           true)                   \
    _(StackFrame,                       true)                   \
    \
    _(ObjectScopeDescriptor,            true)                   \
    _(ObjectScope,                      true)                   \
    _(DeclarativeScopeDescriptor,       true)                   \
    _(DeclarativeScope,                 true)                   \
    _(GlobalScope,                      true)                   \
    \
    _(Object,                           true)                   \
    _(Global,                           true)                   \
    \
    _(ConstantPool,                     true)                   \


// Listing of minimum
#define WHISPER_DEFN_PROPMAP_TYPES(_)                           \
    /* Name */                                                  \
    _(Object)                                                   \
    _(Global)


#endif // WHISPER__VM__HEAP_TYPE_DEFN_HPP
