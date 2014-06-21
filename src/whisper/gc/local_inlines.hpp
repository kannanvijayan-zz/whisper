#ifndef WHISPER__GC__LOCAL_INLINES_HPP
#define WHISPER__GC__LOCAL_INLINES_HPP

#include "common.hpp"
#include "debug.hpp"
#include "runtime.hpp"
#include "gc/local.hpp"
#include "gc/specializations.hpp"

namespace Whisper {
namespace GC {


inline
LocalBase::LocalBase(ThreadContext *threadContext, AllocFormat format)
  : threadContext_(threadContext),
    next_(threadContext_->locals()),
    header_(format, GC::Gen::OnStack, 0, 0)
{
    threadContext_->locals_ = this;
}

inline
LocalBase::LocalBase(RunContext *runContext, AllocFormat format)
  : LocalBase(runContext->threadContext(), format)
{}

inline
LocalBase::LocalBase(AllocationContext &acx, AllocFormat format)
  : LocalBase(acx.threadContext(), format)
{}

inline 
LocalBase::~LocalBase()
{
    WH_ASSERT(threadContext_->locals_ == this);
    threadContext_->locals_ = next_;
}

template <typename Scanner>
void
LocalBase::Scan(Scanner &scanner, void *start, void *end) const
{
    GcScanAllocFormat(format(), dataAfter(), scanner, start, end);
}

template <typename Updater>
void
LocalBase::Update(Updater &updater, void *start, void *end)
{
    GcUpdateAllocFormat(format(), dataAfter(), updater, start, end);
}


} // namespace GC
} // namespace Whisper

#endif // WHISPER__GC__LOCAL_INLINES_HPP
