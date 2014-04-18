#ifndef WHISPER__GC__LOCAL_INLINES_HPP
#define WHISPER__GC__LOCAL_INLINES_HPP

#include "common.hpp"
#include "debug.hpp"
#include "runtime.hpp"
#include "gc/specializations.hpp"

namespace Whisper {


inline
LocalBase::LocalBase(ThreadContext *threadContext, AllocFormat format)
  : threadContext_(threadContext),
    next_(threadContext_->locals()),
    header_(format, GCGen::OnStack, 0, 0)
{
    threadContext_->locals_ = this;
}

inline 
LocalBase::~LocalBase()
{
    WH_ASSERT(threadContext_->locals_ == this);
    threadContext_->locals_ = next_;
}

template <typename Scanner>
void
LocalBase::SCAN(Scanner &scanner, void *start, void *end) const
{
    GcScanAllocFormat(format(), dataAfter(), scanner, start, end);
}

template <typename Updater>
void
LocalBase::UPDATE(Updater &updater, void *start, void *end)
{
    GcUpdateAllocFormat(format(), dataAfter(), updater, start, end);
}


} // namespace Whisper

#endif // WHISPER__GC__LOCAL_INLINES_HPP
