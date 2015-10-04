#ifndef WHISPER__VM__BOX_HPP
#define WHISPER__VM__BOX_HPP

#include <sstream>
#include <cstdio>

#include "vm/predeclare.hpp"
#include "vm/array.hpp"

namespace Whisper {
namespace VM {

class Wobject;

//
// A Box is a 64-bit that represents a pointer or immediate value.
//
// Box format:
//
//  PPPP-PPPP ... PPPP-PPPP PPPP-PPPP PPPP-PPPP PPPP-P000   - Pointer
//  0000-0000 ... 0000-0000 0000-0000 0000-0000 0000-0001   - Undefined
//  IIII-IIII ... IIII-IIII IIII-IIII IIII-IIII 1000-0001   - Integer
//  0000-0000 ... 0000-0000 0000-00B1 0000-0000 B100-0001   - Boolean
//
class Box
{
    friend class TraceTraits<Box>;
  protected:
    uint64_t value_;

  public:
    static constexpr uint64_t PointerAlign = 0x8;

    static constexpr uint64_t PointerTagMask = 0x7;
    static constexpr uint64_t PointerTag = 0x0;

    static constexpr uint64_t PrimitiveTagMask = 0x7;
    static constexpr uint64_t PrimitiveTag = 0x1;

    static constexpr uint64_t UndefinedTagMask = 0xff;
    static constexpr uint64_t UndefinedTag = 0x01;

    static constexpr uint64_t IntegerTagMask = 0xff;
    static constexpr uint64_t IntegerTag = 0x81;
    static constexpr unsigned IntegerShift = 8;
    static constexpr int64_t IntegerMax =
        std::numeric_limits<int64_t>::max() >> IntegerShift;
    static constexpr int64_t IntegerMin =
        std::numeric_limits<int64_t>::min() >> IntegerShift;
    static bool IntegerInRange(int64_t ival) {
        return (ival >= IntegerMin) && (ival <= IntegerMax);
    }

    static constexpr uint64_t BooleanTagMask = 0x7f;
    static constexpr uint64_t BooleanTag = 0x41;
    static constexpr uint64_t BooleanShift = 7;
    static constexpr uint64_t BooleanBit = 1u << BooleanShift;

    static constexpr uint64_t InvalidValue = static_cast<uint64_t>(-1);

  protected:
    Box(uint64_t word)
      : value_(word)
    {}

  public:
    Box() : value_(InvalidValue) {}

    bool isInvalid() const {
        return value_ == InvalidValue;
    }
    bool isValid() const {
        return !isInvalid();
    }
    static Box Invalid() {
        return Box();
    }

    bool isPrimitive() const {
        WH_ASSERT(isValid());
        return !isPointer();
    }

    bool isPointer() const {
        return (value_ & PointerTagMask) == PointerTag;
    }

    template <typename T>
    bool isPointerTo() const {
        static_assert(IsHeapThingType<T>(), "T is not a HeapThing type.");

        if (!isPointer())
            return false;

        HeapFormat format = pointer<HeapThing>()->format();
        return HeapTraits<T>::Format == format;
    }

    template <typename T>
    T* pointer() const {
        static_assert(PointerTag == 0, "");
        static_assert(IsHeapThingType<T>(), "T is not a HeapThing type.");
        WH_ASSERT(isPointer());
        WH_ASSERT(reinterpret_cast<T*>(value_) != nullptr);
        return reinterpret_cast<T*>(value_);
    }
    template <typename T>
    static Box Pointer(T* ptr) {
        static_assert(IsHeapThingType<T>(), "T is not a HeapThing type.");
        WH_ASSERT(ptr != nullptr);
        WH_ASSERT(IsPtrAligned(ptr, PointerAlign));
        return Box(reinterpret_cast<uint64_t>(ptr));
    }

    bool isUndefined() const {
        return (value_ & UndefinedTagMask) == UndefinedTag;
    }
    static Box Undefined() {
        return Box(UndefinedTag);
    }

    bool isInteger() const {
        return (value_ & IntegerTagMask) == IntegerTag;
    }
    int64_t integer() const {
        WH_ASSERT(isInteger());
        return static_cast<int64_t>(value_ >> IntegerShift);
    }
    static Box Integer(int64_t ival) {
        WH_ASSERT(IntegerInRange(ival));
        return Box((static_cast<uint64_t>(ival) << IntegerShift) |
                   IntegerTag);
    }

    bool isBoolean() const {
        return (value_ & BooleanTagMask) == BooleanTag;
    }
    bool boolean() const {
        WH_ASSERT(isBoolean());
        return (value_ & BooleanBit) ? true : false;
    }
    static Box True() {
        return Box(BooleanBit | BooleanTag);
    }
    static Box False() {
        return Box(BooleanTag);
    }
    static Box Boolean(bool val) {
        return Box((val ? BooleanBit : 0u) | BooleanTag);
    }

