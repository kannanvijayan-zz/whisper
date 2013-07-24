#ifndef WHISPER__ROOTING_HPP
#define WHISPER__ROOTING_HPP

#include "common.hpp"
#include "debug.hpp"
#include "value.hpp"
#include "runtime.hpp"

namespace Whisper {

template <typename T> class TypedHandleBase;
template <typename T> class TypedMutableHandleBase;

//
// RootKind is an enum describing the kind of thing being rooted.
// 
enum class RootKind : uint8_t
{
    INVALID = 0,
    Value,
    Object,
    LIMIT
};

//
// RootBase
//
// Base class for stack-rooted references to things.
//
class RootBase
{
  protected:
    ThreadContext *threadContext_;
    RootBase *next_;
    RootKind kind_;

    RootBase(ThreadContext *threadContext, RootKind kind)
      : threadContext_(threadContext),
        next_(threadContext->roots()),
        kind_(kind)
    {}

    void postInit() {
        threadContext_->roots_ = this;
    }

  public:
    ThreadContext *threadContext() const {
        return threadContext_;
    }
    RootBase *next() const {
        return next_;
    }
    RootKind kind() const {
        return kind_;
    }
};

//
// TypedRootBase<...>
//
// Type-specialized base class for rooted things.
//

template <typename T>
class TypedRootBase : public RootBase
{
  protected:
    T thing_;

  public:
    TypedRootBase(ThreadContext *threadContext, RootKind kind, const T &thing)
      : RootBase(threadContext, kind),
        thing_(thing)
    {
        postInit();
    }

    TypedRootBase(ThreadContext *threadContext, RootKind kind)
      : RootBase(threadContext, kind),
        thing_()
    {
        postInit();
    }

    TypedRootBase(RunContext *runContext, RootKind kind, const T &thing)
      : TypedRootBase(runContext->threadContext(), kind, thing)
    {}

    TypedRootBase(RunContext *runContext, RootKind kind)
      : TypedRootBase(runContext->threadContext(), kind)
    {}

    const T &get() const {
        return thing_;
    }
    T &get() {
        return thing_;
    }

    void set(const T &val) {
        thing_ = val;
    }

    operator const T &() const {
        return thing_;
    }
    operator T &() {
        return thing_;
    }

    TypedRootBase<T> &operator =(const T &other) {
        thing_ = other;
        return *this;
    }
    TypedRootBase<T> &operator =(const TypedRootBase<T> &other) {
        thing_ = other.thing_;
        return *this;
    }
    inline TypedRootBase<T> &operator =(const TypedHandleBase<T> &other);
    inline TypedRootBase<T> &operator =(const TypedMutableHandleBase<T> &other);
};

template <typename T>
class PointerRootBase : public TypedRootBase<T *>
{
  public:
    PointerRootBase(ThreadContext *threadContext, RootKind kind)
      : TypedRootBase<T *>(threadContext, kind, nullptr)
    {}

    PointerRootBase(ThreadContext *threadContext, RootKind kind, T *ptr)
      : TypedRootBase<T *>(threadContext, kind, ptr)
    {}

    T *operator ->() const {
        return this->thing_;
    }
};

template <typename T>
class Root
{
    Root(const Root<T> &other) = delete;
};


//
// Lightweight handle classes that uses an underlying Root.
//
template <typename T>
class TypedHandleBase
{
  protected:
    TypedRootBase<T> &rootBase_;

  public:
    TypedHandleBase(TypedRootBase<T> &rootBase)
      : rootBase_(rootBase)
    {}

    const T &get() const {
        return rootBase_.get();
    }

    operator const T &() const {
        return get();
    }
};

template <typename T>
class PointerHandleBase : public TypedHandleBase<T *>
{
  public:
    PointerHandleBase(TypedRootBase<T *> &base)
      : TypedHandleBase<T *>(base)
    {}

    T *operator ->() const {
        return this->rootBase_.get();
    }
};


//
// Lightweight mutable handle classes that uses an underlying Root.
//
template <typename T>
class TypedMutableHandleBase
{
  protected:
    TypedRootBase<T> &rootBase_;

  public:
    TypedMutableHandleBase(TypedRootBase<T> &rootBase)
      : rootBase_(rootBase)
    {}

    T &get() const {
        return rootBase_.get();
    }

    void set(const T &t) const {
        rootBase_.set(t);
    }

    operator T &() const {
        return get();
    }

    TypedMutableHandleBase<T> &operator =(const TypedRootBase<T> &rootBase) {
        rootBase_.set(rootBase.get());
    }

    TypedMutableHandleBase<T> &operator =(
        const TypedHandleBase<T> &hBase)
    {
        rootBase_.set(hBase.get());
    }

    TypedMutableHandleBase<T> &operator =(
        const TypedMutableHandleBase<T> &hBase)
    {
        rootBase_.set(hBase.get());
    }
};

template <typename T>
class PointerMutableHandleBase : public TypedMutableHandleBase<T *>
{
  public:
    PointerMutableHandleBase(TypedRootBase<T *> &base)
      : TypedMutableHandleBase<T *>(base)
    {}

