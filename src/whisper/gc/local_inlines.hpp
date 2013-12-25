#ifndef WHISPER__GC__LOCAL_INLINES_HPP
#define WHISPER__GC__LOCAL_INLINES_HPP

#include "common.hpp"
#include "debug.hpp"
#include "runtime.hpp"

namespace Whisper {
namespace GC {


inline
LocalHolderBase::LocalHolderBase(ThreadContext *threadContext, LocalKind kind)
  : threadContext_(threadContext),
    next_(threadContext_->localHolders()),
    kind_(kind)
{}


} // namespace GC
} // namespace Whisper

#endif // WHISPER__GC__LOCAL_INLINES_HPP
