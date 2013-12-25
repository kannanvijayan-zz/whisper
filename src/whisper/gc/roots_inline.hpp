#ifndef WHISPER__GC_ROOTS_HPP
#define WHISPER__GC_ROOTS_HPP

#include "common.hpp"
#include "debug.hpp"
#include "runtime.hpp"

namespace Whisper {
namespace GC {


inline
RootHolderBase::RootHolderBase(ThreadContext *threadContext, RootKind kind)
  : threadContext_(threadContext),
    next_(threadContext_->roots()),
    kind_(kind)
{}


} // namespace GC
} // namespace Whisper

#endif // WHISPER__GC_ROOTS_HPP
