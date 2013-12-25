#ifndef WHISPER__GC__STACK_INLINES_HPP
#define WHISPER__GC__STACK_INLINES_HPP

#include "common.hpp"
#include "debug.hpp"
#include "runtime.hpp"

namespace Whisper {
namespace GC {


inline
StackHolderBase::StackHolderBase(ThreadContext *threadContext, StackKind kind)
  : threadContext_(threadContext),
    next_(threadContext_->stackHolders()),
    kind_(kind)
{}


} // namespace GC
} // namespace Whisper

#endif // WHISPER__GC__STACK_INLINES_HPP