    T *operator ->() const {
        return this->rootBase_.get();
    }
};

template <typename T>
inline TypedRootBase<T> &
TypedRootBase<T>::operator =(const TypedHandleBase<T> &other)
{
    thing_ = other.get();
}

template <typename T>
inline TypedRootBase<T> &
TypedRootBase<T>::operator =(const TypedMutableHandleBase<T> &other)
{
    thing_ = other.get();
}


//
// Root-wrapped Value
//
template <>
class Root<Value> : public TypedRootBase<Value>
{
    Root(RunContext *cx)
      : TypedRootBase<Value>(cx, RootKind::Value)
    {}

    Root(ThreadContext *cx)
      : TypedRootBase<Value>(cx, RootKind::Value)
    {}

    Root(RunContext *cx, const Value &val)
      : TypedRootBase<Value>(cx, RootKind::Value, val)
    {}

    Root(ThreadContext *cx, const Value &val)
      : TypedRootBase<Value>(cx, RootKind::Value, val)
    {}

#if defined(ENABLE_DEBUG)
    bool isValid() const {
        return thing_.isValid();
    }
#endif // defined(ENABLE_DEBUG)

    //
    // Checker methods
    //

    bool isObject() const {
        return thing_.isObject();
    }

    bool isNativeObject() const {
        return thing_.isNativeObject();
    }

    bool isForeignObject() const {
        return thing_.isForeignObject();
    }

    bool isSpecialObject() const {
        return thing_.isSpecialObject();
    }

    bool isNull() const {
        return thing_.isNull();
    }

    bool isUndefined() const {
        return thing_.isUndefined();
    }

    bool isBoolean() const {
        return thing_.isBoolean();
    }

    bool isHeapString() const {
        return thing_.isHeapString();
    }

    bool isImmString8() const {
        return thing_.isImmString8();
    }

    bool isImmString16() const {
        return thing_.isImmString16();
    }

    bool isImmDoubleLow() const {
        return thing_.isImmDoubleLow();
    }

    bool isImmDoubleHigh() const {
        return thing_.isImmDoubleHigh();
    }

    bool isImmDoubleX() const {
        return thing_.isImmDoubleX();
    }

    bool isNegZero() const {
        return thing_.isNegZero();
    }

    bool isNaN() const {
        return thing_.isNaN();
    }

    bool isPosInf() const {
        return thing_.isPosInf();
    }

    bool isNegInf() const {
        return thing_.isNegInf();
    }

    bool isHeapDouble() const {
        return thing_.isHeapDouble();
    }

    bool isInt32() const {
        return thing_.isInt32();
    }

    bool isMagic() const {
        return thing_.isMagic();
    }

    // Helper functions to check combined types.

    bool isString() const {
        return thing_.isString();
    }

    bool isImmString() const {
        return thing_.isImmString();
    }

    bool isNumber() const {
        return thing_.isNumber();
    }

    bool isDouble() const {
        return thing_.isDouble();
    }

    bool isSpecialImmDouble() const {
        return thing_.isSpecialImmDouble();
    }

    bool isRegularImmDouble() const {
        return thing_.isRegularImmDouble();
    }


    //
    // Getter methods
    //

    VM::Object *getObject() const {
        return thing_.getObject();
    }

    template <typename T>
    T *getForeignObject() const {
        return thing_.getForeignObject<T>();
    }

    template <typename T>
    T *getSpecialObject() const {
        return thing_.getSpecialObject<T>();
    }

    bool getBoolean() const {
        return thing_.getBoolean();
    }

    VM::HeapString *getHeapString() const {
        return thing_.getHeapString();
    }

    unsigned immString8Length() const {
        return thing_.immString8Length();
    }

    uint8_t getImmString8Char(unsigned idx) const {
        return thing_.getImmString8Char(idx);
    }

    template <typename CharT>
    unsigned readImmString8(CharT *buf) const {
        return thing_.readImmString8<CharT>(buf);
    }

    unsigned immString16Length() const {
        return thing_.immString16Length();
    }

    uint16_t getImmString16Char(unsigned idx) const {
        return thing_.getImmString16Char(idx);
    }

    template <typename CharT>
    unsigned readImmString16(CharT *buf) const {
        return thing_.readImmString16<CharT>(buf);
    }

    unsigned immStringLength() const {
        return thing_.immStringLength();
    }

    uint16_t getImmStringChar(unsigned idx) const {
        return thing_.getImmStringChar(idx);
    }

    template <typename CharT>
    unsigned readImmString(CharT *buf) const {
        return thing_.readImmString<CharT>(buf);
    }

    double getImmDoubleHiLoValue() const {
        return thing_.getImmDoubleHiLoValue();
    }

    double getImmDoubleXValue() const {
        return thing_.getImmDoubleXValue();
    }

    double getImmDoubleValue() const {
        return thing_.getImmDoubleValue();
    }

    VM::HeapDouble *getHeapDouble() const {
        return thing_.getHeapDouble();
    }

    Magic getMagic() const {
        return thing_.getMagic();
    }

    int32_t getInt32() const {
        return thing_.getInt32();
    }
};


} // namespace Whisper

#endif // WHISPER__ROOTING_HPP
