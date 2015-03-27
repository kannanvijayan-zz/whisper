#ifndef WHISPER__VM__FRAME_HPP
#define WHISPER__VM__FRAME_HPP

#include "vm/core.hpp"
#include "vm/predeclare.hpp"
#include "vm/box.hpp"

namespace Whisper {
namespace VM {

//
// An interpreter stack frame.
//
class Frame
{
    friend class TraceTraits<Frame>;

  public:
    // The packed syntax tree buffer is at most 2**28 words long, so
    // 4 bits of the offset are usable.  The offset is shifted 4 bits,
    // and the low 4 bits are used to store the kind of syntax
    // element an offset refers to.
    enum class OffsetKind : uint32_t {
        TopLevel    = 0x0,
        Statement   = 0x1
    };

    class OffsetAndKind
    {
      private:
        uint32_t word_;

      public:
        explicit OffsetAndKind(uint32_t word) : word_(word) {}

        OffsetKind kind() const {
            return static_cast<OffsetKind>(word_ & 0xf);
        }
        uint32_t offset() const {
            return word_ >> 4;
        }
    };

  private:
    // The caller frame.
    HeapField<Frame *> caller_;

    // The scripted function being interpreted in this frame.
    HeapField<ScriptedFunction *> func_;

    // The maximal stack and eval depth.
    uint32_t maxStackDepth_;
    uint32_t maxEvalDepth_;

    // The current stack top, and expression eval top.
    HeapField<Box> *stackTop_;
    OffsetAndKind *evalTop_;

    // Implicit following fields:
    //
    //      Box stack_[maxStackDepth_]
    //      OffsetAndKind evalOffsets_[maxEvalDepth_];
    //
    // * stack_ holds Boxed values.
    // * evalOffsets_ holds OffsetAndKind values pointing into the
    //   scripted function's packed syntax tree buffer.

  public:
    Frame(Frame *caller, ScriptedFunction *func,
          uint32_t maxStackDepth, uint32_t maxEvalDepth)
      : caller_(caller),
        func_(func),
        maxStackDepth_(maxStackDepth),
        maxEvalDepth_(maxEvalDepth),
        stackTop_(stackStart()),
        evalTop_(evalStart())
    {}

    static uint32_t StackOffset() {
        return AlignIntUp<uint32_t>(sizeof(Frame), alignof(Box));
    }
    static uint32_t StackEndOffset(uint32_t maxStackDepth) {
        return StackOffset() + (sizeof(Box) * maxStackDepth);
    }
    static uint32_t EvalOffset(uint32_t maxStackDepth) {
        return AlignIntUp<uint32_t>(StackEndOffset(maxStackDepth),
                                    alignof(uint32_t));
    }
    static uint32_t EvalEndOffset(uint32_t maxStackDepth,
                                  uint32_t maxEvalDepth)
    {
        return EvalOffset(maxStackDepth) + (sizeof(uint32_t) * maxEvalDepth);
    }
    static uint32_t CalculateSize(uint32_t maxStackDepth,
                                  uint32_t maxEvalDepth)
    {
        return EvalEndOffset(maxStackDepth, maxEvalDepth);
    }

    static Result<Frame *> Create(AllocationContext acx,
                                  Handle<Frame *> caller,
                                  Handle<ScriptedFunction *> func,
                                  uint32_t maxStackDepth,
                                  uint32_t maxEvalDepth);

    Frame *caller() const {
        return caller_;
    }
    ScriptedFunction *func() const {
        return func_;
    }
    uint32_t maxStackDepth() const {
        return maxStackDepth_;
    }
    uint32_t maxEvalDepth() const {
        return maxEvalDepth_;
    }
    uint32_t stackDepth() const {
        return stackTop_ - stackStart();
    }
    uint32_t evalDepth() const {
        return evalTop_ - evalStart();
    }

    const HeapField<Box> &stackVal(uint32_t idx) const {
        WH_ASSERT(idx < stackDepth());
        return stackTop_[-ToInt32(idx)];
    }
    HeapField<Box> &stackVal(uint32_t idx) {
        WH_ASSERT(idx < stackDepth());
        return stackTop_[-ToInt32(idx)];
    }

    const OffsetAndKind &evalElement(uint32_t idx) const {
        WH_ASSERT(idx < evalDepth());
        return evalTop_[-ToInt32(idx)];
    }
    OffsetAndKind &evalElement(uint32_t idx) {
        WH_ASSERT(idx < evalDepth());
        return evalTop_[-ToInt32(idx)];
    }

  private:
    const HeapField<Box> *stackStart() const {
        const uint8_t *ptr = reinterpret_cast<const uint8_t *>(this);
        return reinterpret_cast<const HeapField<Box> *>(ptr + StackOffset());
    }
    HeapField<Box> *stackStart() {
        uint8_t *ptr = reinterpret_cast<uint8_t *>(this);
        return reinterpret_cast<HeapField<Box> *>(ptr + StackOffset());
    }
    const OffsetAndKind *evalStart() const {
        const uint8_t *ptr = reinterpret_cast<const uint8_t *>(this);
        return reinterpret_cast<const OffsetAndKind *>(
                ptr + EvalOffset(maxStackDepth_));
    }
    OffsetAndKind *evalStart() {
        uint8_t *ptr = reinterpret_cast<uint8_t *>(this);
        return reinterpret_cast<OffsetAndKind *>(
                ptr + EvalOffset(maxStackDepth_));
    }
};


} // namespace VM


//
// GC Specializations
//

template <>
struct TraceTraits<VM::Frame>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner &scanner, const VM::Frame &obj,
                     const void *start, const void *end)
    {
        obj.caller_.scan(scanner, start, end);
        obj.func_.scan(scanner, start, end);
        for (uint32_t i = 0; i < obj.stackDepth(); i++)
            obj.stackVal(i).scan(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater &updater, VM::Frame &obj,
                       const void *start, const void *end)
    {
        obj.caller_.update(updater, start, end);
        obj.func_.update(updater, start, end);
        for (uint32_t i = 0; i < obj.stackDepth(); i++)
            obj.stackVal(i).update(updater, start, end);
    }
};


} // namespace Whisper


#endif // WHISPER__VM__FRAME_HPP
