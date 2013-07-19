#ifndef WHISPER__SPEW_HPP
#define WHISPER__SPEW_HPP

#include "common.hpp"

namespace Whisper {


//
// Define various spew channels.
//
#define WHISPER_DEFN_SPEW_CHANNELS(_) \
    _(Debug)        \
    _(Parser)

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

template <typename... Args>
void SpewDebugNote(const char *fmt, Args... args) {
    Spew(SpewChannel::Debug, SpewLevel::Note, fmt, args...);
}
template <typename... Args>
void SpewDebugWarn(const char *fmt, Args... args) {
    Spew(SpewChannel::Debug, SpewLevel::Warn, fmt, args...);
}
template <typename... Args>
void SpewDebugError(const char *fmt, Args... args) {
    Spew(SpewChannel::Debug, SpewLevel::Error, fmt, args...);
}


template <typename... Args>
void SpewParserNote(const char *fmt, Args... args) {
    Spew(SpewChannel::Parser, SpewLevel::Note, fmt, args...);
}
template <typename... Args>
void SpewParserWarn(const char *fmt, Args... args) {
    Spew(SpewChannel::Parser, SpewLevel::Warn, fmt, args...);
}
template <typename... Args>
void SpewParserError(const char *fmt, Args... args) {
    Spew(SpewChannel::Parser, SpewLevel::Error, fmt, args...);
}



#else // !defined(ENABLE_SPEW)

inline void InitializeSpew() {}

#define SpewDebugNote(...)
#define SpewDebugWarn(...)
#define SpewDebugError(...)

#define SpewParserNote(...)
#define SpewParserWarn(...)
#define SpewParserError(...)


#endif // defined(ENABLE_SPEW)


} // namespace Whisper

#endif // WHISPER__SPEW_HPP
