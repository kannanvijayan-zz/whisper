#ifndef WHISPER__VM__FRAME_HPP
#define WHISPER__VM__FRAME_HPP

#include "vm/core.hpp"
#include "vm/predeclare.hpp"
#include "vm/box.hpp"
#include "vm/exception.hpp"
#include "vm/control_flow.hpp"
#include "vm/slist.hpp"
#include "vm/function.hpp"
#include "parser/packed_syntax.hpp"

namespace Whisper {
namespace VM {

#define WHISPER_DEFN_FRAME_KINDS(_) \
    _(TerminalFrame)                \
    _(EntryFrame)                   \
    _(InvokeSyntaxNodeFrame)        \
    _(FileSyntaxFrame)              \
    _(BlockSyntaxFrame)             \
    _(VarSyntaxFrame)               \
    _(CallExprSyntaxFrame)          \
    _(InvokeApplicativeFrame)       \
    _(InvokeOperativeFrame)         \
    _(NativeCallResumeFrame)


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

    static StepResult ResolveChild(ThreadContext* cx,
                                   Handle<Frame*> frame,
                                   Handle<EvalResult> result);

    static StepResult ResolveChild(ThreadContext* cx,
                                   Handle<Frame*> frame,
                                   EvalResult const& result)
    {
        Local<EvalResult> rootedResult(cx, result);
        return ResolveChild(cx, frame, rootedResult.handle());
    }

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
                                       Handle<EvalResult> result);
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
                                       Handle<EvalResult> result);
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
};

class InvokeSyntaxNodeFrame : public SyntaxFrame
{
    friend class TraceTraits<InvokeSyntaxNodeFrame>;
  public:
    InvokeSyntaxNodeFrame(Frame* parent,
                          EntryFrame* entryFrame,
                          SyntaxTreeFragment* stFrag)
      : SyntaxFrame(parent, entryFrame, stFrag)
    {}

    static Result<InvokeSyntaxNodeFrame*> Create(
            AllocationContext acx,
            Handle<Frame*> parent,
            Handle<EntryFrame*> entryFrame,
            Handle<SyntaxTreeFragment*> stFrag);

    static StepResult ResolveChildImpl(ThreadContext* cx,
                                       Handle<InvokeSyntaxNodeFrame*> frame,
                                       Handle<EvalResult> result);
    static StepResult StepImpl(ThreadContext* cx,
                               Handle<InvokeSyntaxNodeFrame*> frame);
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
                                       Handle<EvalResult> result);
    static StepResult StepImpl(ThreadContext* cx,
                               Handle<FileSyntaxFrame*> frame);
};

class BlockSyntaxFrame : public SyntaxFrame
{
    friend class TraceTraits<BlockSyntaxFrame>;
  private:
    uint32_t statementNo_;

  public:
    BlockSyntaxFrame(Frame* parent,
                     EntryFrame* entryFrame,
                     SyntaxTreeFragment* stFrag,
                     uint32_t statementNo)
      : SyntaxFrame(parent, entryFrame, stFrag),
        statementNo_(statementNo)
    {}

    uint32_t statementNo() const {
        return statementNo_;
    }

    static Result<BlockSyntaxFrame*> Create(
            AllocationContext acx,
            Handle<Frame*> parent,
            Handle<EntryFrame*> entryFrame,
            Handle<SyntaxTreeFragment*> stFrag,
            uint32_t statementNo);

    static Result<BlockSyntaxFrame*> CreateNext(
            AllocationContext acx,
            Handle<BlockSyntaxFrame*> curFrame);

    static StepResult ResolveChildImpl(ThreadContext* cx,
                                       Handle<BlockSyntaxFrame*> frame,
                                       Handle<EvalResult> result);
    static StepResult StepImpl(ThreadContext* cx,
                               Handle<BlockSyntaxFrame*> frame);
};

class VarSyntaxFrame : public SyntaxFrame
{
    friend class TraceTraits<VarSyntaxFrame>;
  private:
    uint32_t bindingNo_;

  public:
    VarSyntaxFrame(Frame* parent,
                   EntryFrame* entryFrame,
                   SyntaxTreeFragment* stFrag,
                   uint32_t bindingNo)
      : SyntaxFrame(parent, entryFrame, stFrag),
        bindingNo_(bindingNo)
    {}

