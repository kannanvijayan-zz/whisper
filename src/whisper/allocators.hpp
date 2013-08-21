#ifndef WHISPER__ALLOCATORS_HPP
#define WHISPER__ALLOCATORS_HPP

#include <new>
#include <algorithm>

#include "common.hpp"
#include "debug.hpp"
#include "helpers.hpp"
#include "memalloc.hpp"

//
// Custom C++ STL allocators.
//

namespace Whisper {


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
// This allocator is not a proper STL allocator.  See STLAllocator
// below.
//
class BumpAllocatorError {
  public:
    BumpAllocatorError() {}
};

class BumpAllocator
{
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
        allocBottom_(nullptr),
        allocTop_(nullptr)
    {
        WH_ASSERT(chunkSize >= 256);

        // Push a new default-sized chunk for allocation.
        pushNewChunk(chunkSize_);
    }

    void *allocate(size_t sz, unsigned align)
    {
        align = Max(align, BasicAlignment);

        // ignore hint, just allocate sz bytes, aligned to BasicAlignment
        uint8_t *result = AlignPtrDown(allocTop_ - sz, align);
        if (result >= allocBottom_ && result <= allocTop_) {
            allocTop_ = result;
            return result;
        }

        // Otherwise, allocate new chunk.  First, determine if
        // new chunk should be standard size or custom oversized.
        size_t newChunkSize = chunkSize_;
        if (sz > (chunkSize_ - MinOverhead))
            newChunkSize = sz + MinOverhead;

        // Allocate new chunk and use it.  If chunk is pushed successfully,
        // then it is guaranteed to be able to satisfy the allocation.
        pushNewChunk(newChunkSize);
        result = AlignPtrDown(allocTop_ - sz, BasicAlignment);
        WH_ASSERT(result >= allocBottom_ && result <= allocTop_);
        allocTop_ = result;
        return result;
    }

