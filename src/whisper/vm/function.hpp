#ifndef WHISPER__VM__FUNCTION_HPP
#define WHISPER__VM__FUNCTION_HPP

#include "vm/core.hpp"
#include "vm/predeclare.hpp"
#include "vm/wobject.hpp"
#include "vm/lookup_state.hpp"
#include "vm/hash_object.hpp"
#include "vm/scope_object.hpp"
#include "vm/packed_syntax_tree.hpp"
#include "vm/box.hpp"
#include "vm/control_flow.hpp"

namespace Whisper {
namespace VM {


class NativeFunction;
class FunctionObject;
class LookupState;

//
// Base type for either a native function or a scripted function.
// NOTE: scripted functions not yet implemented.
//
class Function
{
  protected:
    Function() {}

  public:
    bool isNative() const {
        return HeapThing::From(this)->isNativeFunction();
    }
    bool isScripted() const {
        return HeapThing::From(this)->isScriptedFunction();
    }

    NativeFunction const* asNative() const {
        WH_ASSERT(isNative());
        return reinterpret_cast<NativeFunction const*>(this);
    }
    NativeFunction* asNative() {
        WH_ASSERT(isNative());
        return reinterpret_cast<NativeFunction*>(this);
    }

    ScriptedFunction const* asScripted() const {
        WH_ASSERT(isScripted());
        return reinterpret_cast<ScriptedFunction const*>(this);
    }
    ScriptedFunction* asScripted() {
        WH_ASSERT(isScripted());
        return reinterpret_cast<ScriptedFunction*>(this);
    }

    bool isApplicative() const;
    bool isOperative() const {
        return !isApplicative();
    }

    static bool IsFunctionFormat(HeapFormat format) {
        switch (format) {
          case HeapFormat::NativeFunction:
          case HeapFormat::ScriptedFunction:
            return true;
          default:
            return false;
        }
    }
    static bool IsFunction(HeapThing* heapThing) {
        return IsFunctionFormat(heapThing->format());
    }
};

class NativeCallInfo
{
    friend struct TraceTraits<NativeCallInfo>;

  private:
    StackField<LookupState*> lookupState_;
    StackField<ScopeObject*> scope_;
    StackField<FunctionObject*> calleeFunc_;
    StackField<ValBox> receiver_;

  public:
    NativeCallInfo(LookupState* lookupState,
                   ScopeObject* scope,
                   FunctionObject* calleeFunc,
                   ValBox receiver)
      : lookupState_(lookupState),
        scope_(scope),
        calleeFunc_(calleeFunc),
        receiver_(receiver)
    {
        WH_ASSERT(lookupState_ != nullptr);
        WH_ASSERT(scope_ != nullptr);
        WH_ASSERT(calleeFunc_ != nullptr);
        WH_ASSERT(receiver_->isValid());
    }

    Handle<LookupState*> lookupState() const {
        return lookupState_;
    }
    Handle<ScopeObject*> scope() const {
        return scope_;
    }
    Handle<FunctionObject*> calleeFunc() const {
        return calleeFunc_;
    }
    Handle<ValBox> receiver() const {
        return receiver_;
    }
};

typedef ControlFlow (*NativeApplicativeFuncPtr)(
        ThreadContext* cx,
        Handle<NativeCallInfo> callInfo,
        ArrayHandle<ValBox> args);

typedef ControlFlow (*NativeOperativeFuncPtr)(
        ThreadContext* cx,
        Handle<NativeCallInfo> callInfo,
        ArrayHandle<SyntaxNodeRef> args);

class NativeFunction : public Function
{
    friend struct TraceTraits<NativeFunction>;

  private:
    static constexpr uint8_t OperativeFlag = 0x1;

    union {
        NativeApplicativeFuncPtr applicative_;
        NativeOperativeFuncPtr operative_;
    };

    HeapHeader& header() {
        return HeapThing::From(this)->header();
    }
    HeapHeader const& header() const {
        return HeapThing::From(this)->header();
    }

  public:
    NativeFunction(NativeApplicativeFuncPtr applicative) {
        applicative_ = applicative;
    }

    NativeFunction(NativeOperativeFuncPtr operative) {
        header().setUserData(OperativeFlag);
        operative_ = operative;
    }

    static Result<NativeFunction*> Create(AllocationContext acx,
                                           NativeApplicativeFuncPtr app);
    static Result<NativeFunction*> Create(AllocationContext acx,
                                           NativeOperativeFuncPtr oper);

    bool isApplicative() const {
        return (header().userData() & OperativeFlag) == 0;
    }
    bool isOperative() const {
        return (header().userData() & OperativeFlag) != 0;
    }

    NativeApplicativeFuncPtr applicative() const {
        WH_ASSERT(isApplicative());
        return applicative_;
    }
    NativeOperativeFuncPtr operative() const {
        WH_ASSERT(isOperative());
        return operative_;
    }
};

class ScriptedFunction : public Function
{
    friend struct TraceTraits<ScriptedFunction>;
  private:
    static constexpr uint8_t OperativeFlag = 0x1;

    // The syntax tree of the definition.
    HeapField<PackedSyntaxTree*> pst_;
    uint32_t offset_;