    bool isConst() const {
        WH_ASSERT(stFrag_->isNode());
        return stFrag()->toNode()->isConstStmt();
    }
    bool isVar() const {
        WH_ASSERT(stFrag_->isNode());
        return stFrag()->toNode()->isVarStmt();
    }

    uint32_t bindingNo() const {
        return bindingNo_;
    }

    static Result<VarSyntaxFrame*> Create(
            AllocationContext acx,
            Handle<Frame*> parent,
            Handle<EntryFrame*> entryFrame,
            Handle<SyntaxTreeFragment*> stFrag,
            uint32_t bindingNo);

    static StepResult ResolveChildImpl(ThreadContext* cx,
                                       Handle<VarSyntaxFrame*> frame,
                                       Handle<EvalResult> result);
    static StepResult StepImpl(ThreadContext* cx,
                               Handle<VarSyntaxFrame*> frame);
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

    static Result<CallExprSyntaxFrame*> CreateFirstArg(
            AllocationContext acx,
            Handle<CallExprSyntaxFrame*> calleeFrame,
            Handle<ValBox> callee,
            Handle<FunctionObject*> calleeFunc);

    static Result<CallExprSyntaxFrame*> CreateNextArg(
            AllocationContext acx,
            Handle<CallExprSyntaxFrame*> calleeFrame,
            Handle<Slist<ValBox>*> operands);

    static Result<CallExprSyntaxFrame*> CreateInvoke(
            AllocationContext acx,
            Handle<CallExprSyntaxFrame*> frame,
            Handle<Slist<ValBox>*> operands);

    static Result<CallExprSyntaxFrame*> CreateInvoke(
            AllocationContext acx,
            Handle<CallExprSyntaxFrame*> frame,
            Handle<ValBox> callee,
            Handle<FunctionObject*> calleeFunc,
            Handle<Slist<ValBox>*> operands);

    static StepResult ResolveChildImpl(ThreadContext* cx,
                                       Handle<CallExprSyntaxFrame*> frame,
                                       Handle<EvalResult> result);

    static StepResult StepImpl(ThreadContext* cx,
                               Handle<CallExprSyntaxFrame*> frame);

  private:
    static StepResult ResolveCalleeChild(
            ThreadContext* cx,
            Handle<CallExprSyntaxFrame*> frame,
            Handle<PackedSyntaxTree*> pst,
            Handle<AST::PackedCallExprNode> callExprNode,
            Handle<EvalResult> result);

    static StepResult ResolveArgChild(
            ThreadContext* cx,
            Handle<CallExprSyntaxFrame*> frame,
            Handle<PackedSyntaxTree*> pst,
            Handle<AST::PackedCallExprNode> callExprNode,
            Handle<EvalResult> result);

    static StepResult ResolveInvokeChild(
            ThreadContext* cx,
            Handle<CallExprSyntaxFrame*> frame,
            Handle<PackedSyntaxTree*> pst,
            Handle<AST::PackedCallExprNode> callExprNode,
            Handle<EvalResult> result);

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

class InvokeApplicativeFrame : public Frame
{
    friend class TraceTraits<InvokeApplicativeFrame>;
  private:
    HeapField<ValBox> callee_;
    HeapField<FunctionObject*> calleeFunc_;
    HeapField<Slist<ValBox>*> operands_;

  public:
    InvokeApplicativeFrame(Frame* parent,
                           ValBox const& callee,
                           FunctionObject* calleeFunc,
                           Slist<ValBox>* operands)
      : Frame(parent),
        callee_(callee),
        calleeFunc_(calleeFunc),
        operands_(operands)
    {}

    ValBox const& callee() const {
        return callee_;
    }
    FunctionObject* calleeFunc() const {
        return calleeFunc_;
    }
    Slist<ValBox>* operands() const {
        return operands_;
    }

    static Result<InvokeApplicativeFrame*> Create(
            AllocationContext acx,
            Handle<Frame*> parent,
            Handle<ValBox> callee,
            Handle<FunctionObject*> calleeFunc,
            Handle<Slist<ValBox>*> operands);

