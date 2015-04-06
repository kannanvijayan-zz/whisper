#ifndef WHISPER__VM__FUNCTION_HPP
#define WHISPER__VM__FUNCTION_HPP

#include "vm/core.hpp"
#include "vm/predeclare.hpp"
#include "vm/wobject.hpp"
#include "vm/lookup_state.hpp"
#include "vm/hash_object.hpp"
#include "vm/scope_object.hpp"
#include "vm/packed_syntax_tree.hpp"

namespace Whisper {
namespace VM {


class NativeFunction;

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

    const NativeFunction *asNative() const {
        WH_ASSERT(isNative());
        return reinterpret_cast<const NativeFunction *>(this);
    }
    NativeFunction *asNative() {
        WH_ASSERT(isNative());
        return reinterpret_cast<NativeFunction *>(this);
    }

    const ScriptedFunction *asScripted() const {
        WH_ASSERT(isScripted());
        return reinterpret_cast<const ScriptedFunction *>(this);
    }
    ScriptedFunction *asScripted() {
        WH_ASSERT(isScripted());
        return reinterpret_cast<ScriptedFunction *>(this);
    }

    bool isApplicative() const;
    bool isOperative() const {
        return !isApplicative();
    }

    static bool IsFunctionFormat(HeapFormat format) {
        switch (format) {
          case HeapFormat::NativeFunction:
            return true;
          default:
            return false;
        }
    }
    static bool IsFunction(HeapThing *heapThing) {
        return IsFunctionFormat(heapThing->format());
    }
};


typedef OkResult (*NativeApplicativeFuncPtr)(
        ThreadContext *cx,
        Handle<LookupState *> lookupState,
        Handle<ScopeObject *> callerScope,
        Handle<Wobject *> receiver,
        ArrayHandle<Box> args,
        MutHandle<Box> result);

typedef OkResult (*NativeOperativeFuncPtr)(
        ThreadContext *cx,
        Handle<LookupState *> lookupState,
        Handle<ScopeObject *> callerScope,
        Handle<NativeFunction *> calleeFunc,
        Handle<Wobject *> receiver,
        ArrayHandle<SyntaxTreeFragment *> argExprs,
        MutHandle<Box> result);

class NativeFunction : public Function
{
    friend struct TraceTraits<NativeFunction>;

  private:
    static constexpr uint8_t OperativeFlag = 0x1;

    union {
        NativeApplicativeFuncPtr applicative_;
        NativeOperativeFuncPtr operative_;
    };

    HeapHeader &header() {
        return HeapThing::From(this)->header();
    }
    const HeapHeader &header() const {
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

    static Result<NativeFunction *> Create(AllocationContext acx,
                                           NativeApplicativeFuncPtr app);
    static Result<NativeFunction *> Create(AllocationContext acx,
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
    HeapField<SyntaxTreeFragment *> definition_;

    // The scope chain for the function.
    HeapField<ScopeObject *> scopeChain_;

    HeapHeader &header() {
        return HeapThing::From(this)->header();
    }
    const HeapHeader &header() const {
        return HeapThing::From(this)->header();
    }
  public:
    ScriptedFunction(SyntaxTreeFragment *definition,
                     ScopeObject *scopeChain,
                     bool isOperative)
      : definition_(definition),
        scopeChain_(scopeChain)
    {
        WH_ASSERT(definition != nullptr);
        WH_ASSERT(scopeChain != nullptr);

        if (isOperative)
            header().setUserData(OperativeFlag);
    }

    static Result<ScriptedFunction *> Create(
            AllocationContext acx,
            Handle<SyntaxTreeFragment *> definition,
            Handle<ScopeObject *> scopeChain,
            bool isOperative);

    bool isApplicative() const {
        return (header().userData() & OperativeFlag) == 0;
    }
    bool isOperative() const {
        return (header().userData() & OperativeFlag) != 0;
    }

    SyntaxTreeFragment *definition() const {
        return definition_;
    }
    ScopeObject *scopeChain() const {
        return scopeChain_;
    }
};


class FunctionObject : public HashObject
{
    friend struct TraceTraits<FunctionObject>;
  private:
    HeapField<Function *> func_;

  public:
    FunctionObject(Handle<Array<Wobject *> *> delegates,
                   Handle<PropertyDict *> dict,
                   Handle<Function *> func)
      : HashObject(delegates, dict),
        func_(func)
    {}

    static Result<FunctionObject *> Create(
            AllocationContext acx,
            Handle<Function *> func);

    Function *func() const {
        return func_;
    }

    static void GetDelegates(ThreadContext *cx,
                             Handle<FunctionObject *> obj,
                             MutHandle<Array<Wobject *> *> delegatesOut);

    static bool GetPropertyIndex(Handle<FunctionObject *> obj,
                                 Handle<String *> name,
                                 uint32_t *indexOut);

    static bool GetProperty(ThreadContext *cx,
                            Handle<FunctionObject *> obj,
                            Handle<String *> name,
                            MutHandle<PropertyDescriptor> result);

    static OkResult DefineProperty(ThreadContext *cx,
                                   Handle<FunctionObject *> obj,
                                   Handle<String *> name,
                                   Handle<PropertyDescriptor> defn);
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
    static void Scan(Scanner &scanner, const VM::ScriptedFunction &func,
                     const void *start, const void *end)
    {
        func.definition_.scan(scanner, start, end);
        func.scopeChain_.scan(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater &updater, VM::ScriptedFunction &func,
                       const void *start, const void *end)
    {
        func.definition_.update(updater, start, end);
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
    static void Scan(Scanner &scanner, const VM::FunctionObject &obj,
                     const void *start, const void *end)
    {
        obj.func_.scan(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater &updater, VM::FunctionObject &obj,
                       const void *start, const void *end)
    {
        obj.func_.update(updater, start, end);
    }
};


} // namespace Whisper


#endif // WHISPER__VM__FUNCTION_HPP
