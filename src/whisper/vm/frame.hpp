#ifndef WHISPER__VM__FRAME_HPP
#define WHISPER__VM__FRAME_HPP

#include "vm/core.hpp"
#include "vm/predeclare.hpp"
#include "vm/box.hpp"
#include "vm/control_flow.hpp"
#include "vm/slist.hpp"
#include "parser/packed_syntax.hpp"

namespace Whisper {
namespace VM {

#define WHISPER_DEFN_FRAME_KINDS(_) \
    _(TerminalFrame)                \
    _(EntryFrame)                   \
    _(SyntaxNameLookupFrame)        \
    _(InvokeSyntaxFrame)            \
    _(FileSyntaxFrame)              \
    _(CallExprSyntaxFrame)


#define PREDECLARE_FRAME_CLASSES_(name) class name;
    WHISPER_DEFN_FRAME_KINDS(PREDECLARE_FRAME_CLASSES_)
#undef PREDECLARE_FRAME_CLASSES_

//
// Base class for interpreter frames.
//
class Frame
{
    friend class TraceTraits<Frame>;
  protected:
    // The parent frame.
    HeapField<Frame*> parent_;

    Frame(Frame* parent)
      : parent_(parent)
    {}

  public:
    Frame* parent() const {
        return parent_;
    }

    static StepResult ResolveChild(ThreadContext* cx, Handle<Frame*> frame,
                                   Handle<Frame*> childFrame,
                                   EvalResult const& result);
    static StepResult Step(ThreadContext* cx, Handle<Frame*> frame);

