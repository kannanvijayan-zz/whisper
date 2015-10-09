#ifndef WHISPER__VM__FRAME_HPP
#define WHISPER__VM__FRAME_HPP

#include "vm/core.hpp"
#include "vm/predeclare.hpp"
#include "vm/box.hpp"

namespace Whisper {
namespace VM {

#define WHISPER_DEFN_FRAME_KINDS(_) \
    _(EvalFrame) \
    _(FunctionFrame)

//
// Base class for interpreter frames.
//
class Frame
{
    friend class TraceTraits<Frame>;
  protected:
    // The caller frame.
    HeapField<Frame*> caller_;

    Frame(Frame* caller)
      : caller_(caller)
    {}

  public:
    Frame* caller() const {
        return caller_;
    }

#define FRAME_KIND_METHODS_(name) \
    bool is##name() const { \
        return HeapThing::From(this)->is##name(); \
    } \
    name const* to##name() const { \
        WH_ASSERT(is##name()); \
        return reinterpret_cast<name const*>(this); \
    } \
    name* to##name() { \
        WH_ASSERT(is##name()); \
        return reinterpret_cast<name*>(this); \
    }
    WHISPER_DEFN_FRAME_KINDS(FRAME_KIND_METHODS_)
#undef FRAME_KIND_METHODS_
};

class EvalFrame : public Frame
{
    friend class TraceTraits<EvalFrame>;

  private:
    // The syntax fragment being evaluated.
    HeapField<SyntaxTreeFragment*> syntax_;

  public:
    EvalFrame(Frame* caller, SyntaxTreeFragment* syntax)
      : Frame(caller),
        syntax_(syntax)
    {}

    static Result<EvalFrame*> Create(AllocationContext acx,
                                     Handle<Frame*> caller,
                                     Handle<SyntaxTreeFragment*> syntax);

    static Result<EvalFrame*> Create(AllocationContext acx,
                                     Handle<SyntaxTreeFragment*> syntax);

    SyntaxTreeFragment* syntax() const {
        return syntax_;
    }
};

class FunctionFrame : public Frame
{
    friend class TraceTraits<FunctionFrame>;

  private:
    // The syntax fragment being evaluated.
    HeapField<Function*> function_;

  public:
    FunctionFrame(Frame* caller, Function* function)
      : Frame(caller),
        function_(function)
    {}

    static Result<FunctionFrame*> Create(AllocationContext acx,
                                         Handle<Frame*> caller,
                                         Handle<Function*> anchor);

    static Result<FunctionFrame*> Create(AllocationContext acx,
                                         Handle<Function*> anchor);

    Function* function() const {
        return function_;
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
    static void Scan(Scanner& scanner, VM::Frame const& obj,
                     void const* start, void const* end)
    {
        obj.caller_.scan(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater& updater, VM::Frame& obj,
                       void const* start, void const* end)
    {
        obj.caller_.update(updater, start, end);
    }
};

template <>
struct TraceTraits<VM::EvalFrame>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner& scanner, VM::EvalFrame const& obj,
                     void const* start, void const* end)
    {
        TraceTraits<VM::Frame>::Scan<Scanner>(scanner, obj, start, end);
        obj.syntax_.scan(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater& updater, VM::EvalFrame& obj,
                       void const* start, void const* end)
    {
        TraceTraits<VM::Frame>::Update<Updater>(updater, obj, start, end);
        obj.syntax_.update(updater, start, end);
    }
};

template <>
struct TraceTraits<VM::FunctionFrame>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner& scanner, VM::FunctionFrame const& obj,
                     void const* start, void const* end)
    {
        TraceTraits<VM::Frame>::Scan<Scanner>(scanner, obj, start, end);
        obj.function_.scan(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater& updater, VM::FunctionFrame& obj,
                       void const* start, void const* end)
    {
        TraceTraits<VM::Frame>::Update<Updater>(updater, obj, start, end);
        obj.function_.update(updater, start, end);
    }
};


} // namespace Whisper


#endif // WHISPER__VM__FRAME_HPP
