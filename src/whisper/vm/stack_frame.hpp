#ifndef WHISPER__VM__STACK_FRAME_HPP
#define WHISPER__VM__STACK_FRAME_HPP

#include "common.hpp"
#include "debug.hpp"
#include "value.hpp"
#include "vm/heap_thing.hpp"
#include "vm/script.hpp"

namespace Whisper {
namespace VM {


//
// A StackFrame represents a traced, heap-allocated stack frame object
// used by the C++ interpreter.
//
//      +-----------------------+
//      | Header                |
//      +-----------------------+
//      | CallerFrame           |
//      | Callee                |
//      | PcOffset              |
//      | NumPassedArgs         |
//      | NumArgs               |
//      | NumLocals             |
//      | StackDepth            |
//      +-----------------------+
//      | ArgVal                |
//      | ...                   |
//      | ArgVal                |
//      +-----------------------+
//      | LocalVal              |
//      | ...                   |
//      | LocalVal              |
//      +-----------------------+
//      | StackVal              |
//      | ...                   |
//      | StackVal              |
//      +-----------------------+
//
// CallerFrame - points to the caller StackFrame.  For the initial stack
//  frame, this is null.
//
// Callee - the Script or function object that's executing in this frame.
//
struct StackFrame : public HeapThing,
                    public TypedHeapThing<HeapType::StackFrame>
{
  public:
    struct Config
    {
        uint32_t numPassedArgs;
        uint32_t numArgs;
        uint32_t numLocals;
        uint32_t maxStackDepth;
    };

    static uint32_t CalculateSize(const Config &config);

  private:
    // Pointer to caller frame.
    Heap<StackFrame *> callerFrame_;

    // Either Script or Function executing in this frame.
    Heap<HeapThing *> callee_;

    // The current bytecode pc offset.
    uint32_t pcOffset_;

    // The number of actual arguments provided by the caller.
    uint32_t numPassedArgs_;

    // The number of arguments on frame == max(actualArgs_, formalArgs_)
    uint32_t numArgs_;

    // The number of locals.
    uint32_t numLocals_;

    // The current stack depth.
    uint32_t stackDepth_;

  public:
    StackFrame(Script *callee, const Config &config);

    bool hasCallerFrame() const;
    Handle<StackFrame *> callerFrame() const;

    bool isScriptFrame() const;
    Handle<Script *> script() const;

    bool isTopLevelFrame() const;

    uint32_t pcOffset() const;
    void setPcOffset(uint32_t newPcOffset);
    uint32_t numPassedArgs() const;
    uint32_t numArgs() const;
    uint32_t numLocals() const;
    uint32_t stackDepth() const;
    uint32_t maxStackDepth() const;

    Handle<Value> getArg(uint32_t idx) const;
    void setArg(uint32_t idx, const Value &val);

    Handle<Value> getLocal(uint32_t idx) const;
    void setLocal(uint32_t idx, const Value &val);

    Handle<Value> getStack(uint32_t offset) const;
    void setStack(uint32_t offset, const Value &val);

    void popStack(uint32_t count = 1);
    void pushStack(const Value &val);

    Handle<Value> peekStack(uint32_t offset) const;
    void pokeStack(uint32_t offset, const Value &val);

  private:
    const Value *argStart() const;
    Value *argStart();

    const Value *localStart() const;
    Value *localStart();

    const Value *stackStart() const;
    Value *stackStart();

    const Value &argAt(uint32_t idx) const;
    Value &argAt(uint32_t idx);

    const Value &localAt(uint32_t idx) const;
    Value &localAt(uint32_t idx);

    const Value &stackAt(uint32_t idx) const;
    Value &stackAt(uint32_t idx);

    const Heap<Value> &argRef(uint32_t idx) const;
    Heap<Value> &argRef(uint32_t idx);

    const Heap<Value> &localRef(uint32_t idx) const;
    Heap<Value> &localRef(uint32_t idx);

    const Heap<Value> &stackRef(uint32_t idx) const;
    Heap<Value> &stackRef(uint32_t idx);
};


} // namespace VM
} // namespace Whisper

#endif // WHISPER__VM__STACK_FRAME_HPP
