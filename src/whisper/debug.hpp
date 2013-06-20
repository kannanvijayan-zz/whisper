#ifndef WHISPER__DEBUG_HPP
#define WHISPER__DEBUG_HPP

#include "common.hpp"

namespace Whisper {


#if defined(ENABLE_DEBUG)

#define WH_ASSERT_IF(pre, cond) \
    do { \
        if (pre) \
            ::Whisper::Assert(__FILE__, __LINE__, #pre " => " #cond, cond); \
    } while(false)
#define WH_ASSERT(cond) \
    ::Whisper::Assert(__FILE__, __LINE__, #cond, cond)
void Assert(const char *file, int line, const char *condstr, bool cond);

#define WH_UNREACHABLE(msg) \
    ::Whisper::AssertUnreachable(__FILE__, __LINE__, msg)
void AssertUnreachable(const char *file, int line, const char *msg);

template <typename T>
class DebugVal
{
  private:
    T val_;

  public:
    inline DebugVal() : val_() {}
    inline DebugVal(const T &val) : val_(val) {}

    inline operator const T &() const {
        return val_;
    }
};


#else // !defined(ENABLE_DEBUG)

#define WH_ASSERT_IF(pre, cond)
#define WH_ASSERT(cond)
#define WH_UNREACHABLE(msg)

template <typename T>
class DebugVal
{
  public:
    inline DebugVal() {}
    inline DebugVal(const T &val) {}
};


#endif // defined(ENABLE_DEBUG)


} // namespace Whisper

#endif // WHISPER__DEBUG_HPP
