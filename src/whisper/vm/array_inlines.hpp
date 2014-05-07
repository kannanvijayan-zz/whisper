#ifndef WHISPER__VM__ARRAY_INLINES_HPP
#define WHISPER__VM__ARRAY_INLINES_HPP

namespace Whisper {
namespace VM {


template <typename T>
inline uint32_t
Array<T>::length() const
{
    WH_ASSERT(GC::AllocThing::From(this)->size() % sizeof(T) == 0);
    return GC::AllocThing::From(this)->size() / sizeof(T);
}

template <typename T>
inline const T &
Array<T>::getRaw(uint32_t idx) const
{
    WH_ASSERT(idx < length());
    return vals_[idx];
}

template <typename T>
inline T &
Array<T>::getRaw(uint32_t idx)
{
    WH_ASSERT(idx < length());
    return vals_[idx];
}

template <typename T>
inline T
Array<T>::get(uint32_t idx) const
{
    return vals_[idx];
}

template <typename T>
inline void
Array<T>::set(uint32_t idx, const T &val)
{
    WH_ASSERT(idx < length());
    vals_[idx].set(val, this);
}


} // namespace VM
} // namespace Whisper

#endif // WHISPER__VM__ARRAY_INLINES_HPP
