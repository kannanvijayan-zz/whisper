#ifndef WHISPER__SLAB_HPP
#define WHISPER__SLAB_HPP

#include "common.hpp"
#include "helpers.hpp"
#include "debug.hpp"

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
// size and allocate multiple "small" objects, and singleton slabs,
// which are of variable sized, and hold a single "large" object.
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
// NOTE: The first 8 bytes of the allocation area are a pointer to the
// slab structure.
//

class Slab
{
  friend class SlabList;
  public:
    static constexpr uint32_t AllocAlign = sizeof(void *);
    static constexpr uint32_t CardSizeLog2 = 10;
    static constexpr uint32_t CardSize = 1 << CardSizeLog2;
    static constexpr uint32_t AlienRefSpaceSize = 512;

    enum Generation : uint8_t
    {
        // Hatchery is where new objects are created.  It contains
        // only standard sized slabs and has a fixed maximum size.
        Hatchery,

        // Nursery is where recently created slabs are stored until
        // a subsequent cull.  The nursery never "grows".  Whenever
        // the hatchery becomes full, the nursery (if it contains
        // slabs) is garbage-collected and cleared, and then the hatchery
        // pages moved directly into it.
        Nursery,

        // Tenured generation is the oldest generation of objects.
        Tenured
    };

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
    static Slab *AllocateStandard(Generation gen);
    static Slab *AllocateSingleton(uint32_t objectSize, Generation gen);
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
    Generation gen_;

    Slab(void *region, uint32_t regionSize,
         uint32_t headerCards, uint32_t dataCards,
         Generation gen);

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

