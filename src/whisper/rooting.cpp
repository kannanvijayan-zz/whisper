#include "rooting.hpp"
#include "rooting_inlines.hpp"

namespace Whisper {

//
// RootBase
//

RootBase::RootBase(ThreadContext *threadContext, RootKind kind)
  : threadContext_(threadContext),
    next_(threadContext->roots()),
    kind_(kind)
{}

void
RootBase::postInit()
{
    threadContext_->roots_ = this;
}

ThreadContext *
RootBase::threadContext() const
{
    return threadContext_;
}

RootBase *
RootBase::next() const
{
    return next_;
}

RootKind
RootBase::kind() const
{
    return kind_;
}

//
// Root<Value>
//

Root<Value>::Root(RunContext *cx)
  : TypedRootBase<Value>(cx, RootKind::Value, UndefinedValue())
{}

Root<Value>::Root(ThreadContext *cx)
  : TypedRootBase<Value>(cx, RootKind::Value, UndefinedValue())
{}

Root<Value>::Root(RunContext *cx, const Value &val)
  : TypedRootBase<Value>(cx, RootKind::Value, val)
{}

Root<Value>::Root(ThreadContext *cx, const Value &val)
  : TypedRootBase<Value>(cx, RootKind::Value, val)
{}

#if defined(ENABLE_DEBUG)
bool
Root<Value>::isValid() const
{
    return thing_.isValid();
}
#endif // defined(ENABLE_DEBUG)

bool
Root<Value>::isObject() const
{
    return thing_.isObject();
}

bool
Root<Value>::isNativeObject() const
{
    return thing_.isNativeObject();
}

bool
Root<Value>::isForeignObject() const
{
    return thing_.isForeignObject();
}

bool
Root<Value>::isNull() const
{
    return thing_.isNull();
}

bool
Root<Value>::isUndefined() const
{
    return thing_.isUndefined();
}

bool
Root<Value>::isBoolean() const
{
    return thing_.isBoolean();
}

bool
Root<Value>::isHeapString() const
{
    return thing_.isHeapString();
}

bool
Root<Value>::isImmString8() const
{
    return thing_.isImmString8();
}

bool
Root<Value>::isImmString16() const
{
    return thing_.isImmString16();
}

bool
Root<Value>::isPosImmDoubleSmall() const
{
    return thing_.isPosImmDoubleSmall();
}

bool
Root<Value>::isPosImmDoubleBig() const
{
    return thing_.isPosImmDoubleBig();
}

bool
Root<Value>::isNegImmDoubleSmall() const
{
    return thing_.isNegImmDoubleSmall();
}

bool
Root<Value>::isNegImmDoubleBig() const
{
    return thing_.isNegImmDoubleBig();
}

bool
Root<Value>::isRegularImmDouble() const
{
    return thing_.isRegularImmDouble();
}

bool
Root<Value>::isSpecialImmDouble() const
{
    return thing_.isSpecialImmDouble();
}

bool
Root<Value>::isNegZero() const
{
    return thing_.isNegZero();
}

bool
Root<Value>::isNaN() const
{
    return thing_.isNaN();
}

bool
Root<Value>::isPosInf() const
{
    return thing_.isPosInf();
}

bool
Root<Value>::isNegInf() const
{
    return thing_.isNegInf();
}

bool
Root<Value>::isHeapDouble() const
{
    return thing_.isHeapDouble();
}

bool
Root<Value>::isInt32() const
{
    return thing_.isInt32();
}

bool
Root<Value>::isMagic() const
{
    return thing_.isMagic();
}

bool
Root<Value>::isString() const
{
    return thing_.isString();
}

bool
Root<Value>::isImmString() const
{
    return thing_.isImmString();
}

bool
Root<Value>::isNumber() const
{
    return thing_.isNumber();
}

bool
Root<Value>::isDouble() const
{
    return thing_.isDouble();
}

bool
Root<Value>::isImmDouble() const
{
    return thing_.isImmDouble();
}

bool
Root<Value>::getBoolean() const
{
    return thing_.getBoolean();
}

VM::HeapString *
Root<Value>::getHeapString() const
{
    return thing_.getHeapString();
}

unsigned
Root<Value>::immString8Length() const
{
    return thing_.immString8Length();
}

uint8_t
Root<Value>::getImmString8Char(unsigned idx) const
{
    return thing_.getImmString8Char(idx);
}

unsigned
Root<Value>::immString16Length() const
{
    return thing_.immString16Length();
}

uint16_t
Root<Value>::getImmString16Char(unsigned idx) const
{
    return thing_.getImmString16Char(idx);
}

unsigned
Root<Value>::immStringLength() const
{
    return thing_.immStringLength();
}

uint16_t
Root<Value>::getImmStringChar(unsigned idx) const
{
    return thing_.getImmStringChar(idx);
}

double
Root<Value>::getRegularImmDoubleValue() const
{
    return thing_.getRegularImmDoubleValue();
}

double
Root<Value>::getSpecialImmDoubleValue() const
{
    return thing_.getSpecialImmDoubleValue();
}

double
Root<Value>::getImmDoubleValue() const
{
    return thing_.getImmDoubleValue();
}

VM::HeapDouble *
Root<Value>::getHeapDouble() const
{
    return thing_.getHeapDouble();
}

int32_t
Root<Value>::getInt32() const
{
    return thing_.getInt32();
}

//
// Handle<Value>
//

Handle<Value>::Handle(const Value &val)
  : TypedHandleBase<Value>(val)
{}

#if defined(ENABLE_DEBUG)
bool
Handle<Value>::isValid() const
{
    return ref_.isValid();
}
#endif // defined(ENABLE_DEBUG)

bool
Handle<Value>::isObject() const
{
    return ref_.isObject();
}

bool
Handle<Value>::isNativeObject() const
{
    return ref_.isNativeObject();
}

bool
Handle<Value>::isForeignObject() const
{
    return ref_.isForeignObject();
}

bool
Handle<Value>::isNull() const
{
    return ref_.isNull();
}

bool
Handle<Value>::isUndefined() const
{
    return ref_.isUndefined();
}

bool
Handle<Value>::isBoolean() const
{
    return ref_.isBoolean();
}

bool
Handle<Value>::isHeapString() const
{
    return ref_.isHeapString();
}

bool
Handle<Value>::isImmString8() const
{
    return ref_.isImmString8();
}

bool
Handle<Value>::isImmString16() const
{
    return ref_.isImmString16();
}

bool
Handle<Value>::isPosImmDoubleSmall() const
{
    return ref_.isPosImmDoubleSmall();
}

bool
Handle<Value>::isPosImmDoubleBig() const
{
    return ref_.isPosImmDoubleBig();
}

bool
Handle<Value>::isNegImmDoubleSmall() const
{
    return ref_.isNegImmDoubleSmall();
}

bool
Handle<Value>::isNegImmDoubleBig() const
{
    return ref_.isNegImmDoubleBig();
}

bool
Handle<Value>::isRegularImmDouble() const
{
    return ref_.isRegularImmDouble();
}

bool
Handle<Value>::isSpecialImmDouble() const
{
    return ref_.isSpecialImmDouble();
}

bool
Handle<Value>::isNegZero() const
{
    return ref_.isNegZero();
}

bool
Handle<Value>::isNaN() const
{
    return ref_.isNaN();
}

bool
Handle<Value>::isPosInf() const
{
    return ref_.isPosInf();
}

bool
Handle<Value>::isNegInf() const
{
    return ref_.isNegInf();
}

bool
Handle<Value>::isHeapDouble() const
{
    return ref_.isHeapDouble();
}

bool
Handle<Value>::isInt32() const
{
    return ref_.isInt32();
}

bool
Handle<Value>::isMagic() const
{
    return ref_.isMagic();
}

bool
Handle<Value>::isString() const
{
    return ref_.isString();
}

bool
Handle<Value>::isImmString() const
{
    return ref_.isImmString();
}

bool
Handle<Value>::isNumber() const
{
    return ref_.isNumber();
}

bool
Handle<Value>::isDouble() const
{
    return ref_.isDouble();
}

bool
Handle<Value>::isImmDouble() const
{
    return ref_.isImmDouble();
}

bool
Handle<Value>::getBoolean() const
{
    return ref_.getBoolean();
}

VM::HeapString *
Handle<Value>::getHeapString() const
{
    return ref_.getHeapString();
}

unsigned
Handle<Value>::immString8Length() const
{
    return ref_.immString8Length();
}

uint8_t
Handle<Value>::getImmString8Char(unsigned idx) const
{
    return ref_.getImmString8Char(idx);
}

unsigned
Handle<Value>::immString16Length() const
{
    return ref_.immString16Length();
}

uint16_t
Handle<Value>::getImmString16Char(unsigned idx) const
{
    return ref_.getImmString16Char(idx);
}

unsigned
Handle<Value>::immStringLength() const
{
    return ref_.immStringLength();
}

uint16_t
Handle<Value>::getImmStringChar(unsigned idx) const
{
    return ref_.getImmStringChar(idx);
}

double
Handle<Value>::getRegularImmDoubleValue() const
{
    return ref_.getRegularImmDoubleValue();
}

double
Handle<Value>::getSpecialImmDoubleValue() const
{
    return ref_.getSpecialImmDoubleValue();
}

double
Handle<Value>::getImmDoubleValue() const
{
    return ref_.getImmDoubleValue();
}

VM::HeapDouble *
Handle<Value>::getHeapDouble() const
{
    return ref_.getHeapDouble();
}

int32_t
Handle<Value>::getInt32() const
{
    return ref_.getInt32();
}


} // namespace Whisper
