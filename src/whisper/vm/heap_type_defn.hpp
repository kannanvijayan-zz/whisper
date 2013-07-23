#ifndef WHISPER__VM__HEAP_TYPE_DEFN_HPP
#define WHISPER__VM__HEAP_TYPE_DEFN_HPP


// Macro iterating over all heap types.
#define WHISPER_DEFN_HEAP_TYPES(_)                              \
    /* Name                             Traced */               \
    \
    _(HeapDouble,                       false)                  \
    _(HeapString,                       false)                  \
    \
    _(CompletionRecord,                 true)                   \
    _(Reference,                        true)                   \
    _(PropertyDescriptor,               true)                   \
    \
    _(Script,                           true)                   \
    _(ConstantPool,                     true)                   \
    _(Bytecode,                         false)                  \
    \
    _(Object,                           true)                   \
    _(ObjectSlots,                      true)                   \
    _(ObjectType,                       true)                   \
    _(ObjectShape,                      true)


#endif // WHISPER__VM__HEAP_TYPE_DEFN_HPP
