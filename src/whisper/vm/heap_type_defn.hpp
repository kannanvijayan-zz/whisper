#ifndef WHISPER__VM__HEAP_TYPE_DEFN_HPP
#define WHISPER__VM__HEAP_TYPE_DEFN_HPP


// Macro iterating over all heap types.
#define WHISPER_DEFN_HEAP_TYPES(_)                              \
    /* Name                             Traced */               \
    \
    _(HeapDouble,                       false)                  \
    _(HeapString,                       false)                  \
    \
    _(Object,                           true)                   \
    _(ObjectSlots,                      true)                   \
    \
    _(Class,                            true)


#endif // WHISPER__VM__HEAP_TYPE_DEFN_HPP
