#ifndef WHISPER__ALLOCATORS_HPP
#define WHISPER__ALLOCATORS_HPP

#include <new>

#include "common.hpp"
#include "debug.hpp"
#include "helpers.hpp"
#include "memalloc.hpp"

//
// Custom C++ STL allocators.
//

namespace Whisper {


//
// WrapAllocator
//
// Allocates memory using an indirect pointer to another allocator.
// This is for situations where the destruction of the original
// allocator performs cleanup, and thus the original allocator cannot be
// copied.
//
template <typename T, template <typename X> class A>
class WrapAllocator
{
  public:
    typedef typename A<T>::size_type           size_type;
    typedef typename A<T>::difference_type     difference_type;
    typedef typename A<T>::pointer             pointer;
    typedef typename A<T>::const_pointer       const_pointer;
    typedef typename A<T>::reference           reference;
    typedef typename A<T>::const_reference     const_reference;
    typedef typename A<T>::value_type          value_type;
    template <class U> struct rebind {
        typedef WrapAllocator<U, A> other;
    };

  private:
    A<T> &wrapped_;

  public:
    inline WrapAllocator(A<T> &wrapped) throw()
      : wrapped_(wrapped) {}

    inline WrapAllocator(const WrapAllocator &other) throw()
      : wrapped_(other.wrapped_) {}

    template <class U>
    inline WrapAllocator(const WrapAllocator<U, A> &other) throw()
      : wrapped_(A<U>(other.wrapped_))
    {}

    inline ~WrapAllocator() {}

    inline pointer address(reference x) const {
        return A<T>::address(x);
    }
    inline const_pointer address(const_reference x) const {
        return A<T>::address(x);
    }

    inline pointer allocate(size_type sz,
                            typename A<void>::const_pointer hint = nullptr)
    {
        return wrapped_.allocate(sz, hint);
    }

    inline void deallocate(pointer p, size_type n) {
        return wrapped_.deallocate(p, n);
    }

    inline size_type max_size() const throw() {
        return wrapped_.max_size();
    }

    inline void construct(pointer p, const value_type &val) {
        wrapped_.construct(p, val);
    }
    inline void destroy(pointer p) {
        wrapped_.destroy(p);
    }
};

//
// BumpAllocator
//
// Allocates memory by first allocating larger arenas, within which
// bump allocation is used to allocate smaller chunks.
//
// The allocator keeps a linked list (chain) of memory chunks that
// are used for successive allocations.  The linked list is embedded
// within the chain.
//
// Within a chunk, allocation happens from high-to-low, similar to stack
// allocation.
//
template <typename T> class BumpAllocator;

template <>
class BumpAllocator<void>
{
  public:
    typedef void *          pointer;
    typedef const void *    const_pointer;
    typedef void            value_type;
    template <class U> struct rebind {
        typedef BumpAllocator<U> other;
    };
};

template <typename T>
class BumpAllocator
{
  public:
    typedef size_t              size_type;
    typedef ptrdiff_t           difference_type;
    typedef T *                 pointer;
    typedef const T *           const_pointer;
    typedef T &                 reference;
    typedef const T &           const_reference;
    typedef T                   value_type;
    template <class U> struct rebind {
        typedef BumpAllocator<U> other;
    };

    class Error {
        Error() {}
    };

  private:
    struct Chain {
        void *addr;
        size_t size;
        Chain *prev;
        Chain(void *addr, size_t size, Chain *prev)
          : addr(addr), size(size), prev(prev) {}
    };

    constexpr static unsigned BasicAlignment = sizeof(word_t);
    constexpr static unsigned MinOverhead = BasicAlignment*2 + sizeof(Chain);

    // Default chunk size for new chunk allocations.
    constexpr static size_t DefaultChunkSize = 4096;
    size_t chunkSize_;

    // Pointer to the top of the chain.
    Chain *chainEnd_;

    // Allocation grows down (top to bottom).
    uint8_t *allocBottom_;
    uint8_t *allocTop_;

  public:
    BumpAllocator() : BumpAllocator(DefaultChunkSize) {}
    BumpAllocator(size_t chunkSize)
      : chunkSize_(chunkSize),
        chainEnd_(nullptr),
        allocTop_(nullptr),
        allocBottom_(nullptr)
    {
        WH_ASSERT(chunkSize >= 256);

        // Push a new default-sized chunk for allocation.
        pushNewChunk(chunkSize_);
    }

    inline pointer address(reference x) const {
        return &x;
    }
    inline const_pointer address(const_reference x) const {
        return &x;
    }

    inline pointer allocate(size_type sz, void *hint = nullptr)
    {
        // ignore hint, just allocate sz bytes, aligned to BasicAlignment
        uint8_t *result = AlignPtrDown(allocTop_ - sz, BasicAlignment);
        if (result >= allocBottom_ && result <= allocTop_)
            return static_cast<pointer>(result);

        // Otherwise, allocate new chunk.  First, determine if
        // new chunk should be standard size of custom oversized.
        size_t newChunkSize = chunkSize_;
        if (sz > (chunkSize_ - MinOverhead))
            newChunkSize = sz + MinOverhead;

        // Allocate new chunk and use it.  If chunk is pushed successfully,
        // then it is guaranteed to be able to satisfy the allocation.
        pushNewChunk(newChunkSize);
        result = AlignPtrDown(allocTop_ - sz, BasicAlignment);
        WH_ASSERT(result >= allocBottom_ && result <= allocTop_);
        return static_cast<pointer>(result);
    }

    inline void deallocate(pointer p, size_type n) {
        // Deallocation is a no-op.
        return;
    }

    inline size_type max_size() const throw() {
        return SIZE_MAX;
    }

    inline void construct(pointer p, const value_type &val) {
        new (p) value_type(val);
    }
    inline void destroy(pointer p) {
        p->~value_type();
    }

  private:
    void pushNewChunk(size_t size) {
        void *mem = AllocateMemory(size);
        if (!mem)
            throw Error();

        uint8_t *memu8 = static_cast<uint8_t *>(mem);

        // Align the memory up to the alignment reqired by Chain.
        uint8_t *chainAddr = AlignPtrUp(memu8, alignof(Chain));
        chainEnd_ = new (chainAddr) Chain(memu8, size, chainEnd_);

        // Set up allocTop and allocBottom
        allocBottom_ = chainAddr + sizeof(Chain);
        allocTop_ = memu8 + size;
    }

    void popChunk() {
        WH_ASSERT(chainEnd_);
        Chain *chainPrev = chainEnd_->prev;
        ReleaseMemory(chainEnd_->addr);
        chainEnd_ = chainPrev;
    }

    void releaseChunks() {
        while (chainEnd_)
            popChunk();
    }
};


} // namespace Whisper

#endif // WHISPER__ALLOCATORS_HPP
