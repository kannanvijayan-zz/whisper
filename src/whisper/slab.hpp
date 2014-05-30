#ifndef WHISPER__SLAB_HPP
#define WHISPER__SLAB_HPP

#include "common.hpp"
#include "helpers.hpp"
#include "debug.hpp"
#include "gc/core.hpp"

namespace Whisper {


//
// Slabs
// =====
//
// Slabs are used to allocate garbage-collected heap objects.
//
// A slab's layout is as follows:
//
//
//  /-> +-----------------------+   <--- Top - aligned to 1k
//  |   | Forward/Next          |   }
//  |   |                       |   }-- Header (multiple of 1k)
//  |   |                       |   }
//  |   +-----------------------+
//  \---|-- |     |   Traced    |   }
//      |---/     |   Objects   |   }
//      |         v             |   }
//      |~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~|   }
//      |                       |   }
//      |    Free Space         |   }-- Data space (multiple of 1k cards)
//      |                       |   }
//      |                       |   }
//      |~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~|   }
//      |         ^  NonTraced  |   }
//      |         |  Objects    |   }
//      |         |             |   }
//      +-----------------------+
//
// Slabs come in two basic forms: standard slab, which are of a fixed
// size and allocate multiple small objects, and singleton slabs,
// which vary in size, and hold a single large object.
//
// Singleton slabs are not necessarily larger than standard slabs.
// They are simply used for objects which are larger than a certain
// threshold size.  This reduces memory that would be wasted by
// allocating large objects within standard slabs.
//
// There's a maximum possible size for standard slabs implied by
// the size of the CardNo field in an object's header.  That field
// needs to be able to describe the card number it's allocated on.
// Since objects within standard chunks can exist on any card, the
// maximum card within such a chunk is limited to maximum CardNo
// describable by the object.
//
// Singleton chunks do not suffer this problem as only a single object
// is allocated in them, and thus the start of the object allocated on it
// will be in the first card.
//
// Additionally, The first ptr-sized word of the allocation area stores a
// pointer back to the slab structure.  If the allocated object stores the
// cardNo it starts on, then a pointer to the object can be mapped back to
// the Slab as follows:
//
//      Slab *PointerToSlab(AllocThing *ptr):
//          // Get cardNo
//          unsigned cardNo = ptr->cardNo
//
//          // Get card-aligned pointer
//          Card *card = AlignPtrDown((Card *) ptr, CARD_SIZE)
//
//          // Go to card 0
//          char *card0 = ((char *) card) - (cardNo * CARD_SIZE)
//
//          // Read slab pointer.
//          return *((Slab **) card0)
//
// This is a relatively efficient operation, in machine code it is:
//      1 memory read (cardNo)
//      1 bit-and (ptr & ~(CARD_SIZE - 1))
//      1 shift (cardNo * CARD_SIZE)
//      1 subtract (card - (cardNo * CARD_SIZE))
//      1 memory read (*card0)
//
//  Working out to two memory reads and three fast int ops.
//

class Slab
{
  friend class SlabList;
  public:
    static constexpr uint32_t AllocAlign = sizeof(uint64_t);
    static constexpr uint32_t CardSizeLog2 = 10;
    static constexpr uint32_t CardSize = 1 << CardSizeLog2;
    static constexpr uint32_t AlienRefSpaceSize = 512;

    static uint32_t PageSize();

    static uint32_t StandardSlabCards();
    static uint32_t StandardSlabHeaderCards();
    static uint32_t StandardSlabDataCards();
    static uint32_t StandardSlabMaxObjectSize();

    // Calculate the number of data cards required to store an object
    // of a particular size.
    static uint32_t NumDataCardsForObjectSize(uint32_t objectSize);

    // Calculate the number of header cards required in a chunk with the
    // given number of data cards.
    static uint32_t NumHeaderCardsForDataCards(uint32_t dataCards);

    // Allocate/destroy slabs.
    static Slab *AllocateStandard(GC::Gen gen);
    static Slab *AllocateSingleton(uint32_t objectSize, GC::Gen gen);
    static void Destroy(Slab *slab);

  private:
    // Pointer to the actual system-allocated memory region containing
    // the slab.
    void *region_;
    uint32_t regionSize_;

    // Next/previous slab pointers.
    Slab *next_ = nullptr;
    Slab *previous_ = nullptr;

