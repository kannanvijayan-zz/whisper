#ifndef WHISPER__ROOTING_HPP
#define WHISPER__ROOTING_HPP

#include "common.hpp"
#include "debug.hpp"
#include "value.hpp"

namespace Whisper {

class RunContext;
class ThreadContext;

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

    RootBase(ThreadContext *threadContext, RootKind kind);

    void postInit();

  public:
    ThreadContext *threadContext() const;

    RootBase *next() const;

    RootKind kind() const;
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
                         const T &thing);

    inline TypedRootBase(ThreadContext *threadContext, RootKind kind);

    inline TypedRootBase(RunContext *runContext, RootKind kind,
                         const T &thing);

    inline TypedRootBase(RunContext *runContext, RootKind kind);

    inline const T &get() const;
    inline T &get();

    inline void set(const T &val);

    inline operator const T &() const;
    inline operator T &();

    inline TypedRootBase<T> &operator =(const T &other);
    inline TypedRootBase<T> &operator =(const TypedRootBase<T> &other);
    inline TypedRootBase<T> &operator =(const TypedHandleBase<T> &other);
    inline TypedRootBase<T> &operator =(const TypedMutableHandleBase<T> &other);
};

template <typename T>
class PointerRootBase : public TypedRootBase<T *>
{
  public:
    inline PointerRootBase(ThreadContext *threadContext, RootKind kind);
    inline PointerRootBase(ThreadContext *threadContext, RootKind kind, T *ptr);
    inline T *operator ->() const;
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
    inline TypedHandleBase(TypedRootBase<T> &rootBase);
    inline const T &get() const;
    inline operator const T &() const;
};

template <typename T>
class PointerHandleBase : public TypedHandleBase<T *>
{
  public:
    inline PointerHandleBase(TypedRootBase<T *> &base);
    inline T *operator ->() const;
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
    inline TypedMutableHandleBase(TypedRootBase<T> &rootBase);
    inline T &get() const;
    inline void set(const T &t) const;
    inline operator T &() const;

    inline TypedMutableHandleBase<T> &
    operator =(const TypedRootBase<T> &rootBase);

    inline TypedMutableHandleBase<T> &
    operator =(const TypedHandleBase<T> &hBase);

    inline TypedMutableHandleBase<T> &
    operator =(const TypedMutableHandleBase<T> &hBase);
};

template <typename T>
class PointerMutableHandleBase : public TypedMutableHandleBase<T *>
{
  public:
    inline PointerMutableHandleBase(TypedRootBase<T *> &base);
    inline T *operator ->() const;
};


//
// Root-wrapped Value
//
template <>
class Root<Value> : public TypedRootBase<Value>
{
    Root(RunContext *cx);
    Root(ThreadContext *cx);
    Root(RunContext *cx, const Value &val);
    Root(ThreadContext *cx, const Value &val);

#if defined(ENABLE_DEBUG)
    bool isValid() const;
#endif // defined(ENABLE_DEBUG)

    //
    // Checker methods
    //

    bool isObject() const;
    bool isNativeObject() const;

    template <typename T>
    inline bool isNativeObjectOf() const;

    bool isForeignObject() const;
    bool isNull() const;
    bool isUndefined() const;
    bool isBoolean() const;
    bool isHeapString() const;
    bool isImmString8() const;
    bool isImmString16() const;
    bool isImmDoubleLow() const;
    bool isImmDoubleHigh() const;
    bool isImmDoubleX() const;
    bool isNegZero() const;
    bool isNaN() const;
    bool isPosInf() const;
    bool isNegInf() const;
    bool isHeapDouble() const;
    bool isInt32() const;
    bool isMagic() const;

    // Helper functions to check combined types.
    bool isString() const;
    bool isImmString() const;
    bool isNumber() const;
    bool isDouble() const;
    bool isSpecialImmDouble() const;
    bool isRegularImmDouble() const;

    //
    // Getter methods
    //
    template <typename T=VM::Object>
    T *getNativeObject() const;

    template <typename T>
    T *getForeignObject() const;

    bool getBoolean() const;

    VM::HeapString *getHeapString() const;
    unsigned immString8Length() const;
    uint8_t getImmString8Char(unsigned idx) const;

    template <typename CharT>
    unsigned readImmString8(CharT *buf) const;

    unsigned immString16Length() const;
    uint16_t getImmString16Char(unsigned idx) const;

    template <typename CharT>
    unsigned readImmString16(CharT *buf) const;

    unsigned immStringLength() const;
    uint16_t getImmStringChar(unsigned idx) const;

    template <typename CharT>
    unsigned readImmString(CharT *buf) const;

    double getImmDoubleHiLoValue() const;
    double getImmDoubleXValue() const;
    double getImmDoubleValue() const;
    VM::HeapDouble *getHeapDouble() const;
    Magic getMagic() const;
    int32_t getInt32() const;
};


} // namespace Whisper

#endif // WHISPER__ROOTING_HPP
