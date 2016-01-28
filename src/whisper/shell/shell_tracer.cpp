
#include <list>
#include <unordered_set>

#include "common.hpp"
#include "allocators.hpp"
#include "spew.hpp"
#include "runtime.hpp"
#include "runtime_inlines.hpp"
#include "gc.hpp"

#include "vm/predeclare.hpp"
#include "shell/shell_tracer.hpp"

using namespace Whisper;

struct StackTracer
{
    std::unordered_set<HeapThing*>* seen;
    std::list<HeapThing*>* remaining;
    StackThing* node;
    TracerVisitor* visitor;

    StackTracer(std::unordered_set<HeapThing*>* seen,
                std::list<HeapThing*>* remaining,
                StackThing* node, TracerVisitor* visitor)
      : seen(seen), remaining(remaining), node(node), visitor(visitor)
    {}

    inline void operator () (void const* addr, HeapThing* ptr) {
        if (seen->find(ptr) == seen->end()) {
            visitor->visitHeapThing(ptr);
            seen->insert(ptr);
            remaining->push_back(ptr);
        }
        visitor->visitStackChild(node, ptr);
    }
};

struct HeapTracer
{
    std::unordered_set<HeapThing*>* seen;
    std::list<HeapThing*>* remaining;
    HeapThing* node;
    TracerVisitor* visitor;

    HeapTracer(std::unordered_set<HeapThing*>* seen,
               std::list<HeapThing*>* remaining,
               HeapThing* node, TracerVisitor* visitor)
      : seen(seen), remaining(remaining), node(node), visitor(visitor)
    {}

    inline void operator () (void const* addr, HeapThing* ptr) {
        if (seen->find(ptr) == seen->end()) {
            visitor->visitHeapThing(ptr);
            seen->insert(ptr);
            remaining->push_back(ptr);
        }
        visitor->visitHeapChild(node, ptr);
    }
};

void
trace_heap(ThreadContext* cx, TracerVisitor* visitor)
{
    trace_heap(cx, visitor, nullptr);
}

void
trace_heap(ThreadContext* cx, TracerVisitor* visitor, VM::Frame* frame)
{
    // Set of seen heap-things.
    std::unordered_set<HeapThing*> seen;

    // List of heap-things left to visit in queue.
    std::list<HeapThing*> remaining;

    if (frame) {
        HeapThing* frameThing = HeapThing::From(frame);
        visitor->visitHeapThing(frameThing);
        remaining.push_back(frameThing);
        seen.insert(frameThing);
    }

    if (cx->hasThreadState()) {
        HeapThing* threadState = HeapThing::From(cx->threadState());
        visitor->visitHeapThing(threadState);
        remaining.push_back(threadState);
        seen.insert(threadState);
    }

    // Visit all stack things.
    for (LocalBase* loc = cx->locals(); loc != nullptr; loc = loc->next()) {
        for (uint32_t i = 0; i < loc->count(); i++) {
            StackThing* stackThing = loc->stackThing(i);
            visitor->visitStackRoot(stackThing, i);

            StackTracer tracer(&seen, &remaining, stackThing, visitor);
            GC::ScanStackThing(tracer, stackThing, nullptr, nullptr);
        }
    }

    // Process heap thing queue.
    while (!remaining.empty()) {
        HeapThing* heapThing = remaining.front();
        remaining.pop_front();

        HeapTracer tracer(&seen, &remaining, heapThing, visitor);
        GC::ScanHeapThing(tracer, heapThing, nullptr, nullptr);
    }
}
