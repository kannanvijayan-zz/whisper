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
    enum Kind {
        Eval,       /* Bare syntax evaluation. */
        Func        /* Function call. */
    };

  private:
    // The caller frame.
    HeapField<Frame*> caller_;

    // Kind of frame.
    Kind kind_;

    // Frame anchor.  This depends on the kind.
    // For Eval kind, this is a SyntaxTreeFragment.
    // For Func kind, this is a pointer to the function.
    HeapField<HeapThing*> anchor_;

    // An object representing the leave location from this frame to a
    // callee.  Value depends on kind of frame.
    // For Eval frames, this is a SyntaxTreeFragment.
    // For Func frames, depends on whether function is native or not.
    //  If func is native, then this is either null or String.
    //  If func is scripted, then this is a SyntaxTreeFragment referring
    //  to the call site.
    HeapField<HeapThing*> leaveLocation_;

  public:
    Frame(Frame* caller, Kind kind, HeapThing* anchor)
      : caller_(caller),
        kind_(kind),
        anchor_(anchor),
        leaveLocation_(nullptr)
    {}

    static Result<Frame*> CreateEval(AllocationContext acx,
                                     Handle<Frame*> caller,
                                     Handle<SyntaxTreeFragment*> anchor);

    static Result<Frame*> CreateFunc(AllocationContext acx,
                                     Handle<Frame*> caller,
                                     Handle<Function*> anchor);

    Frame* caller() const {
        return caller_;
    }
    Kind kind() const {
        return kind_;
    }
    bool isEval() const {
        return kind() == Eval;
    }
    bool isFunc() const {
        return kind() == Func;
    }
    SyntaxTreeFragment* anchorSyntax() const {
        WH_ASSERT(isEval());
        return anchor_->to<SyntaxTreeFragment>();
    }
    Function* anchorFunction() const {
        WH_ASSERT(isFunc());
        return anchor_->to<Function>();
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
        obj.anchor_.scan(scanner, start, end);
        obj.leaveLocation_.scan(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater& updater, VM::Frame& obj,
                       void const* start, void const* end)
    {
        obj.caller_.update(updater, start, end);
        obj.anchor_.update(updater, start, end);
        obj.leaveLocation_.update(updater, start, end);
    }
};


} // namespace Whisper


#endif // WHISPER__VM__FRAME_HPP