    EntryFrame* maybeAncestorEntryFrame();
    EntryFrame* ancestorEntryFrame() {
        EntryFrame* result = maybeAncestorEntryFrame();
        WH_ASSERT(result);
        return result;
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

//
// An TerminalFrame is signifies the end of computation when its
// child is resolved.
//
// It is always the bottom-most frame in the frame stack, and
// thus has a null parent frame.
//
class TerminalFrame : public Frame
{
    friend class TraceTraits<TerminalFrame>;

  private:
    HeapField<EvalResult> result_;

  public:
    TerminalFrame()
      : Frame(nullptr),
        result_(EvalResult::Void())
    {}

    static Result<TerminalFrame*> Create(AllocationContext acx);

    EvalResult const& result() const {
        return result_;
    }

    static StepResult ResolveChildImpl(ThreadContext* cx,
                                       Handle<TerminalFrame*> frame,
                                       Handle<Frame*> childFrame,
                                       EvalResult const& result);
    static StepResult StepImpl(ThreadContext* cx,
                               Handle<TerminalFrame*> frame);
};

//
// An EntryFrame establishes an object in the frame chain which
// represents the entry into a new evaluation scope.  It establishes
// the PackedSyntaxTree in effect, the offset of the logical AST
// node the evaluation relates to (e.g. the File or DefStmt node),
// and the scope object in effect.
//
// All syntactic child frames within the lexical scope of this
// entry frame refer to it.
//
class EntryFrame : public Frame
{
    friend class TraceTraits<EntryFrame>;

  private:
    // The syntax tree in effect.
    HeapField<SyntaxTreeFragment*> stFrag_;

    // The scope in effect.
    HeapField<ScopeObject*> scope_;

  public:
    EntryFrame(Frame* parent,
               SyntaxTreeFragment* stFrag,
               ScopeObject* scope)
      : Frame(parent),
        stFrag_(stFrag),
        scope_(scope)
    {
        WH_ASSERT(parent != nullptr);
    }

    static Result<EntryFrame*> Create(AllocationContext acx,
                                      Handle<Frame*> parent,
                                      Handle<SyntaxTreeFragment*> stFrag,
                                      Handle<ScopeObject*> scope);

    SyntaxTreeFragment* stFrag() const {
        return stFrag_;
    }
    ScopeObject* scope() const {
        return scope_;
    }

    static StepResult ResolveChildImpl(ThreadContext* cx,
                                       Handle<EntryFrame*> frame,
                                       Handle<Frame*> childFrame,
                                       EvalResult const& result);
    static StepResult StepImpl(ThreadContext* cx, Handle<EntryFrame*> frame);
};

class SyntaxFrame : public Frame
{
    friend class TraceTraits<SyntaxFrame>;

  protected:
    // The entry frame corresponding to the syntax frame.
    HeapField<EntryFrame*> entryFrame_;

    // The syntax tree fargment corresponding to the frame being
    // evaluated.
    HeapField<SyntaxTreeFragment*> stFrag_;

    SyntaxFrame(Frame* parent,
                EntryFrame* entryFrame,
                SyntaxTreeFragment* stFrag)
      : Frame(parent),
        entryFrame_(entryFrame),
        stFrag_(stFrag)
    {
        WH_ASSERT(parent != nullptr);
        WH_ASSERT(entryFrame != nullptr);
        WH_ASSERT(stFrag != nullptr);
    }

  public:
    EntryFrame* entryFrame() const {
        return entryFrame_;
    }
    SyntaxTreeFragment* stFrag() const {
        return stFrag_;
    }
    ScopeObject* scope() const {
        return entryFrame_->scope();
    }
};

class SyntaxNameLookupFrame : public SyntaxFrame
{
    friend class TraceTraits<SyntaxNameLookupFrame>;

  public:
    SyntaxNameLookupFrame(Frame* parent,
                          EntryFrame* entryFrame,
                          SyntaxTreeFragment* stFrag)
      : SyntaxFrame(parent, entryFrame, stFrag)
    {}

    static Result<SyntaxNameLookupFrame*> Create(
            AllocationContext acx,
            Handle<Frame*> parent,
            Handle<EntryFrame*> entryFrame,
            Handle<SyntaxTreeFragment*> stFrag);

    static StepResult ResolveChildImpl(ThreadContext* cx,
                                       Handle<SyntaxNameLookupFrame*> frame,
                                       Handle<Frame*> childFrame,
                                       EvalResult const& result);
    static StepResult StepImpl(ThreadContext* cx,
                               Handle<SyntaxNameLookupFrame*> frame);
};

class InvokeSyntaxFrame : public SyntaxFrame
{
    friend class TraceTraits<InvokeSyntaxFrame>;
  private:
    HeapField<ValBox> syntaxHandler_;

  public:
    InvokeSyntaxFrame(Frame* parent,
                      EntryFrame* entryFrame,
                      SyntaxTreeFragment* stFrag,
                      ValBox syntaxHandler)
      : SyntaxFrame(parent, entryFrame, stFrag),
        syntaxHandler_(syntaxHandler)
    {}

    ValBox const& syntaxHandler() const {
        return syntaxHandler_;
    }

    static Result<InvokeSyntaxFrame*> Create(
            AllocationContext acx,
            Handle<Frame*> parent,
            Handle<EntryFrame*> entryFrame,
            Handle<SyntaxTreeFragment*> stFrag,
            Handle<ValBox> syntaxHandler);

    static StepResult ResolveChildImpl(ThreadContext* cx,
                                       Handle<InvokeSyntaxFrame*> frame,
                                       Handle<Frame*> childFrame,
                                       EvalResult const& result);
    static StepResult StepImpl(ThreadContext* cx,
                               Handle<InvokeSyntaxFrame*> frame);
};

class FileSyntaxFrame : public SyntaxFrame
{
    friend class TraceTraits<FileSyntaxFrame>;
  private:
    uint32_t statementNo_;

  public:
    FileSyntaxFrame(Frame* parent,
                    EntryFrame* entryFrame,
                    SyntaxTreeFragment* stFrag,
                    uint32_t statementNo)
      : SyntaxFrame(parent, entryFrame, stFrag),
        statementNo_(statementNo)
    {}

    uint32_t statementNo() const {
        return statementNo_;
    }

    static Result<FileSyntaxFrame*> Create(
            AllocationContext acx,
            Handle<Frame*> parent,
            Handle<EntryFrame*> entryFrame,
            Handle<SyntaxTreeFragment*> stFrag,
            uint32_t statementNo);

    static Result<FileSyntaxFrame*> CreateNext(
            AllocationContext acx,
            Handle<FileSyntaxFrame*> curFrame);

    static StepResult ResolveChildImpl(ThreadContext* cx,
                                       Handle<FileSyntaxFrame*> frame,
                                       Handle<Frame*> childFrame,
                                       EvalResult const& result);
    static StepResult StepImpl(ThreadContext* cx,
                               Handle<FileSyntaxFrame*> frame);
};

class CallExprSyntaxFrame : public SyntaxFrame
{
    friend class TraceTraits<CallExprSyntaxFrame>;
  public:
    enum class State : uint8_t { Callee, Arg, Invoke };

  private:
    State state_;
    uint32_t argNo_;
    HeapField<ValBox> callee_;
    HeapField<FunctionObject*> calleeFunc_;
    HeapField<Slist<ValBox>*> operands_;

  public:
    CallExprSyntaxFrame(Frame* parent,
                        EntryFrame* entryFrame,
                        SyntaxTreeFragment* stFrag,
                        State state,
                        uint32_t argNo,
                        ValBox const& callee,
                        FunctionObject* calleeFunc,
                        Slist<ValBox>* operands)
      : SyntaxFrame(parent, entryFrame, stFrag),
        state_(state),
        argNo_(argNo),
        callee_(callee),
        calleeFunc_(calleeFunc),
        operands_(operands)
    {}

    State state() const {
        return state_;
    }
    bool inCalleeState() const {
        return state() == State::Callee;
    }
    bool inArgState() const {
        return state() == State::Arg;
    }
    bool inInvokeState() const {
        return state() == State::Invoke;
    }

    uint32_t argNo() const {
        WH_ASSERT(inArgState());
        return argNo_;
    }
    ValBox const& callee() const {
        WH_ASSERT(inArgState() || inInvokeState());
        return callee_;
    }
    FunctionObject* calleeFunc() const {
        WH_ASSERT(inArgState() || inInvokeState());
        return calleeFunc_;
    }
    Slist<ValBox>* operands() const {
        WH_ASSERT(inArgState() || inInvokeState());
        return operands_;
    }

    static Result<CallExprSyntaxFrame*> CreateCallee(
            AllocationContext acx,
            Handle<Frame*> parent,
            Handle<EntryFrame*> entryFrame,
            Handle<SyntaxTreeFragment*> stFrag);

    static StepResult ResolveChildImpl(ThreadContext* cx,
                                       Handle<CallExprSyntaxFrame*> frame,
                                       Handle<Frame*> childFrame,
                                       EvalResult const& result);

    static StepResult StepImpl(ThreadContext* cx,
                               Handle<CallExprSyntaxFrame*> frame);

  private:
    static StepResult ResolveCalleeChild(
            ThreadContext* cx,
            Handle<CallExprSyntaxFrame*> frame,
            Handle<PackedSyntaxTree*> pst,
            Handle<AST::PackedCallExprNode> callExprNode,
            EvalResult const& result);

    static StepResult ResolveArgChild(
            ThreadContext* cx,
            Handle<CallExprSyntaxFrame*> frame,
            Handle<PackedSyntaxTree*> pst,
            Handle<AST::PackedCallExprNode> callExprNode,
            EvalResult const& result);

    static StepResult ResolveInvokeChild(
            ThreadContext* cx,
            Handle<CallExprSyntaxFrame*> frame,
            Handle<PackedSyntaxTree*> pst,
            Handle<AST::PackedCallExprNode> callExprNode,
            EvalResult const& result);

    static StepResult StepCallee(ThreadContext* cx,
                                 Handle<CallExprSyntaxFrame*> frame);
    static StepResult StepArg(ThreadContext* cx,
                              Handle<CallExprSyntaxFrame*> frame);
    static StepResult StepInvoke(ThreadContext* cx,
                                 Handle<CallExprSyntaxFrame*> frame);

    static StepResult StepSubexpr(ThreadContext* cx,
                                  Handle<CallExprSyntaxFrame*> frame,
                                  Handle<PackedSyntaxTree*> pst,
                                  uint32_t offset);
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
        obj.parent_.scan(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater& updater, VM::Frame& obj,
                       void const* start, void const* end)
    {
        obj.parent_.update(updater, start, end);
    }
};

template <>
struct TraceTraits<VM::TerminalFrame>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner& scanner, VM::TerminalFrame const& obj,
                     void const* start, void const* end)
    {
        TraceTraits<VM::Frame>::Scan<Scanner>(scanner, obj, start, end);
        obj.result_.scan(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater& updater, VM::TerminalFrame& obj,
                       void const* start, void const* end)
    {
        TraceTraits<VM::Frame>::Update<Updater>(updater, obj, start, end);
        obj.result_.update(updater, start, end);
    }
};


template <>
struct TraceTraits<VM::EntryFrame>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner& scanner, VM::EntryFrame const& obj,
                     void const* start, void const* end)
    {
        TraceTraits<VM::Frame>::Scan<Scanner>(scanner, obj, start, end);
        obj.stFrag_.scan(scanner, start, end);
        obj.scope_.scan(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater& updater, VM::EntryFrame& obj,
                       void const* start, void const* end)
    {
        TraceTraits<VM::Frame>::Update<Updater>(updater, obj, start, end);
        obj.stFrag_.update(updater, start, end);
        obj.scope_.update(updater, start, end);
    }
};

template <>
struct TraceTraits<VM::SyntaxFrame>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner& scanner, VM::SyntaxFrame const& obj,
                     void const* start, void const* end)
    {
        TraceTraits<VM::Frame>::Scan<Scanner>(scanner, obj, start, end);
        obj.entryFrame_.scan(scanner, start, end);
        obj.stFrag_.scan(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater& updater, VM::SyntaxFrame& obj,
                       void const* start, void const* end)
    {
        TraceTraits<VM::Frame>::Update<Updater>(updater, obj, start, end);
        obj.entryFrame_.update(updater, start, end);
        obj.stFrag_.update(updater, start, end);
    }
};

template <>
struct TraceTraits<VM::SyntaxNameLookupFrame>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner& scanner, VM::SyntaxNameLookupFrame const& obj,
                     void const* start, void const* end)
    {
        TraceTraits<VM::SyntaxFrame>::Scan<Scanner>(scanner, obj, start, end);
    }

    template <typename Updater>
    static void Update(Updater& updater, VM::SyntaxNameLookupFrame& obj,
                       void const* start, void const* end)
    {
        TraceTraits<VM::SyntaxFrame>::Update<Updater>(updater, obj, start, end);
    }
};