    static StepResult ResolveChildImpl(ThreadContext* cx,
                                       Handle<InvokeApplicativeFrame*> frame,
                                       Handle<EvalResult> result);
    static StepResult StepImpl(ThreadContext* cx,
                               Handle<InvokeApplicativeFrame*> frame);
};

class InvokeOperativeFrame : public Frame
{
    friend class TraceTraits<InvokeOperativeFrame>;
  private:
    HeapField<ValBox> callee_;
    HeapField<FunctionObject*> calleeFunc_;
    HeapField<SyntaxTreeFragment*> stFrag_;

  public:
    InvokeOperativeFrame(Frame* parent,
                         ValBox const& callee,
                         FunctionObject* calleeFunc,
                         SyntaxTreeFragment* stFrag)
      : Frame(parent),
        callee_(callee),
        calleeFunc_(calleeFunc),
        stFrag_(stFrag)
    {}

    ValBox const& callee() const {
        return callee_;
    }
    FunctionObject* calleeFunc() const {
        return calleeFunc_;
    }
    SyntaxTreeFragment* stFrag() const {
        return stFrag_;
    }

    static Result<InvokeOperativeFrame*> Create(
            AllocationContext acx,
            Handle<Frame*> parent,
            Handle<ValBox> callee,
            Handle<FunctionObject*> calleeFunc,
            Handle<SyntaxTreeFragment*> stFrag);

    static StepResult ResolveChildImpl(ThreadContext* cx,
                                       Handle<InvokeOperativeFrame*> frame,
                                       Handle<EvalResult> result);
    static StepResult StepImpl(ThreadContext* cx,
                               Handle<InvokeOperativeFrame*> frame);
};

typedef VM::CallResult (*NativeCallResumeFuncPtr)(
        ThreadContext* cx,
        Handle<NativeCallInfo> callInfo,
        Handle<HeapThing*> state,
        Handle<VM::EvalResult> evalResult);

class NativeCallResumeFrame : public Frame
{
    friend class TraceTraits<NativeCallResumeFrame>;
  private:
    HeapField<LookupState*> lookupState_;
    HeapField<ScopeObject*> callerScope_;
    HeapField<FunctionObject*> calleeFunc_;
    HeapField<ValBox> receiver_;
    HeapField<ScopeObject*> evalScope_;
    HeapField<SyntaxTreeFragment*> syntaxFragment_;
    NativeCallResumeFuncPtr resumeFunc_;
    HeapField<HeapThing*> resumeState_;

  public:
    NativeCallResumeFrame(Frame* parent,
                          NativeCallInfo const& callInfo,
                          ScopeObject* evalScope,
                          SyntaxTreeFragment* syntaxFragment,
                          NativeCallResumeFuncPtr resumeFunc,
                          HeapThing* resumeState)
      : Frame(parent),
        lookupState_(callInfo.lookupState()),
        callerScope_(callInfo.callerScope()),
        calleeFunc_(callInfo.calleeFunc()),
        receiver_(callInfo.receiver()),
        evalScope_(evalScope),
        syntaxFragment_(syntaxFragment),
        resumeFunc_(resumeFunc),
        resumeState_(resumeState)
    {}

    LookupState* lookupState() const {
        return lookupState_;
    }

    ScopeObject* callerScope() const {
        return callerScope_;
    }

    FunctionObject* calleeFunc() const {
        return calleeFunc_;
    }

    ValBox const& receiver() const {
        return receiver_;
    }

    ScopeObject* evalScope() const {
        return evalScope_;
    }

    SyntaxTreeFragment* syntaxFragment() const {
        return syntaxFragment_;
    }

    NativeCallResumeFuncPtr resumeFunc() const {
        return resumeFunc_;
    }

    HeapThing* resumeState() const {
        return resumeState_;
    }

    static Result<NativeCallResumeFrame*> Create(
            AllocationContext acx,
            Handle<Frame*> parent,
            Handle<NativeCallInfo> callInfo,
            Handle<ScopeObject*> evalScope,
            Handle<SyntaxTreeFragment*> syntaxFragment,
            NativeCallResumeFuncPtr resumeFunc,
            Handle<HeapThing*> resumeState);

    static StepResult ResolveChildImpl(ThreadContext* cx,
                                       Handle<NativeCallResumeFrame*> frame,
                                       Handle<EvalResult> result);
    static StepResult StepImpl(ThreadContext* cx,
                               Handle<NativeCallResumeFrame*> frame);
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
struct TraceTraits<VM::InvokeSyntaxNodeFrame>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner& scanner, VM::InvokeSyntaxNodeFrame const& obj,
                     void const* start, void const* end)
    {
        TraceTraits<VM::SyntaxFrame>::Scan<Scanner>(scanner, obj, start, end);
    }