    void snprint(char* buf, size_t n) const;

  protected:
    template <typename T>
    void setPointer(T* ptr) {
        static_assert(IsHeapThingType<T>(), "T is not a HeapThing type.");
        WH_ASSERT(ptr != nullptr);
        WH_ASSERT(IsPtrAligned(ptr, PointerAlign));
        value_ = reinterpret_cast<uint64_t>(ptr);
    }
};

//
// A ValBox is a box for in-language values.  This means that all
// pointers stored in a ValBox are definitely pointers to Wobjects.
//
class ValBox : public Box
{
    friend class TraceTraits<ValBox>;

  protected:
    ValBox(uint64_t word)
      : Box(word)
    {}

  public:
    ValBox() : Box() {}
    explicit ValBox(Box const& box);

    static ValBox Invalid() {
        return ValBox();
    }

    template <typename T>
    static ValBox Pointer(T* ptr) {
        // Ensure ptr converts to Wobject.
        static_assert(std::is_convertible<T*, Wobject*>(),
                      "T is not a Wobject type.");
        WH_ASSERT(ptr != nullptr);
        WH_ASSERT(IsPtrAligned(ptr, PointerAlign));
        return ValBox(reinterpret_cast<uint64_t>(ptr));
    }
    Wobject* objectPointer() const {
        return pointer<Wobject>();
    }

    static ValBox Undefined() {
        return ValBox(UndefinedTag);
    }

    static ValBox Integer(int64_t ival) {
        WH_ASSERT(IntegerInRange(ival));
        return ValBox((static_cast<uint64_t>(ival) << IntegerShift) |
                      IntegerTag);
    }

    static ValBox True() {
        return ValBox(BooleanBit | BooleanTag);
    }
    static ValBox False() {
        return ValBox(BooleanTag);
    }
    static ValBox Boolean(bool val) {
        return ValBox((val ? BooleanBit : 0u) | BooleanTag);
    }

    static ValBox Object(Wobject* obj) {
        return Pointer<Wobject>(obj);
    }

    OkResult toString(ThreadContext* cx, std::ostringstream &out) const;

  private:
    template <typename T>
    void setPointer(T* ptr) {
        static_assert(IsHeapThingType<T>(), "T is not a HeapThing type.");
        WH_ASSERT(ptr != nullptr);
        WH_ASSERT(IsPtrAligned(ptr, PointerAlign));
        value_ = reinterpret_cast<uint64_t>(ptr);
    }
};

template <>
struct ArrayTraits<Box>
{
    ArrayTraits() = delete;

    static const bool Specialized = true;
    static const HeapFormat ArrayFormat = HeapFormat::BoxArray;
};

template <>
struct ArrayTraits<ValBox>
{
    ArrayTraits() = delete;

    static const bool Specialized = true;
    static const HeapFormat ArrayFormat = HeapFormat::ValBoxArray;
};


} // namespace VM


//
// GC-Specializations.
//

template <>
struct FieldTraits<VM::Box>
{
    FieldTraits() = delete;
    static constexpr bool Specialized = true;
};
template <>
struct FieldTraits<VM::ValBox>
{
    FieldTraits() = delete;
    static constexpr bool Specialized = true;
};

template <>
struct HeapFormatTraits<HeapFormat::BoxArray>
{
    HeapFormatTraits() = delete;
    typedef VM::Array<VM::Box> Type;
};
template <>
struct HeapFormatTraits<HeapFormat::ValBoxArray>
{
    HeapFormatTraits() = delete;
    typedef VM::Array<VM::ValBox> Type;
};

template <>
struct TraceTraits<VM::Box>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner& scanner, VM::Box const& box,
                     void const* start, void const* end)
    {
        if (!box.isPointer())
            return;
        HeapThing* heapThing = box.pointer<HeapThing>();
        WH_ASSERT(heapThing != nullptr);
        scanner(&(box.value_), heapThing);
    }

    template <typename Updater>
    static void Update(Updater& updater, VM::Box& box,
                       void const* start, void const* end)
    {
        if (!box.isPointer())
            return;
        HeapThing* heapThing = box.pointer<HeapThing>();
        WH_ASSERT(heapThing != nullptr);
        HeapThing* updated = updater(&(box.value_), heapThing);
        if (updated != heapThing)
            box.setPointer(updated);
    }
};
template <>
struct TraceTraits<VM::ValBox>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner& scanner, VM::ValBox const& box,
                     void const* start, void const* end)
    {
        TraceTraits<VM::Box>::Scan<Scanner>(scanner, box, start, end);
    }

    template <typename Updater>
    static void Update(Updater& updater, VM::Box& box,
                       void const* start, void const* end)
    {
        TraceTraits<VM::Box>::Update<Updater>(updater, box, start, end);
    }
};


} // namespace Whisper


#endif // WHISPER__VM__BOX_HPP
