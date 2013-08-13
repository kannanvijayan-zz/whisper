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
    \
    _(Object,                           true)                   \
    _(Global,                           true)                   \
    \
    _(DeclEnv,                          true)                   \
    _(ObjEnv,                           true)                   \
    _(FuncEnv,                          true)                   \
    _(GlobalEnv,                        true)                   \
    \
    _(ConstantPool,                     true)                   \


// Listing of all PropertyMap types.
#define WHISPER_DEFN_PROPMAP_TYPES(_)                           \
    /* Name */                                                  \
    _(Object)


#endif // WHISPER__VM__HEAP_TYPE_DEFN_HPP