    // The scope chain for the function.
    HeapField<ScopeObject*> scopeChain_;

    HeapHeader& header() {
        return HeapThing::From(this)->header();
    }
    HeapHeader const& header() const {
        return HeapThing::From(this)->header();
    }
  public:
    ScriptedFunction(PackedSyntaxTree* pst,
                     uint32_t offset,
                     ScopeObject* scopeChain,
                     bool isOperative)
      : pst_(pst),
        offset_(offset),
        scopeChain_(scopeChain)
    {
        WH_ASSERT(pst != nullptr);
        WH_ASSERT(scopeChain != nullptr);

        if (isOperative)
            header().setUserData(OperativeFlag);
    }

    static Result<ScriptedFunction*> Create(
            AllocationContext acx,
            Handle<PackedSyntaxTree*> pst,
            uint32_t offset,
            Handle<ScopeObject*> scopeChain,
            bool isOperative);

    bool isApplicative() const {
        return (header().userData() & OperativeFlag) == 0;
    }
    bool isOperative() const {
        return (header().userData() & OperativeFlag) != 0;
    }

    PackedSyntaxTree* pst() const {
        return pst_;
    }
    uint32_t offset() const {
        return offset_;
    }
    ScopeObject* scopeChain() const {
        return scopeChain_;
    }

    SyntaxNodeRef stRef() const {
        return SyntaxNodeRef(pst_, offset_);
    }

    uint32_t numParams() const {
        return stRef().astDefStmt().numParams();
    }
    VM::String *paramName(uint32_t paramNo) const {
        WH_ASSERT(paramNo < numParams());
        uint32_t paramCid = stRef().astDefStmt().paramCid(paramNo);
        return pst_->getConstantString(paramCid);
    }

    SyntaxBlockRef bodyBlockRef() const {
        return SyntaxBlockRef(pst_, stRef().astDefStmt().bodyBlock());
    }
};


class FunctionObject : public HashObject
{
    friend struct TraceTraits<FunctionObject>;
  private:
    HeapField<Function*> func_;
    HeapField<ValBox> receiver_;
    HeapField<LookupState*> lookupState_;

  public:
    FunctionObject(Handle<Array<Wobject*>*> delegates,
                   Handle<PropertyDict*> dict,
                   Handle<Function*> func,
                   Handle<ValBox> receiver,
                   Handle<LookupState*> lookupState)
      : HashObject(delegates, dict),
        func_(func),
        receiver_(receiver),
        lookupState_(lookupState)
    {
        WH_ASSERT(func.get() != nullptr);
        WH_ASSERT(receiver->isValid());
        WH_ASSERT(lookupState.get() != nullptr);
    }

    static Result<FunctionObject*> Create(
            AllocationContext acx,
            Handle<Function*> func,
            Handle<ValBox> receiver,
            Handle<LookupState*> lookupState);

    static Result<FunctionObject*> Create(
            AllocationContext acx,
            Handle<Function*> func,
            Handle<Wobject*> receiver,
            Handle<LookupState*> lookupState)
    {
        Local<ValBox> receiverBox(acx, ValBox::Object(receiver));
        return Create(acx, func, receiverBox, lookupState);
    }

    WobjectHooks const* getFunctionObjectHooks() const;

    Function* func() const {
        return func_;
    }

    ValBox const& receiver() const {
        return receiver_;
    }

    LookupState* lookupState() const {
        return lookupState_;
    }

    bool isApplicative() const {
        return func_->isApplicative();
    }
    bool isOperative() const {
        return func_->isOperative();
    }
};


} // namespace VM


//
// GC Specializations
//

template <>
struct TraceTraits<VM::NativeFunction>
  : public UntracedTraceTraits<VM::NativeFunction>
{};

template <>
struct TraceTraits<VM::ScriptedFunction>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner& scanner, VM::ScriptedFunction const& func,
                     void const* start, void const* end)
    {
        func.pst_.scan(scanner, start, end);
        func.scopeChain_.scan(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater& updater, VM::ScriptedFunction& func,
                       void const* start, void const* end)
    {
        func.pst_.update(updater, start, end);
        func.scopeChain_.update(updater, start, end);
    }
};

template <>
struct TraceTraits<VM::FunctionObject>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner& scanner, VM::FunctionObject const& obj,
                     void const* start, void const* end)
    {
        obj.func_.scan(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater& updater, VM::FunctionObject& obj,
                       void const* start, void const* end)
    {
        obj.func_.update(updater, start, end);
    }
};

template <>
struct TraceTraits<VM::NativeCallInfo>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner& scanner, VM::NativeCallInfo const& info,
                     void const* start, void const* end)
    {
        info.lookupState_.scan(scanner, start, end);
        info.scope_.scan(scanner, start, end);
        info.calleeFunc_.scan(scanner, start, end);
        info.receiver_.scan(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater& updater, VM::NativeCallInfo& info,
                       void const* start, void const* end)
    {
        info.lookupState_.update(updater, start, end);
        info.scope_.update(updater, start, end);
        info.calleeFunc_.update(updater, start, end);
        info.receiver_.update(updater, start, end);
    }
};


} // namespace Whisper


#endif // WHISPER__VM__FUNCTION_HPP