    template <typename Updater>
    static void Update(Updater& updater, VM::InvokeSyntaxNodeFrame& obj,
                       void const* start, void const* end)
    {
        TraceTraits<VM::SyntaxFrame>::Update<Updater>(updater, obj, start, end);
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
struct TraceTraits<VM::BlockSyntaxFrame>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner& scanner, VM::BlockSyntaxFrame const& obj,
                     void const* start, void const* end)
    {
        TraceTraits<VM::SyntaxFrame>::Scan<Scanner>(scanner, obj, start, end);
    }

    template <typename Updater>
    static void Update(Updater& updater, VM::BlockSyntaxFrame& obj,
                       void const* start, void const* end)
    {
        TraceTraits<VM::SyntaxFrame>::Update<Updater>(updater, obj, start, end);
    }
};

template <>
struct TraceTraits<VM::VarSyntaxFrame>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner& scanner, VM::VarSyntaxFrame const& obj,
                     void const* start, void const* end)
    {
        TraceTraits<VM::SyntaxFrame>::Scan<Scanner>(scanner, obj, start, end);
    }

    template <typename Updater>
    static void Update(Updater& updater, VM::VarSyntaxFrame& obj,
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

template <>
struct TraceTraits<VM::InvokeApplicativeFrame>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner& scanner, VM::InvokeApplicativeFrame const& obj,
                     void const* start, void const* end)
    {
        TraceTraits<VM::Frame>::Scan<Scanner>(scanner, obj, start, end);
        obj.callee_.scan(scanner, start, end);
        obj.calleeFunc_.scan(scanner, start, end);
        obj.operands_.scan(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater& updater, VM::InvokeApplicativeFrame& obj,
                       void const* start, void const* end)
    {
        TraceTraits<VM::Frame>::Update<Updater>(updater, obj, start, end);
        obj.callee_.update(updater, start, end);
        obj.calleeFunc_.update(updater, start, end);
        obj.operands_.update(updater, start, end);
    }
};

template <>
struct TraceTraits<VM::InvokeOperativeFrame>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner& scanner, VM::InvokeOperativeFrame const& obj,
                     void const* start, void const* end)
    {
        TraceTraits<VM::Frame>::Scan<Scanner>(scanner, obj, start, end);
        obj.callee_.scan(scanner, start, end);
        obj.calleeFunc_.scan(scanner, start, end);
        obj.stFrag_.scan(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater& updater, VM::InvokeOperativeFrame& obj,
                       void const* start, void const* end)
    {
        TraceTraits<VM::Frame>::Update<Updater>(updater, obj, start, end);
        obj.callee_.update(updater, start, end);
        obj.calleeFunc_.update(updater, start, end);
        obj.stFrag_.update(updater, start, end);
    }
};

template <>
struct TraceTraits<VM::NativeCallResumeFrame>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner& scanner, VM::NativeCallResumeFrame const& obj,
                     void const* start, void const* end)
    {
        TraceTraits<VM::Frame>::Scan<Scanner>(scanner, obj, start, end);
        obj.lookupState_.scan(scanner, start, end);
        obj.callerScope_.scan(scanner, start, end);
        obj.calleeFunc_.scan(scanner, start, end);
        obj.receiver_.scan(scanner, start, end);
        obj.evalScope_.scan(scanner, start, end);
        obj.syntaxFragment_.scan(scanner, start, end);
        obj.resumeState_.scan(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater& updater, VM::NativeCallResumeFrame& obj,
                       void const* start, void const* end)
    {
        TraceTraits<VM::Frame>::Update<Updater>(updater, obj, start, end);
        obj.lookupState_.update(updater, start, end);
        obj.callerScope_.update(updater, start, end);
        obj.calleeFunc_.update(updater, start, end);
        obj.receiver_.update(updater, start, end);
        obj.evalScope_.update(updater, start, end);
        obj.syntaxFragment_.update(updater, start, end);
        obj.resumeState_.update(updater, start, end);
    }
};


} // namespace Whisper


#endif // WHISPER__VM__FRAME_HPP