    Generation gen() const {
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
// A list implementation that uses the next/forward pointers in slabs.
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

//
// SlabAllocType
//
// Objects that are allocated within slabs must each be described
// with a specific SlabAllocType that allows the runtime to
// dispatch various operations on it, such as tracing for garbage
// collection.
//
#define WHISPER_DEFN_SLAB_ALLOC_TYPES(_) \
    _(Array)                \
    _(Vector)               \
    _(VectorContents)       \
    _(HashTable)            \
    _(HashTableContents)    \
    _(Module)               \
    _(Type)

enum class SlabAllocType : uint32_t
{
    INVALID = 0,
#define ENUM_(name) name,
    WHISPER_DEFN_SLAB_ALLOC_TYPES(ENUM_)
#undef ENUM_
    LIMIT
};

inline constexpr uint32_t SlabAllocTypeValue(SlabAllocType sat) {
    return static_cast<uint32_t>(sat);
}

inline constexpr bool IsValidSlabAllocType(SlabAllocType sat) {
    return (sat > SlabAllocType::INVALID) && (sat < SlabAllocType::LIMIT);
}


//
// SlabAllocHeader
//
// A single word_t that describes the allocation.  Its format is cosmetically
// different for 32-bit and 64-bit systems, in that the 64-bit word has
// zeros for its high 32 bits.
//
// The low bit of a SlabAllocHeader is 0.  This is to enable the runtime
// to distinguish a SlabAllocHeader from a SlabSizeExtHeader.
//
// Format:
//             24        16         8         0
//      CCCC-CCCC SSSS-SSSS TTTT-TTTT FFFF-0000
//
class SlabAllocHeader
{
  public:
    static constexpr unsigned CARDNUM_SHIFT = 24;
    static constexpr uint32_t CARDNUM_MAX = 0xffu;

    static constexpr unsigned ALLOCSIZE_SHIFT = 16;
    static constexpr uint32_t ALLOCSIZE_MAX = 0xffu;

    static constexpr unsigned ALLOCTYPE_SHIFT = 8;
    static constexpr uint32_t ALLOCTYPE_MAX = 0xffu;

    static constexpr unsigned FLAGS_SHIFT = 4;
    static constexpr uint32_t FLAGS_MAX = 0xfu;

    static_assert(SlabAllocTypeValue(SlabAllocType::LIMIT) <= ALLOCTYPE_MAX,
                  "Too many types in SlabAllocType.");

  private:
    word_t bits_;

  public:
    inline SlabAllocHeader(uint32_t cardNum, uint32_t allocSize,
                           SlabAllocType allocType, uint32_t flags=0)
      : bits_((cardNum << CARDNUM_SHIFT) |
              (allocSize << ALLOCSIZE_SHIFT) |
              (SlabAllocTypeValue(allocType) << ALLOCTYPE_SHIFT) |
              (flags << FLAGS_SHIFT))
    {
        WH_ASSERT(cardNum <= CARDNUM_MAX);
        WH_ASSERT(allocSize <= ALLOCSIZE_MAX);
        WH_ASSERT(IsValidSlabAllocType(allocType));
        WH_ASSERT(flags <= FLAGS_MAX);
    }

    inline uint32_t cardNum() const {
        return (bits_ >> CARDNUM_SHIFT) & CARDNUM_MAX;
    }

    inline uint32_t allocSize() const {
        return (bits_ >> ALLOCSIZE_SHIFT) & ALLOCSIZE_MAX;
    }

    inline SlabAllocType allocType() const {
        return static_cast<SlabAllocType>(
                (bits_ >> ALLOCTYPE_SHIFT) & ALLOCTYPE_MAX);
    }

    inline uint8_t flags() const {
        return (bits_ >> FLAGS_SHIFT) & FLAGS_MAX;
    }
};


//
// SlabSizeExtHeader
//
// For allocations with a size greater than SlabAllocHeader::SIZE_MAX,
// this header word follows the SlabAllocHeader word.
//
// The value is shifted up by 1 bit and the low bit set to 1, to enable
// the runtime to distinguish between this size ext word and the alloc
// header word.
//
// Format:
//             24        16         8         0
//      SSSS-SSSS SSSS-SSSS SSSS-SSSS SSSS-SSS1
//
class SlabSizeExtHeader
{
  public:
    static constexpr unsigned ALLOCSIZE_SHIFT = 1;
    static constexpr uint32_t ALLOCSIZE_MAX = 0x7fffffffu;

  private:
    word_t bits_;

  public:
    inline SlabSizeExtHeader(uint32_t allocSize)
      : bits_((allocSize << ALLOCSIZE_SHIFT) & 0x1u)
    {
        WH_ASSERT(allocSize <= ALLOCSIZE_MAX);
    }

    inline uint32_t allocSize() const {
        return (bits_ >> ALLOCSIZE_SHIFT) & ALLOCSIZE_MAX;
    }
};

class SlabThing;

//
// All objects which are allocated on the slab must provide a
// specialization for SlabThingTraits that enables the extraction
// of a SlabThing pointer from a pointer to the object.
//
template <typename T>
struct SlabThingTraits
{
    SlabThingTraits() = delete;
    SlabThingTraits(const SlabThingTraits &) = delete;
    
    static constexpr bool SPECIALIZED = false;
    // static constexpr bool SPECIALIZED = true;
};

//
// SlabThing
//
// SlabThing represents a pointer to a thing on the slab.  Objects which
// are slab allocated do not need to inherit from SlabThing.  Instead,
// they must provide a specialization for SlabThingTraits that allows
// the extraction of a slab thing pointer from them.
//
class SlabThing
{
  private:
    SlabThing() = delete;

    inline word_t *back_() {
        return reinterpret_cast<word_t *>(
                    reinterpret_cast<uint8_t *>(this) - sizeof(word_t));
    }
    inline const word_t *back_() const {
        return reinterpret_cast<const word_t *>(
                    reinterpret_cast<const uint8_t *>(this) - sizeof(word_t));
    }

    inline word_t *back2_() {
        return reinterpret_cast<word_t *>(
                    reinterpret_cast<uint8_t *>(this) - sizeof(word_t));
    }
    inline const word_t *back2_() const {
        return reinterpret_cast<const word_t *>(
                    reinterpret_cast<const uint8_t *>(this) - sizeof(word_t));
    }

  public:
    template <typename T>
    static inline SlabThing *From(T *ptr) {
        static_assert(SlabThingTraits<T>::SPECIALIZED,
                      "Type is not specialized for slab-thing.");
        return reinterpret_cast<SlabThing *>(ptr);
    }
    template <typename T>
    static inline const SlabThing *From(const T *ptr) {
        static_assert(SlabThingTraits<T>::SPECIALIZED,
                      "Type is not specialized for slab-thing.");
        return reinterpret_cast<const SlabThing *>(ptr);
    }

    inline bool isLarge() const {
        // Check the low bit of the preceding word.
        // If it's large, it'll be a sizeExt word with the low bit
        // set to 1.
        return *back_() & 0x1u;
    }

    inline const SlabAllocHeader &allocHeader() const {
        return *reinterpret_cast<const SlabAllocHeader *>(
                    isLarge() ? back2_() : back_());
    }

    inline SlabAllocHeader &allocHeader() {
        return *reinterpret_cast<SlabAllocHeader *>(
                    isLarge() ? back2_() : back_());
    }

    inline const SlabSizeExtHeader &sizeExtHeader() const {
        WH_ASSERT(isLarge());
        return *reinterpret_cast<const SlabSizeExtHeader *>(back_());
    }

    inline SlabSizeExtHeader &sizeExtHeader() {
        WH_ASSERT(isLarge());
        return *reinterpret_cast<SlabSizeExtHeader *>(back_());
    }

    inline uint32_t allocSize() const {
        return isLarge() ? sizeExtHeader().allocSize()
                         : allocHeader().allocSize();
    }

    inline uint8_t flags() const {
        return allocHeader().flags();
    }
#define CHK_(name) \
    inline bool is##name() const { \
        return allocHeader().allocType() == SlabAllocType::name; \
    }
    WHISPER_DEFN_SLAB_ALLOC_TYPES(CHK_)
#undef CHK_
};

//
// Specialize SlabThingTratis for SlabThing itself.
//
template <>
struct SlabThingTraits<SlabThing>
{
    SlabThingTraits() = delete;
    SlabThingTraits(const SlabThingTraits &) = delete;
    
    static constexpr bool SPECIALIZED = true;
};


} // namespace Whisper

#endif // WHISPER__SLAB_HPP