    // Pointer to top and bottom of allocation space
    uint8_t *allocTop_ = nullptr;
    uint8_t *allocBottom_ = nullptr;

    // Pointer to head and tail allocation pointers.
    uint8_t *headAlloc_ = nullptr;
    uint8_t *tailAlloc_ = nullptr;

    // Number of header cards.
    uint32_t headerCards_;

    // Number of data cards.
    uint32_t dataCards_;

    // Slab generation.
    GC::Gen gen_;

    Slab(void *region, uint32_t regionSize,
         uint32_t headerCards, uint32_t dataCards,
         GC::Gen gen);

    ~Slab() {}

  public:
    uint8_t *headStartAlloc() const {
        WH_ASSERT(allocTop_);
        return allocTop_ + AlignIntUp<uint32_t>(sizeof(void *), AllocAlign);
    }

    uint8_t *tailStartAlloc() const {
        WH_ASSERT(allocTop_);
        return allocBottom_;
    }

    Slab *next() const {
        return next_;
    }
    Slab *previous() const {
        return previous_;
    }

    uint32_t headerCards() const {
        return headerCards_;
    }

    uint32_t dataCards() const {
        return dataCards_;
    }

    GC::Gen gen() const {
        return gen_;
    }

    uint8_t *headEndAlloc() const {
        return headAlloc_;
    }
    uint8_t *tailEndAlloc() const {
        return tailAlloc_;
    }

    // Allocate memory from Top
    uint8_t *allocateHead(uint32_t amount) {
        WH_ASSERT(IsIntAligned(amount, AllocAlign));

        uint8_t *oldTop = headAlloc_;
        uint8_t *newTop = oldTop + amount;
        if (newTop > allocBottom_)
            return nullptr;

        headAlloc_ = newTop;
        return oldTop;
    }

    // Allocate memory from Bottom
    uint8_t *allocateTail(uint32_t amount) {
        WH_ASSERT(IsIntAligned(amount, AllocAlign));

        uint8_t *newBot = tailAlloc_ - amount;
        if (newBot < allocTop_)
            return nullptr;

        tailAlloc_ = newBot;
        return newBot;
    }

    uint32_t calculateCardNumber(uint8_t *ptr) const {
        WH_ASSERT(ptr >= allocTop_ && ptr < allocBottom_);
        WH_ASSERT(ptr < headAlloc_ || ptr >= tailAlloc_);
        uint32_t diff = ptr - allocTop_;
        return diff >> CardSizeLog2;
    }
};


//
// SlabList
//
// A list implementation that uses the embedded next/forward pointers in slabs.
//
class SlabList
{
  private:
    uint32_t numSlabs_;
    Slab *firstSlab_;
    Slab *lastSlab_;

  public:
    SlabList()
      : numSlabs_(0),
        firstSlab_(nullptr),
        lastSlab_(nullptr)
    {}

    uint32_t numSlabs() const {
        return numSlabs_;
    }

    void addSlab(Slab *slab) {
        WH_ASSERT(slab->next_ == nullptr);
        WH_ASSERT(slab->previous_ == nullptr);

        if (numSlabs_ == 0) {
            lastSlab_ = firstSlab_ = slab;
        } else {
            slab->previous_ = lastSlab_;
            lastSlab_->next_ = slab;
            lastSlab_ = slab;
        }
        numSlabs_++;
    }

    class Iterator
    {
      friend class SlabList;
      private:
        const SlabList &list_;
        Slab *slab_;

        Iterator(const SlabList &list, Slab *slab)
          : list_(list), slab_(slab)
        {}

      public:
        Slab *operator *() const {
            WH_ASSERT(slab_);
            return slab_;
        }

        bool operator ==(const Iterator &other) const {
            return slab_ == other.slab_;
        }

        bool operator !=(const Iterator &other) const {
            return slab_ != other.slab_;
        }

        Iterator &operator ++() {
            slab_ = slab_->next();
            return *this;
        }
        Iterator &operator --() {
            WH_ASSERT(slab_ != list_.firstSlab_);
            slab_ = slab_ ? slab_->previous() : list_.lastSlab_;
            return *this;
        }
    };

    Iterator begin() const {
        return Iterator(*this, firstSlab_);
    }
    Iterator end() const {
        return Iterator(*this, lastSlab_);
    }
};


} // namespace Whisper

#endif // WHISPER__SLAB_HPP
