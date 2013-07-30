#ifndef WHISPER__VM__HEAP_TYPE_DEFN_HPP
#define WHISPER__VM__HEAP_TYPE_DEFN_HPP


// Macro iterating over all heap types.
#define WHISPER_DEFN_HEAP_TYPES(_)                              \
    /* Name                             Traced */               \
    \
    _(HeapDouble,                       false)                  \
    _(HeapString,                       false)                  \
    \
    _(PropertyDescriptor,               true)                   \
    \
    _(Tuple,                            true)                   \
    \
    _(ShapeTree,                        true)                   \
    _(ShapeTreeChild,                   true)                   \
    _(Shape,                            true)                   \
    \
    _(DeclEnv,                          true)                   \
    _(ObjEnv,                           true)                   \
    _(FuncEnv,                          true)                   \
    _(GlobalEnv,                        true)                   \
    \
    _(Script,                           true)                   \
    _(ConstantPool,                     true)                   \
    _(Bytecode,                         false)                  \
    \
    _(Object,                           true)                   \
    _(ObjectSlots,                      true)                   \


#endif // WHISPER__VM__HEAP_TYPE_DEFN_HPP
