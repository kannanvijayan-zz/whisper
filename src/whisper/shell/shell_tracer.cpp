
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
    std::unordered_set<HeapThing *> *seen;
    std::list<HeapThing *> *remaining;
    StackThing *node;
    TracerVisitor *visitor;

    StackTracer(std::unordered_set<HeapThing *> *seen,
                std::list<HeapThing *> *remaining,
                StackThing *node, TracerVisitor *visitor)
      : seen(seen), remaining(remaining), node(node), visitor(visitor)
    {}

    inline void operator () (const void *addr, HeapThing *ptr) {
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
    std::unordered_set<HeapThing *> *seen;
    std::list<HeapThing *> *remaining;
    HeapThing *node;
    TracerVisitor *visitor;

    HeapTracer(std::unordered_set<HeapThing *> *seen,
               std::list<HeapThing *> *remaining,
               HeapThing *node, TracerVisitor *visitor)
      : seen(seen), remaining(remaining), node(node), visitor(visitor)
    {}

    inline void operator () (const void *addr, HeapThing *ptr) {
        if (seen->find(ptr) == seen->end()) {
            visitor->visitHeapThing(ptr);
            seen->insert(ptr);
            remaining->push_back(ptr);
        }
        visitor->visitHeapChild(node, ptr);
    }
};

void
trace_heap(ThreadContext *cx, TracerVisitor *visitor)
{
    // Set of seen heap-things.
    std::unordered_set<HeapThing *> seen;

    // List of heap-things left to visit in queue.
    std::list<HeapThing *> remaining;

    // Add unrooted heap things hanging directly off of cx.
    if (cx->hasLastFrame()) {
        HeapThing *lastFrameThing = HeapThing::From(cx->lastFrame());
        visitor->visitHeapThing(lastFrameThing);
        remaining.push_back(lastFrameThing);
        seen.insert(lastFrameThing);
    }
    if (cx->hasGlobal()) {
        HeapThing *globalThing = HeapThing::From(cx->global());
        visitor->visitHeapThing(globalThing);
        remaining.push_back(globalThing);
        seen.insert(globalThing);
    }

    // Visit all stack things.
    for (LocalBase *loc = cx->locals(); loc != nullptr; loc = loc->next()) {
        StackThing *stackThing = loc->stackThing();
        visitor->visitStackRoot(stackThing);

        StackTracer tracer(&seen, &remaining, stackThing, visitor);
        GC::ScanStackThing(tracer, stackThing, nullptr, nullptr);
    }

    // Process heap thing queue.
    while (!remaining.empty()) {
        HeapThing *heapThing = remaining.front();
        remaining.pop_front();

        HeapTracer tracer(&seen, &remaining, heapThing, visitor);
        GC::ScanHeapThing(tracer, heapThing, nullptr, nullptr);
    }
}
