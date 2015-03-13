#ifndef WHISPER__GC__LOCAL_INLINES_HPP
#define WHISPER__GC__LOCAL_INLINES_HPP

#include "common.hpp"
#include "debug.hpp"
#include "runtime.hpp"
#include "gc/local.hpp"
#include "gc/handle.hpp"
#include "gc/specializations.hpp"

namespace Whisper {


inline
LocalBase::LocalBase(ThreadContext *threadContext,
                     StackFormat format, uint32_t size)
  : threadContext_(threadContext),
    next_(threadContext_->locals()),
    header_(format, size)
{
    threadContext_->locals_ = this;
}

inline
LocalBase::LocalBase(RunContext *runContext,
                     StackFormat format, uint32_t size)
  : LocalBase(runContext->threadContext(), format, size)
{}

inline
LocalBase::LocalBase(const AllocationContext &acx,
                     StackFormat format, uint32_t size)
  : LocalBase(acx.threadContext(), format, size)
{}

inline 
LocalBase::~LocalBase()
{
    WH_ASSERT(threadContext_->locals_ == this);
    threadContext_->locals_ = next_;
}

template <typename Scanner>
void
LocalBase::scan(Scanner &scanner, void *start, void *end) const
{
    GC::ScanStackThing(format(), stackThing(), scanner, start, end);
}

template <typename Updater>
void
LocalBase::update(Updater &updater, void *start, void *end)
{
    GC::UpdateStackThing(format(), stackThing(), updater, start, end);
}


template <typename T>
inline MutHandle<T>
Local<T>::mutHandle()
{
    return MutHandle<T>(*this);
}

template <typename T>
inline Handle<T>
Local<T>::handle() const
{
    return Handle<T>(*this);
}


} // namespace Whisper


#endif // WHISPER__GC__LOCAL_INLINES_HPP
