#ifndef WHISPER__VM__ARITHMETIC_OPS
#define WHISPER__VM__ARITHMETIC_OPS

#include <cmath>

#include "common.hpp"
#include "rooting.hpp"

namespace Whisper {
namespace VM {


bool PerformAdd(RunContext *cx, Handle<Value> lhs, Handle<Value> rhs,
                MutHandle<Value> out);

bool PerformSub(RunContext *cx, Handle<Value> lhs, Handle<Value> rhs,
                MutHandle<Value> out);

bool PerformMul(RunContext *cx, Handle<Value> lhs, Handle<Value> rhs,
                MutHandle<Value> out);

bool PerformDiv(RunContext *cx, Handle<Value> lhs, Handle<Value> rhs,
                MutHandle<Value> out);

bool PerformMod(RunContext *cx, Handle<Value> lhs, Handle<Value> rhs,
                MutHandle<Value> out);

bool PerformNeg(RunContext *cx, Handle<Value> in, MutHandle<Value> out);


} // namespace VM
} // namespace Whisper

#endif // WHISPER__VM__ARITHMETIC_OPS
