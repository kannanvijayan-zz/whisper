#ifndef WHISPER__VM__FRAME_HPP
#define WHISPER__VM__FRAME_HPP

#include "vm/core.hpp"
#include "vm/predeclare.hpp"
#include "vm/box.hpp"
#include "vm/control_flow.hpp"

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

    Result<Frame*> resolveChild(ThreadContext* cx,
                                Handle<Frame*> child,
                                ControlFlow const& flow);
    Result<Frame*> step(ThreadContext* cx);

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
    HeapField<PackedSyntaxTree*> syntaxTree_;

    // Offset of the logical element relating to this evaluation.
    uint32_t syntaxOffset_;

    // The scope in effect.
    HeapField<ScopeObject*> scope_;

  public:
    EntryFrame(Frame* caller,
               PackedSyntaxTree* syntaxTree,
               uint32_t syntaxOffset,
               ScopeObject* scope)
      : Frame(caller),
        syntaxTree_(syntaxTree),
        syntaxOffset_(syntaxOffset),
        scope_(scope)
    {
        WH_ASSERT(caller != nullptr);
    }

    static Result<EntryFrame*> Create(AllocationContext acx,
                                      Handle<Frame*> caller,
                                      Handle<PackedSyntaxTree*> syntaxTree,
                                      uint32_t syntaxOffset,
                                      Handle<ScopeObject*> scope);

    static Result<EntryFrame*> Create(AllocationContext acx,
                                      Handle<PackedSyntaxTree*> syntaxTree,
                                      uint32_t syntaxOffset,
                                      Handle<ScopeObject*> scope);

    PackedSyntaxTree* syntaxTree() const {
        return syntaxTree_;
    }
    uint32_t syntaxOffset() const {
        return syntaxOffset_;
    }
    ScopeObject* scope() const {
        return scope_;
    }

    Result<Frame*> resolveEntryFrameChild(ThreadContext* cx,
                                          Handle<Frame*> child,
                                          ControlFlow const& flow);
    Result<Frame*> stepEntryFrame(ThreadContext* cx);
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
    {
        WH_ASSERT(caller != nullptr);
    }

    static Result<EvalFrame*> Create(AllocationContext acx,
                                     Handle<Frame*> caller,
                                     Handle<SyntaxTreeFragment*> syntax);

    static Result<EvalFrame*> Create(AllocationContext acx,
                                     Handle<SyntaxTreeFragment*> syntax);

    SyntaxTreeFragment* syntax() const {
        return syntax_;
    }

    Result<Frame*> resolveEvalFrameChild(ThreadContext* cx,
                                         Handle<Frame*> child,
                                         ControlFlow const& flow);
    Result<Frame*> stepEvalFrame(ThreadContext* cx);
};


class SyntaxFrame : public Frame
{
    friend class TraceTraits<SyntaxFrame>;

  private:
    // The syntax node being evaluated.
    HeapField<SyntaxNode*> node_;

    // The position within the syntax node being evaluated.
    uint32_t position_;

    // The C function pointer for handling a resolved child.
    typedef Result<Frame*> (*ResolveChildFunc)(
        ThreadContext* cx,
        Handle<SyntaxFrame*> frame,
        Handle<Frame*> child,
        ControlFlow const& flow);
    ResolveChildFunc resolveChildFunc_;

    // The C function pointer for stepping.
    typedef Result<Frame*> (*StepFunc)(
        ThreadContext* cx,
        Handle<SyntaxFrame*> frame);
    StepFunc stepFunc_;

  public:
    SyntaxFrame(Frame* caller, SyntaxNode* node, uint32_t position,
                ResolveChildFunc resolveChildFunc, StepFunc stepFunc)
      : Frame(caller),
        node_(node),
        position_(position),
        resolveChildFunc_(resolveChildFunc),
        stepFunc_(stepFunc)
    {
        WH_ASSERT(caller != nullptr);
    }

    static Result<SyntaxFrame*> Create(AllocationContext acx,
                                       Handle<Frame*> caller,
                                       Handle<SyntaxNode*> node,
                                       uint32_t position,
                                       ResolveChildFunc resolveChildFunc,
                                       StepFunc stepFunc);

    static Result<SyntaxFrame*> Create(AllocationContext acx,
                                       Handle<SyntaxNode*> node,
                                       uint32_t position,
                                       ResolveChildFunc resolveChildFunc,
                                       StepFunc stepFunc);

    SyntaxNode* node() const {
        return node_;
    }

    Result<Frame*> resolveSyntaxFrameChild(ThreadContext* cx,
                                           Handle<Frame*> child,
                                           ControlFlow const& flow);
    Result<Frame*> stepSyntaxFrame(ThreadContext* cx);
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
    {
        WH_ASSERT(caller != nullptr);
    }

    static Result<FunctionFrame*> Create(AllocationContext acx,
                                         Handle<Frame*> caller,
                                         Handle<Function*> anchor);

    static Result<FunctionFrame*> Create(AllocationContext acx,
                                         Handle<Function*> anchor);

    Function* function() const {
        return function_;
    }

    Result<Frame*> resolveFunctionFrameChild(ThreadContext* cx,
                                             Handle<Frame*> child,
                                             ControlFlow const& flow);
    Result<Frame*> stepFunctionFrame(ThreadContext* cx);
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
        obj.syntaxTree_.scan(scanner, start, end);
        obj.scope_.scan(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater& updater, VM::EntryFrame& obj,
                       void const* start, void const* end)
    {
        TraceTraits<VM::Frame>::Update<Updater>(updater, obj, start, end);
        obj.syntaxTree_.update(updater, start, end);
        obj.scope_.update(updater, start, end);
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
        obj.node_.scan(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater& updater, VM::SyntaxFrame& obj,
                       void const* start, void const* end)
    {
        TraceTraits<VM::Frame>::Update<Updater>(updater, obj, start, end);
        obj.node_.update(updater, start, end);
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
