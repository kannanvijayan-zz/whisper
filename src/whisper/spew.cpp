
#include <iostream>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>

#include "spew.hpp"
#include "debug.hpp"

namespace Whisper {


#if defined(ENABLE_SPEW)

static bool SPEW_INITIALIZED = false;
static const unsigned SPEW_NUM_CHANNELS = static_cast<int>(SpewChannel::LIMIT);
static SpewLevel SPEW_LEVELS[SPEW_NUM_CHANNELS];

struct LevelInfo {
    const char *str;
    int len;
    SpewLevel level;
} LEVEL_INFO[] = {
    { "note", 4, SpewLevel::Note },
    { "warn", 4, SpewLevel::Warn },
    { "error", 5, SpewLevel::Error },
    { "none", 4, SpewLevel::None },
    { nullptr, 0, SpewLevel::None }
};

static SpewLevel
ParseSpewLevel(const char *s)
{
    for (int i = 0; LEVEL_INFO[i].str != nullptr; i++) {
        LevelInfo &inf = LEVEL_INFO[i];
        if (!strcmp(inf.str, s) && (s[inf.len] == '\0' || s[inf.len] == ','))
            return inf.level;
    }
    return SpewLevel::Warn;
}

static bool
CheckChannelName(const char *s, const char *name, SpewChannel chan)
{
    int i = 0;
    bool found = false;
    bool gotEquals = false;
    for (i = 0;; i++) {
        if (name[i] == '\0') {
            if (s[i] == '\0' || s[i] == ',' || s[i] == '=') {
                found = true;
                gotEquals = (s[i] == '=');
            }
            break;
        }

        if (tolower(name[i]) != tolower(s[i]))
            break;
    }

    if (!found)
        return false;

    SpewLevel level = SpewLevel::Warn;
    // Parse channel level if needed.
    if (gotEquals)
        level = ParseSpewLevel(&s[i+1]);

    SPEW_LEVELS[static_cast<int>(chan)] = level;
    return true;
}

static void
CheckSpewChannel(const char *envstr, const char *channelName, SpewChannel chan)
{
    const char *cur = envstr;
    while (cur[0] == ',')
        cur++;

    for (;;) {
        // Check for end of string.
        if (cur[0] == '\0')
            break;

        // Check for channel name.
        if (CheckChannelName(cur, channelName, chan))
            return;

        // Skip to next ',' or '\0'
        while (cur[0] != '\0') {
            if (cur[0] == ',') {
                cur++;
                break;
            }
            cur++;
        }
    }
}

void
InitializeSpew()
{
    WH_ASSERT(!SPEW_INITIALIZED);

    for (unsigned i = 0; i < SPEW_NUM_CHANNELS; i++)
        SPEW_LEVELS[i] = SpewLevel::Warn;

    // Get environment variable WHSPEW
    const char *whspew = getenv("WHSPEW");

    if (whspew) {

        // Check spew each channel.
#define CHECK_(n)    CheckSpewChannel(whspew, #n, SpewChannel::n);
        WHISPER_DEFN_SPEW_CHANNELS(CHECK_)
#undef CHECK_

    }

    SPEW_INITIALIZED = 1;
}


static const char *
SpewChannelString(SpewChannel chan)
{
    switch (chan) {
      case SpewChannel::INVALID:
        return "INVALID";
#define CASE_(n)    case SpewChannel::n: return #n;
    WHISPER_DEFN_SPEW_CHANNELS(CASE_)
#undef CASE_
      default:
        return "UNKNOWN";
    }
}

void
Spew(SpewChannel chan, SpewLevel level, const char *fmt, ...)
{
    WH_ASSERT(SPEW_INITIALIZED);
    WH_ASSERT(chan > SpewChannel::INVALID);
    WH_ASSERT(chan < SpewChannel::LIMIT);
    if (SPEW_LEVELS[static_cast<int>(chan)] > level)
        return;

    const char *levelName = nullptr;
    if (level == SpewLevel::Note)
        levelName = "NOTE";
    else if (level == SpewLevel::Warn)
        levelName = "WARN";
    else if (level == SpewLevel::Error)
        levelName = "ERROR";

    char buf[1000];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, 1000, fmt, args);
    va_end(args);

    fprintf(stderr, "[%s] %s: %s\n", levelName, SpewChannelString(chan), buf);
}


#endif // defined(ENABLE_SPEW)


} // namespace Whisper
