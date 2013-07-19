#ifndef WHISPER__SPEW_HPP
#define WHISPER__SPEW_HPP

#include "common.hpp"

namespace Whisper {


//
// Define various spew channels.
//
#define WHISPER_DEFN_SPEW_CHANNELS(_) \
    _(Debug)        \
    _(Parser)       \
    _(Memory)       \
    _(Slab)

enum class SpewChannel
{
    INVALID = 0,
#define ENUM_(n)    n,
    WHISPER_DEFN_SPEW_CHANNELS(ENUM_)
#undef ENUM_
    LIMIT
};

enum class SpewLevel
{
    Note,
    Warn,
    Error,
    None
};


#if defined(ENABLE_SPEW)

void InitializeSpew();
void Spew(SpewChannel chan, SpewLevel level, const char *fmt, ...);

#define DEFSPEW_(n) \
    template <typename... Args> \
    void Spew##n##Note(const char *fmt, Args... args) {\
        Spew(SpewChannel::n, SpewLevel::Note, fmt, args...); \
    } \
    template <typename... Args> \
    void Spew##n##Warn(const char *fmt, Args... args) {\
        Spew(SpewChannel::n, SpewLevel::Warn, fmt, args...); \
    } \
    template <typename... Args> \
    void Spew##n##Error(const char *fmt, Args... args) {\
        Spew(SpewChannel::n, SpewLevel::Error, fmt, args...); \
    }
    WHISPER_DEFN_SPEW_CHANNELS(DEFSPEW_)
#undef DEFSPEW_


#else // !defined(ENABLE_SPEW)

inline void InitializeSpew() {}

#define SpewDebugNote(...)
#define SpewDebugWarn(...)
#define SpewDebugError(...)

#define SpewParserNote(...)
#define SpewParserWarn(...)
#define SpewParserError(...)

#define SpewMemoryNote(...)
#define SpewMemoryWarn(...)
#define SpewMemoryError(...)

#define SpewSlabNote(...)
#define SpewSlabWarn(...)
#define SpewSlabError(...)


#endif // defined(ENABLE_SPEW)


} // namespace Whisper

#endif // WHISPER__SPEW_HPP
