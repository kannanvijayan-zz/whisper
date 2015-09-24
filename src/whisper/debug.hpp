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
void Assert(char const* file, int line, char const* condstr, bool cond);

#define WH_UNREACHABLE(msg) \
    ::Whisper::AssertUnreachable(__FILE__, __LINE__, msg)
void AssertUnreachable(char const* file, int line, char const* msg);

template <typename T>
class DebugVal
{
  private:
    T val_;

  public:
    DebugVal() : val_() {}
    DebugVal(T const& val) : val_(val) {}

    operator T const&() const {
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
    DebugVal() {}
    DebugVal(T const& val) {}
};


#endif // defined(ENABLE_DEBUG)


} // namespace Whisper

#endif // WHISPER__DEBUG_HPP