  private:
    void pushNewChunk(size_t size) {
        void *mem = AllocateMemory(size);
        if (!mem)
            throw BumpAllocatorError();

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

//
// STLBumpAllocator
//
// STL allocator which wraps BumpAllocator.
//
template <typename T> class STLBumpAllocator;

template <>
class STLBumpAllocator<void>
{
  public:
    typedef void *          pointer;
    typedef const void *    const_pointer;
    typedef void            value_type;
    template <class U> struct rebind {
        typedef STLBumpAllocator<U> other;
    };
};

template <typename T>
class STLBumpAllocator
{
    template <typename U>
    friend class STLBumpAllocator;

  public:
    typedef size_t              size_type;
    typedef ptrdiff_t           difference_type;
    typedef T *                 pointer;
    typedef const T *           const_pointer;
    typedef T &                 reference;
    typedef const T &           const_reference;
    typedef T                   value_type;
    template <class U> struct rebind {
        typedef STLBumpAllocator<U> other;
    };

  private:
    BumpAllocator &base_;

  public:
    STLBumpAllocator(BumpAllocator &base) : base_(base) {}

    STLBumpAllocator(const STLBumpAllocator &other) throw ()
      : base_(other.base_) {}

    template <typename U>
    STLBumpAllocator(const STLBumpAllocator<U> &other) throw ()
      : base_(other.base_) {}

    pointer address(reference x) const {
        return &x;
    }
    const_pointer address(const_reference x) const {
        return &x;
    }

    pointer allocate(size_type n, void *hint = nullptr)
    {
        return static_cast<pointer>(base_.allocate(n * sizeof(T), alignof(T)));
    }

    void deallocate(pointer p, size_type n) {
        // Deallocation is a no-op.
    }

    size_type max_size() const throw() {
        return SIZE_MAX;
    }

    void construct(pointer p, const value_type &val) {
        new (p) value_type(val);
    }
    void destroy(pointer p) {
        p->~value_type();
    }
};


//
// PoolAllocator
//
// Uses an underlying BumpAllocator, but also keeps a free-list of
// allocations of particular sizes.  Also, this allocator can only
// be used to allocate with alignment equal to "BasicAlignment".
//

class PoolAllocatorError {
  public:
    PoolAllocatorError() {}
};

class PoolAllocator
{
  public:
    constexpr static unsigned BasicAlignment = sizeof(word_t);
    constexpr static unsigned NumFreeLists = 16;

  private:
    struct FreeList {
        FreeList *next;
        FreeList(FreeList *next) : next(next) {}
    };

    BumpAllocator &bumpAllocator_;
    FreeList **freeLists_;

  public:
    PoolAllocator(BumpAllocator &bumpAllocator)
      : bumpAllocator_(bumpAllocator),
        freeLists_(nullptr)
    {
        size_t arraySize = NumFreeLists * sizeof(FreeList *);
        void *arrayPtr = bumpAllocator_.allocate(arraySize, BasicAlignment);
        freeLists_ = static_cast<FreeList **>(arrayPtr);

        for (unsigned i = 0; i < NumFreeLists; i++)
            freeLists_[i] = nullptr;
    }

    void *allocate(size_t sz)
    {
        // Check the free list.
        unsigned idx = sz / BasicAlignment;
        if (idx < NumFreeLists && freeLists_[idx] != nullptr) {
            FreeList *area = freeLists_[idx];
            freeLists_[idx] = area->next;
#if defined(ENABLE_DEBUG)
            uint8_t *ptr = reinterpret_cast<uint8_t *>(area);
            std::fill(ptr, ptr + sz, 0);
#endif
            return area;
        }

        // Otherwise, just allocate.
        return bumpAllocator_.allocate(sz, BasicAlignment);
    }

    void deallocate(void *ptr, size_t sz) {
        // See if we can add it to the free list.
        unsigned idx = sz / BasicAlignment;
        if (idx < NumFreeLists) {
#if defined(ENABLE_DEBUG)
            uint8_t *u8ptr = reinterpret_cast<uint8_t *>(ptr);
            std::fill(u8ptr, u8ptr + sz, 0);
#endif
            freeLists_[idx] = new (ptr) FreeList(freeLists_[idx]);
        }
    }
};


//
// STLPoolAllocator wraps PoolAllocator to present an STL allocator
// interface.
//

template <typename T> class STLPoolAllocator;

template <>
class STLPoolAllocator<void>
{
  public:
    typedef void *          pointer;
    typedef const void *    const_pointer;
    typedef void            value_type;
    template <class U> struct rebind {
        typedef STLPoolAllocator<U> other;
    };
};

template <typename T>
class STLPoolAllocator
{
    template <typename U>
    friend class STLPoolAllocator;

  public:
    typedef size_t              size_type;
    typedef ptrdiff_t           difference_type;
    typedef T *                 pointer;
    typedef const T *           const_pointer;
    typedef T &                 reference;
    typedef const T &           const_reference;
    typedef T                   value_type;
    template <class U> struct rebind {
        typedef STLPoolAllocator<U> other;
    };

  private:
    PoolAllocator &base_;

  public:
    STLPoolAllocator(PoolAllocator &base) : base_(base) {}

    STLPoolAllocator(const STLPoolAllocator &other) throw ()
      : base_(other.base_) {}

    template <typename U>
    STLPoolAllocator(const STLPoolAllocator<U> &other) throw ()
      : base_(other.base_) {}

    pointer address(reference x) const {
        return &x;
    }
    const_pointer address(const_reference x) const {
        return &x;
    }

    pointer allocate(size_type n, void *hint = nullptr)
    {
        WH_ASSERT(alignof(T) <= PoolAllocator::BasicAlignment);
        return static_cast<pointer>(base_.allocate(n * sizeof(T)));
    }

    void deallocate(pointer p, size_type n) {
        base_.deallocate(p, n * sizeof(T));
    }

    size_type max_size() const throw() {
        return SIZE_MAX;
    }

    void construct(pointer p, const value_type &val) {
        new (p) value_type(val);
    }
    void destroy(pointer p) {
        p->~value_type();
    }
};


} // namespace Whisper

#endif // WHISPER__ALLOCATORS_HPP
