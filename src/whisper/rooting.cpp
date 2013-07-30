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
  : TypedRootBase<Value>(cx, RootKind::Value)
{}

Root<Value>::Root(ThreadContext *cx)
  : TypedRootBase<Value>(cx, RootKind::Value)
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
Root<Value>::isImmDoubleLow() const
{
    return thing_.isImmDoubleLow();
}

bool
Root<Value>::isImmDoubleHigh() const
{
    return thing_.isImmDoubleHigh();
}

bool
Root<Value>::isImmDoubleX() const
{
    return thing_.isImmDoubleX();
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
Root<Value>::isSpecialImmDouble() const
{
    return thing_.isSpecialImmDouble();
}

bool
Root<Value>::isRegularImmDouble() const
{
    return thing_.isRegularImmDouble();
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
Root<Value>::getImmDoubleHiLoValue() const
{
    return thing_.getImmDoubleHiLoValue();
}

double
Root<Value>::getImmDoubleXValue() const
{
    return thing_.getImmDoubleXValue();
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

Magic
Root<Value>::getMagic() const
{
    return thing_.getMagic();
}

int32_t
Root<Value>::getInt32() const
{
    return thing_.getInt32();
}


} // namespace Whisper
