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
LocalBase::LocalBase(ThreadContext* threadContext,
                     StackFormat format,
                     uint32_t count,
                     uint32_t size)
  : threadContext_(threadContext),
    next_(threadContext_->locals()),
    header_(format, size, count)
{
    threadContext_->locals_ = this;
}

inline
LocalBase::LocalBase(AllocationContext const& acx,
                     StackFormat format,
                     uint32_t count,
                     uint32_t size)
  : LocalBase(acx.threadContext(), format, count, size)
{}

inline 
LocalBase::~LocalBase()
{
    WH_ASSERT(threadContext_->locals_ == this);
    threadContext_->locals_ = next_;
}

template <typename Scanner>
void
LocalBase::scan(Scanner& scanner, void* start, void* end) const
{
    for (uint32_t i = 0; i < count(); i++) {
        GC::ScanStackThing(format(), stackThing(i), scanner, start, end);
    }
}

template <typename Updater>
void
LocalBase::update(Updater& updater, void* start, void* end)
{
    for (uint32_t i = 0; i < count(); i++) {
        GC::UpdateStackThing(format(), stackThing(i), updater, start, end);
    }
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

template <typename T>
inline MutHandle<T>
LocalArrayBase<T, UINT32_MAX>::mutHandle(uint32_t idx)
{
    return MutHandle<T>(*address(idx));
}

template <typename T>
inline Handle<T>
LocalArrayBase<T, UINT32_MAX>::handle(uint32_t idx) const
{
    return Handle<T>(*address(idx));
}


} // namespace Whisper


#endif // WHISPER__GC__LOCAL_INLINES_HPP
