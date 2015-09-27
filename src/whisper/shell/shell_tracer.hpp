#ifndef WHISPER__SHELL__TRACER_HPP
#define WHISPER__SHELL__TRACER_HPP

#include "common.hpp"
#include "debug.hpp"
#include "spew.hpp"
#include "gc.hpp"

//
// Define a virtual visitor class for the tracer.
//
class TracerVisitor
{
  protected:
    TracerVisitor() {}

  public:
    virtual void visitStackRoot(Whisper::StackThing* root, uint32_t idx) = 0;
    virtual void visitStackChild(Whisper::StackThing* holder,
                                 Whisper::HeapThing* child) = 0;

    virtual void visitHeapThing(Whisper::HeapThing* thing) = 0;
    virtual void visitHeapChild(Whisper::HeapThing* holder,
                                 Whisper::HeapThing* child) = 0;
};

//
// Inspect the heap using a visitor.
//
void trace_heap(Whisper::ThreadContext* cx, TracerVisitor* visitor);

#endif // WHISPER__SHELL__TRACER_HPP