template <>
struct TraceTraits<VM::InvokeSyntaxFrame>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner& scanner, VM::InvokeSyntaxFrame const& obj,
                     void const* start, void const* end)
    {
        TraceTraits<VM::SyntaxFrame>::Scan<Scanner>(scanner, obj, start, end);
        obj.syntaxHandler_.scan(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater& updater, VM::InvokeSyntaxFrame& obj,
                       void const* start, void const* end)
    {
        TraceTraits<VM::SyntaxFrame>::Update<Updater>(updater, obj, start, end);
        obj.syntaxHandler_.update(updater, start, end);
    }
};

template <>
struct TraceTraits<VM::FileSyntaxFrame>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner& scanner, VM::FileSyntaxFrame const& obj,
                     void const* start, void const* end)
    {
        TraceTraits<VM::SyntaxFrame>::Scan<Scanner>(scanner, obj, start, end);
    }

    template <typename Updater>
    static void Update(Updater& updater, VM::FileSyntaxFrame& obj,
                       void const* start, void const* end)
    {
        TraceTraits<VM::SyntaxFrame>::Update<Updater>(updater, obj, start, end);
    }
};

template <>
struct TraceTraits<VM::CallExprSyntaxFrame>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner& scanner, VM::CallExprSyntaxFrame const& obj,
                     void const* start, void const* end)
    {
        TraceTraits<VM::SyntaxFrame>::Scan<Scanner>(scanner, obj, start, end);
        obj.callee_.scan(scanner, start, end);
        obj.operands_.scan(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater& updater, VM::CallExprSyntaxFrame& obj,
                       void const* start, void const* end)
    {
        TraceTraits<VM::SyntaxFrame>::Update<Updater>(updater, obj, start, end);
        obj.callee_.update(updater, start, end);
        obj.operands_.update(updater, start, end);
    }
};


} // namespace Whisper


#endif // WHISPER__VM__FRAME_HPP
