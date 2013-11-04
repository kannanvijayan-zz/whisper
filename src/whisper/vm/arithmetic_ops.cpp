
#include <cmath>

#include "common.hpp"
#include "spew.hpp"
#include "runtime.hpp"
#include "runtime_inlines.hpp"
#include "rooting_inlines.hpp"
#include "vm/heap_thing_inlines.hpp"
#include "vm/stack_frame.hpp"
#include "vm/arithmetic_ops.hpp"

namespace Whisper {
namespace VM {

template <typename T>
static bool
SetOutputAndReturn(MutHandle<T> out, const T &value)
{
    out = value;
    return true;
}


bool
PerformAdd(RunContext *cx, Handle<Value> lhs, Handle<Value> rhs,
           MutHandle<Value> out)
{
    if (lhs->isInt32() && rhs->isInt32()) {
        int32_t lhsVal = lhs->int32Value();
        int32_t rhsVal = rhs->int32Value();
        int32_t resultVal = lhsVal + rhsVal;

        // First case: Add two positive ints yielding positive int.
        // Second case: Overflow not possible if sign(lhs) != sign(rhs).
        // Third case: Add two negative ints yielding negative int.
        if (((lhsVal >= 0) && (rhsVal >= 0) && (resultVal >= 0)) ||
            ((lhsVal >= 0 && rhsVal < 0) || (lhsVal < 0 && rhsVal >= 0)) ||
            ((lhsVal < 0) && (rhsVal < 0) && (resultVal < 0)))
        {
            return SetOutputAndReturn(out, Value::Int32(resultVal));
        }

        // None of the above, so it must be overflow.
    }

    if (lhs->isNumber() && rhs->isNumber()) {
        double lhsVal = lhs->numberValue();
        double rhsVal = rhs->numberValue();

        if (DoubleIsNaN(lhsVal) || DoubleIsNaN(rhsVal))
            return SetOutputAndReturn(out, Value::NaN());

        Root<Value> result(cx);
        if (!cx->inHatchery().createNumber(lhsVal + rhsVal, &result))
            return false;

        return SetOutputAndReturn(out, result.get());
    }

    WH_UNREACHABLE("Non-int32 add not implemented yet!");
    return false;
}


bool
PerformSub(RunContext *cx, Handle<Value> lhs, Handle<Value> rhs,
           MutHandle<Value> out)
{
    if (lhs->isInt32() && rhs->isInt32()) {
        int32_t lhsVal = lhs->int32Value();
        int32_t rhsVal = rhs->int32Value();
        int32_t resultVal = lhsVal - rhsVal;

        // |lhs - rhs| will not overflow if:
        //   lhs >= 0 and rhsVal >= 0 - worst case: 0 - INT_MAX == INT_MIN + 1
        //   lhs < 0 and rhsVal < 0 - worst case: -1 - INT_MIN == INT_MAX
        //
        // Otherwise, if sign of |lhsVal| and |rhsVal| are different,
        // we have |(-x) - y| or |x - (-y)|.
        // If an overflow occurrs, the result will have opposite sign
        // from |lhsVal|.  In the most extreme case, we have
        //   INT32_MIN - INT32_MAX == INT32_MAX - (INT32_MAX-1) == 1
        // or
        //   INT32_MAX - INT32_MIN == INT32_MIN - (INT32_MIN+1) == -1
        if (((lhsVal >= 0 && rhsVal >= 0) || (lhsVal < 0 && rhsVal < 0)) ||
            ((resultVal >= 0) == (lhsVal >= 0)))
        {   
            return SetOutputAndReturn(out, Value::Int32(resultVal));
        }
    }

    if (lhs->isNumber() && rhs->isNumber()) {
        double lhsVal = lhs->numberValue();
        double rhsVal = rhs->numberValue();

        if (DoubleIsNaN(lhsVal) || DoubleIsNaN(rhsVal))
            return SetOutputAndReturn(out, Value::NaN());

        Root<Value> result(cx);
        if (!cx->inHatchery().createNumber(lhsVal - rhsVal, &result))
            return false;

        return SetOutputAndReturn(out, result.get());
    }

    WH_UNREACHABLE("Non-number subtract not implemented yet!");
    return false;
}


static unsigned
NumSignificantBits(int32_t val)
{
    unsigned result = 0;
    while (val != 0 && val != -1) {
        result += 1;
        val >>= 1;
    }
    // one extra bit needed to represent sign.
    result += 1;
    return result;
}


bool
PerformMul(RunContext *cx, Handle<Value> lhs, Handle<Value> rhs,
           MutHandle<Value> out)
{
    if (lhs->isInt32() && rhs->isInt32()) {
        int32_t lhsVal = lhs->int32Value();
        int32_t rhsVal = rhs->int32Value();

        // Check number of significant bits in each number.
        unsigned lhsBits = NumSignificantBits(lhsVal);
        unsigned rhsBits = NumSignificantBits(rhsVal);

        // Do int32 multiply only if overflow not possible.
        if (lhsBits + rhsBits < 31)
            return SetOutputAndReturn(out, Value::Int32(lhsVal * rhsVal));
    }

    if (lhs->isNumber() && rhs->isNumber()) {
        double lhsVal = lhs->numberValue();
        double rhsVal = rhs->numberValue();

        if (DoubleIsNaN(lhsVal) || DoubleIsNaN(rhsVal))
            return SetOutputAndReturn(out, Value::NaN());

        Root<Value> result(cx);
        if (!cx->inHatchery().createNumber(lhsVal * rhsVal, &result))
            return false;

        return SetOutputAndReturn(out, result.get());
    }

    WH_UNREACHABLE("Non-number multiply not implemented yet!");
    return false;
}


bool
PerformDiv(RunContext *cx, Handle<Value> lhs, Handle<Value> rhs,
           MutHandle<Value> out)
{
    if (lhs->isInt32() && rhs->isInt32()) {
        int32_t lhsVal = lhs->int32Value();
        int32_t rhsVal = rhs->int32Value();

        if (rhsVal == 0) {
            // +N/0 is +Inf
            if (lhsVal > 0)
                return SetOutputAndReturn(out, Value::PosInf());

            // -N/0 is -Inf
            if (lhsVal < 0)
                return SetOutputAndReturn(out, Value::NegInf());

            // 0/0 is NaN
            return SetOutputAndReturn(out, Value::NaN());
        }

        if (lhsVal % rhsVal == 0)
            return SetOutputAndReturn(out, Value::Int32(lhsVal / rhsVal));
    }

    if (lhs->isNumber() && rhs->isNumber()) {
        double lhsVal = lhs->numberValue();
        double rhsVal = rhs->numberValue();

        if (DoubleIsNaN(lhsVal) || DoubleIsNaN(rhsVal))
            return SetOutputAndReturn(out, Value::NaN());

        if (rhsVal == 0.0f) {
            bool rhsIsNeg = GetDoubleSign(rhsVal);

            if (lhsVal == 0.0f)
                return SetOutputAndReturn(out, Value::NaN());

            if (lhsVal < 0.0f) {
                if (rhsIsNeg)
                    return SetOutputAndReturn(out, Value::PosInf());

                return SetOutputAndReturn(out, Value::NegInf());
            }

            if (rhsIsNeg)
                return SetOutputAndReturn(out, Value::NegInf());

            return SetOutputAndReturn(out, Value::PosInf());
        }

        Root<Value> result(cx);
        if (!cx->inHatchery().createNumber(lhsVal / rhsVal, &result))
            return false;

        return SetOutputAndReturn(out, result.get());
    }

    WH_UNREACHABLE("Non-number divide not implemented yet!");
    return false;
}


bool
PerformMod(RunContext *cx, Handle<Value> lhs, Handle<Value> rhs,
           MutHandle<Value> out)
{
    if (lhs->isInt32() && rhs->isInt32()) {
        int32_t lhsVal = lhs->int32Value();
        int32_t rhsVal = rhs->int32Value();
        if (lhsVal >= 0 && rhsVal >= 0)
            return SetOutputAndReturn(out, Value::Int32(lhsVal % rhsVal));
    }

    if (lhs->isNumber() && rhs->isNumber()) {
        int32_t lhsVal = lhs->numberValue();
        int32_t rhsVal = rhs->numberValue();

        Root<Value> result(cx);
        if (!cx->inHatchery().createNumber(fmod(lhsVal, rhsVal), &result))
            return false;

        return SetOutputAndReturn(out, result.get());
    }

    WH_UNREACHABLE("Non-number modulo not implemented yet!");
    return false;
}


bool
PerformNeg(RunContext *cx, Handle<Value> in, MutHandle<Value> out)
{
    if (in->isInt32()) {
        int32_t inVal = in->int32Value();

        bool overflow = (inVal == INT32_MIN);
        if (!overflow)
            return SetOutputAndReturn(out, Value::Int32(-inVal));
    }

    WH_UNREACHABLE("Non-int32 negate not implemented yet!");
    return false;
}


} // namespace VM
} // namespace Whisper
