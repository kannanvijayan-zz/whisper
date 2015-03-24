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

  private:
    // The caller frame.
    HeapField<Frame *> caller_;

    // The scripted function being interpreted in this frame.
    HeapField<ScriptedFunction *> func_;

    // The maximal stack and eval depth.
    uint32_t maxStackDepth_;
    uint32_t maxEvalDepth_;

    // The current stack depth, and expression eval depth.
    uint32_t stackDepth_;
    uint32_t evalDepth_;

    // Implicit following fields:
    //
    //      Box stack_[maxStackDepth_]
    //      uint32_t evalOffsets_[maxEvalDepth_];
    //
    // stack_ holds Boxed values.
    // evalOffsets_ holds offsets into the the scripted function's
    // definition.

  public:
    Frame(Frame *caller, ScriptedFunction *func,
          uint32_t maxStackDepth, uint32_t maxEvalDepth)
      : caller_(caller),
        func_(func),
        maxStackDepth_(maxStackDepth),
        maxEvalDepth_(maxEvalDepth),
        stackDepth_(0),
        evalDepth_(0)
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
        return stackDepth_;
    }
    uint32_t evalDepth() const {
        return evalDepth_;
    }
    const HeapField<Box> &stackVal(uint32_t idx) const {
        WH_ASSERT(idx < stackDepth());
        return stackStart()[idx];
    }
    HeapField<Box> &stackVal(uint32_t idx) {
        WH_ASSERT(idx < stackDepth());
        return stackStart()[idx];
    }

  private:
    const HeapField<Box> *stackStart() const {
        const uint8_t *ptr = reinterpret_cast<const uint8_t *>(this);
        return reinterpret_cast<const HeapField<Box> *>(ptr + StackOffset());
    }
    const HeapField<Box> *stackEnd() const {
        return stackStart() + stackDepth_;
    }
    HeapField<Box> *stackStart() {
        uint8_t *ptr = reinterpret_cast<uint8_t *>(this);
        return reinterpret_cast<HeapField<Box> *>(ptr + StackOffset());
    }
    HeapField<Box> *stackEnd() {
        return stackStart() + stackDepth_;
    }
};


} // namespace VM


template <>
struct HeapFormatTraits<HeapFormat::Frame>
{
    HeapFormatTraits() = delete;
    typedef VM::Frame Type;
};

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
        for (uint32_t i = 0; i < obj.stackDepth_; i++)
            obj.stackVal(i).scan(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater &updater, VM::Frame &obj,
                       const void *start, const void *end)
    {
        obj.caller_.update(updater, start, end);
        obj.func_.update(updater, start, end);
        for (uint32_t i = 0; i < obj.stackDepth_; i++)
            obj.stackVal(i).update(updater, start, end);
    }
};


} // namespace Whisper


#endif // WHISPER__VM__FRAME_HPP
