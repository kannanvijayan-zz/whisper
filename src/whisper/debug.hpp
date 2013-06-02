#ifndef WHISPER__DEBUG_HPP
#define WHISPER__DEBUG_HPP

namespace Whisper {


#if defined(ENABLE_DEBUG)

#define WH_ASSERT(cond) ::Whisper::Assert(__FILE__, __LINE__, #cond, cond)
void Assert(const char *file, int line, const char *condstr, bool cond);


#else // !defined(ENABLE_DEBUG)

#define WH_ASSERT(cond)


#endif // defined(ENABLE_DEBUG)


} // namespace Whisper

#endif // WHISPER__DEBUG_HPP
