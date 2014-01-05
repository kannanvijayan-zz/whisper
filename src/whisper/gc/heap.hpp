#ifndef WHISPER__GC__HEAP_HPP
#define WHISPER__GC__HEAP_HPP

#include "common.hpp"
#include "debug.hpp"

namespace Whisper {


class SlabThing;


namespace GC {


//
// HeapTraits<T>()
//
// Specializations of HeapTrait must be provided by every heap-marked
// type.  The specialization must contain the following definitions:
//
//  static constexpr bool SPECIALIZED = true;
//      Marker boolean indicating that HeapTraits has been specialized
//      for this type.
//
//  template <typename Marker> void MARK(Marker &marker, const T &t);
//      Method to scan the heap-marked thing for references.
//      For each heap reference contained within the heap-marked thing,
//      the method |scanner(ptr, addr, discrim)| should be called
//      once, where |ptr| is pointer (of type SlabThing*), |addr|
//      is the address of the pointer (of type void*), and |discrim|
//      is a T-specific uint32_t value that helps distinguish the
//      format of the reference being held.
//
//  void UPDATE(void *addr, uint32_t discrim, SlabThing *newPtr);
//      Method to update a previously-scanned pointer address with
//      a relocated pointer.
//
// Note that if a heap-wrapped type embeds any other heap-types,
// it should define MARK to recursively call MARK on the embedded
// values, using the same marker.
//
template <typename T> struct HeapTraits;


//
// HeapHolder
//
// Helper base class for defining Heap<T> specializations.
//

template <typename T>
class HeapHolder
{
    static_assert(HeapTraits<T>::SPECIALIZED,
                  "Heap traits has not been specialized for type.");

  private:
    T val_;

  public:
    template <typename... Args>
    inline HeapHolder(Args... args)
      : val_(std::forward<Args>(args)...)
    {}

    inline const T &get() const {
        return reinterpret_cast<const T &>(this->val_);
    }
    inline T &get() {
        return reinterpret_cast<T &>(this->val_);
    }

    inline void notifySetPre() {
        // TODO: Do any pre-barriers here.
    }
    inline void notifySetPost(SlabThing *container) {
        // TODO: Set write barrier for container here.
    }

    inline void set(const T &ref, SlabThing *container) {
        notifySetPre();
        this->val_ = ref;
        notifySetPost(container);
    }
    inline void set(T &&ref, SlabThing *container) {
        notifySetPre();
        this->val_ = ref;
        notifySetPost(container);
    }

    template <typename... Args>
    inline void init(SlabThing *container, Args... args) {
        // Pre-notification not required as value is not initialized.
        new (&this->val_) T(std::forward<Args>(args)...);
        notifySetPost();
    }

    inline void destroy(SlabThing *container) {
        notifySetPre();
        val_.~T();
        // Post-notification not required as value is desetroyed.
    }

    inline operator const T &() const {
        return this->get();
    }

    inline const T *operator &() const {
        return &(this->get());
    }

    T &operator =(const HeapHolder<T> &other) = delete;

    template <typename MARKER, typename CONTAINER>
    inline void MARK(MARKER &marker, const CONTAINER *container,
                     uint32_t discrim=0)
    {
        marker(&val_, ToSlabThing(container), discrim);
    }
};



} // namespace GC
} // namespace Whisper

#endif // WHISPER__GC__HEAP_HPP
