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

    inline RootBase(ThreadContext *threadContext, RootKind kind)
      : threadContext_(threadContext),
        next_(threadContext->roots()),
        kind_(kind)
    {}

    inline void postInit() {
        threadContext_->roots_ = this;
    }

  public:
    inline ThreadContext *threadContext() const {
        return threadContext_;
    }
    inline RootBase *next() const {
        return next_;
    }
    inline RootKind kind() const {
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
    inline TypedRootBase(ThreadContext *threadContext, RootKind kind,
                         const T &thing)
      : RootBase(threadContext, kind),
        thing_(thing)
    {
        postInit();
    }

    inline TypedRootBase(ThreadContext *threadContext, RootKind kind)
      : RootBase(threadContext, kind),
        thing_()
    {
        postInit();
    }

    inline TypedRootBase(RunContext *runContext, RootKind kind,
                         const T &thing)
      : TypedRootBase(runContext->threadContext(), kind, thing)
    {}

    inline TypedRootBase(RunContext *runContext, RootKind kind)
      : TypedRootBase(runContext->threadContext(), kind)
    {}

    inline const T &get() const {
        return thing_;
    }
    inline T &get() {
        return thing_;
    }

    inline void set(const T &val) {
        thing_ = val;
    }

    inline operator const T &() const {
        return thing_;
    }
    inline operator T &() {
        return thing_;
    }

    inline TypedRootBase<T> &operator =(const T &other) {
        thing_ = other;
        return *this;
    }
    inline TypedRootBase<T> &operator =(const TypedRootBase<T> &other) {
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
    inline PointerRootBase(ThreadContext *threadContext, RootKind kind)
      : TypedRootBase<T *>(threadContext, kind, nullptr)
    {}

    inline PointerRootBase(ThreadContext *threadContext, RootKind kind,
                           T *ptr)
      : TypedRootBase<T *>(threadContext, kind, ptr)
    {}

    inline T *operator ->() const {
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

    inline T *operator ->() const {
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

    inline T *operator ->() const {
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
    inline bool isValid() const {
        return thing_.isValid();
    }
#endif // defined(ENABLE_DEBUG)

    //
    // Checker methods
    //

    inline bool isObject() const {
        return thing_.isObject();
    }

    inline bool isNativeObject() const {
        return thing_.isNativeObject();
    }

    inline bool isForeignObject() const {
        return thing_.isForeignObject();
    }

    inline bool isSpecialObject() const {
        return thing_.isSpecialObject();
    }

    inline bool isNull() const {
        return thing_.isNull();
    }

    inline bool isUndefined() const {
        return thing_.isUndefined();
    }

    inline bool isBoolean() const {
        return thing_.isBoolean();
    }

    inline bool isHeapString() const {
        return thing_.isHeapString();
    }

    inline bool isImmString8() const {
        return thing_.isImmString8();
    }

    inline bool isImmString16() const {
        return thing_.isImmString16();
    }

    inline bool isImmDoubleLow() const {
        return thing_.isImmDoubleLow();
    }

    inline bool isImmDoubleHigh() const {
        return thing_.isImmDoubleHigh();
    }

    inline bool isImmDoubleX() const {
        return thing_.isImmDoubleX();
    }

    inline bool isNegZero() const {
        return thing_.isNegZero();
    }

    inline bool isNaN() const {
        return thing_.isNaN();
    }

    inline bool isPosInf() const {
        return thing_.isPosInf();
    }

    inline bool isNegInf() const {
        return thing_.isNegInf();
    }

    inline bool isHeapDouble() const {
        return thing_.isHeapDouble();
    }

    inline bool isInt32() const {
        return thing_.isInt32();
    }

    inline bool isMagic() const {
        return thing_.isMagic();
    }

    // Helper functions to check combined types.

    inline bool isString() const {
        return thing_.isString();
    }

    inline bool isImmString() const {
        return thing_.isImmString();
    }

    inline bool isNumber() const {
        return thing_.isNumber();
    }

    inline bool isDouble() const {
        return thing_.isDouble();
    }

    inline bool isSpecialImmDouble() const {
        return thing_.isSpecialImmDouble();
    }

    inline bool isRegularImmDouble() const {
        return thing_.isRegularImmDouble();
    }


    //
    // Getter methods
    //

    inline Object *getObject() const {
        return thing_.getObject();
    }

    template <typename T>
    inline T *getForeignObject() const {
        return thing_.getForeignObject<T>();
    }

    template <typename T>
    inline T *getSpecialObject() const {
        return thing_.getSpecialObject<T>();
    }

    inline bool getBoolean() const {
        return thing_.getBoolean();
    }

    inline HeapString *getHeapString() const {
        return thing_.getHeapString();
    }

    inline unsigned immString8Length() const {
        return thing_.immString8Length();
    }

    inline uint8_t getImmString8Char(unsigned idx) const {
        return thing_.getImmString8Char(idx);
    }

    template <typename CharT>
    inline unsigned readImmString8(CharT *buf) const {
        return thing_.readImmString8<CharT>(buf);
    }

    inline unsigned immString16Length() const {
        return thing_.immString16Length();
    }

    inline uint16_t getImmString16Char(unsigned idx) const {
        return thing_.getImmString16Char(idx);
    }

    template <typename CharT>
    inline unsigned readImmString16(CharT *buf) const {
        return thing_.readImmString16<CharT>(buf);
    }

    inline unsigned immStringLength() const {
        return thing_.immStringLength();
    }

    inline uint16_t getImmStringChar(unsigned idx) const {
        return thing_.getImmStringChar(idx);
    }

    template <typename CharT>
    inline unsigned readImmString(CharT *buf) const {
        return thing_.readImmString<CharT>(buf);
    }

    inline double getImmDoubleHiLoValue() const {
        return thing_.getImmDoubleHiLoValue();
    }

    inline double getImmDoubleXValue() const {
        return thing_.getImmDoubleXValue();
    }

    inline double getImmDoubleValue() const {
        return thing_.getImmDoubleValue();
    }

    inline HeapDouble *getHeapDouble() const {
        return thing_.getHeapDouble();
    }

    inline Magic getMagic() const {
        return thing_.getMagic();
    }

    inline int32_t getInt32() const {
        return thing_.getInt32();
    }
};


} // namespace Whisper

#endif // WHISPER__ROOTING_HPP
