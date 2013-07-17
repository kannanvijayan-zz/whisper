#ifndef WHISPER__VM__HEAP_TYPE_DEFN_HPP
#define WHISPER__VM__HEAP_TYPE_DEFN_HPP


// Macro iterating over all heap types.
#define WHISPER_DEFN_HEAP_TYPES(_) \
    /* Heap Doubles */             \
    _(Double)                      \
    \
    /* Heap Strings */             \
    _(String)                      \
    \
    /* Regular objects */          \
    _(Object)                      \
    _(ObjectSlots)                 \
    \
    /* Classes */                  \
    _(Class)


#endif // WHISPER__VM__HEAP_TYPE_DEFN_HPP
