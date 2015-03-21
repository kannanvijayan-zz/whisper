#ifndef WHISPER__VM__FUNCTION_HPP
#define WHISPER__VM__FUNCTION_HPP

#include "vm/core.hpp"
#include "vm/predeclare.hpp"
#include "vm/wobject.hpp"
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

    const NativeFunction *asNative() const {
        WH_ASSERT(isNative());
        return reinterpret_cast<const NativeFunction *>(this);
    }
    NativeFunction *asNative() {
        WH_ASSERT(isNative());
        return reinterpret_cast<NativeFunction *>(this);
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
        Handle<ScopeObject *> callerScope,
        Handle<Wobject *> receiver,
        ArrayHandle<Box> args,
        MutHandle<Box> result);

typedef OkResult (*NativeOperativeFuncPtr)(
        ThreadContext *cx,
        Handle<ScopeObject *> callerScope,
        Handle<Wobject *> receiver,
        ArrayHandle<SyntaxTreeFragment *> argExprs,
        MutHandle<Box> result);

class NativeFunction : public Function
{
    friend struct TraceTraits<NativeFunction>;

  private:
    static constexpr uint8_t OperativeFlag = 0x1;

    union {
        NativeApplicativeFuncPtr *applicative_;
        NativeOperativeFuncPtr *operative_;
    };

    HeapHeader &header() {
        return HeapThing::From(this)->header();
    }
    const HeapHeader &header() const {
        return HeapThing::From(this)->header();
    }

  public:
    NativeFunction(NativeApplicativeFuncPtr *applicative) {
        applicative_ = applicative;
    }

    NativeFunction(NativeOperativeFuncPtr *operative) {
        header().setUserData(OperativeFlag);
        operative_ = operative;
    }

    Result<NativeFunction *> Create(AllocationContext acx,
                                    NativeApplicativeFuncPtr *applicative);
    Result<NativeFunction *> Create(AllocationContext acx,
                                    NativeOperativeFuncPtr *operative);

    bool isApplicative() const {
        return (header().userData() & OperativeFlag) == 0;
    }
    bool isOperative() const {
        return (header().userData() & OperativeFlag) != 0;
    }

    NativeApplicativeFuncPtr *applicative() const {
        WH_ASSERT(isApplicative());
        return applicative_;
    }
    NativeOperativeFuncPtr *operative() const {
        WH_ASSERT(isOperative());
        return operative_;
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
struct HeapFormatTraits<HeapFormat::NativeFunction>
{
    HeapFormatTraits() = delete;
    typedef VM::NativeFunction Type;
};
template <>
struct TraceTraits<VM::NativeFunction>
  : public UntracedTraceTraits<VM::NativeFunction>
{};


template <>
struct HeapFormatTraits<HeapFormat::FunctionObject>
{
    HeapFormatTraits() = delete;
    typedef VM::FunctionObject Type;
};
template <>
struct TraceTraits<VM::FunctionObject>
  : public UntracedTraceTraits<VM::FunctionObject>
{};


} // namespace Whisper


#endif // WHISPER__VM__FUNCTION_HPP
