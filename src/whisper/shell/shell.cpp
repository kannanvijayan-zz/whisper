
#include <iostream>
#include <vector>
#include <map>
#include <set>

#include "common.hpp"
#include "allocators.hpp"
#include "fnv_hash.hpp"
#include "spew.hpp"
#include "runtime.hpp"
#include "runtime_inlines.hpp"
#include "gc.hpp"
#include "parser/code_source.hpp"
#include "parser/tokenizer.hpp"
#include "parser/syntax_tree_inlines.hpp"
#include "parser/parser.hpp"
#include "parser/packed_syntax.hpp"
#include "parser/packed_writer.hpp"
#include "parser/packed_reader.hpp"
#include "vm/array.hpp"
#include "vm/string.hpp"
#include "vm/box.hpp"
#include "vm/source_file.hpp"
#include "vm/packed_syntax_tree.hpp"

using namespace Whisper;


template <typename T> struct Node;
typedef Node<HeapThing> HeapNode;
typedef Node<StackThing> StackNode;

template <typename T>
struct Node {
    T *ptr;
    typedef std::vector<HeapNode *> ChildList;
    ChildList children;

    Node(T *ptr) : ptr(ptr), children() {}

    void addChild(HeapNode *child) {
        children.push_back(child);
    }
};


// HeapTracer calculates the rooted object graph.
struct HeapGraph {
    // Mapping from heapthings to graph nodes.
    typedef std::map<HeapThing *, HeapNode *> HeapToNodeMap;
    HeapToNodeMap heapToNode;

    // Set of things remaining to be scanned.
    typedef std::set<HeapNode *> RemainingSet;
    RemainingSet remaining;

    // List of root stack things.
    typedef std::vector<StackNode *> RootList;
    RootList rootList;

    HeapGraph() : heapToNode(), remaining(), rootList() {}

    StackNode *addRoot(StackThing *rootPtr) {
        StackNode *rootNode = new StackNode(rootPtr);
        rootList.push_back(rootNode);
        fprintf(stderr, "Add Root %p(%s)\n", rootPtr,
                StackFormatString(rootPtr->format()));
        return rootNode;
    }

    HeapNode *addChild(HeapNode *parentNode, HeapThing *child) {
        HeapNode *childNode = getOrAddHeapThing(child);
        parentNode->addChild(childNode);
        fprintf(stderr, "Add Heap %p(%s) --> %p(%s)\n",
                parentNode->ptr, HeapFormatString(parentNode->ptr->format()),
                child, HeapFormatString(child->format()));
        return childNode;
    }

    HeapNode *addChild(StackNode *stackNode, HeapThing *child) {
        HeapNode *childNode = getOrAddHeapThing(child);
        stackNode->addChild(childNode);
        fprintf(stderr, "Add Stack %p(%s) --> %p(%s)\n",
                stackNode->ptr, StackFormatString(stackNode->ptr->format()),
                child, HeapFormatString(child->format()));
        return childNode;
    }

    HeapNode *getOrAddHeapThing(HeapThing *heapThing) {
        HeapToNodeMap::const_iterator iter = heapToNode.find(heapThing);
        if (iter != heapToNode.end())
            return iter->second;
        HeapNode *node = new HeapNode(heapThing);
        heapToNode.insert(HeapToNodeMap::value_type(heapThing, node));
        remaining.insert(node);
        return node;
    }
};

template <typename T>
struct Tracer
{
    Node<T> *node;
    HeapGraph *graph;

    Tracer(Node<T> *node, HeapGraph *graph)
      : node(node), graph(graph)
    {}

    inline void operator () (const void *addr, HeapThing *ptr) {
        graph->addChild(node, ptr);
    }
};

int main(int argc, char **argv) {
    std::cout << "Whisper says hello." << std::endl;

    // Initialize static tables.
    InitializeSpew();
    InitializeTokenizer();

    // FIXME: re-enable
    //  Interp::InitializeOpcodeInfo();

    // Open input file.
    if (argc <= 1) {
        std::cerr << "No input file provided!" << std::endl;
        exit(1);
    }

    FileCodeSource inputFile(argv[1]);
    if (inputFile.hasError()) {
        std::cerr << "Could not open input file " << argv[1]
                  << " for reading." << std::endl;
        std::cerr << inputFile.error() << std::endl;
        exit(1);
    }
    Tokenizer tokenizer(inputFile);

    BumpAllocator allocator;
    STLBumpAllocator<uint8_t> wrappedAllocator(allocator);
    Parser parser(wrappedAllocator, tokenizer);

    FileNode *fileNode = parser.parseFile();
    if (!fileNode) {
        WH_ASSERT(parser.hasError());
        std::cerr << "Parse error: " << parser.error() << std::endl;
        return 1;
    }

    // Initialize a runtime.
    Runtime runtime;
    if (!runtime.initialize()) {
        WH_ASSERT(runtime.hasError());
        std::cerr << "Runtime error: " << runtime.error() << std::endl;
        return 1;
    }

    // Create a new thread context.
    const char *err = runtime.registerThread();
    if (err) {
        std::cerr << "ThreadContext error: " << err << std::endl;
        return 1;
    }
    ThreadContext *thrcx = runtime.threadContext();

    // Create a run context for execution.
    RunContext runcx(thrcx);
    RunActivationHelper _rah(runcx);

    RunContext *cx = &runcx;
    AllocationContext acx(cx->inTenured());

    // Write out the syntax tree in packed format.
    Local<AST::PackedWriter> packedWriter(cx,
        PackedWriter(
            STLBumpAllocator<uint32_t>(wrappedAllocator),
            tokenizer.sourceReader(),
            acx));
    packedWriter->writeNode(fileNode);

    ArrayHandle<uint32_t> buffer = packedWriter->buffer();
    ArrayHandle<VM::Box> constPool = packedWriter->constPool();

    Local<VM::PackedSyntaxTree *> packedSt(cx,
        VM::PackedSyntaxTree::Create(acx, buffer, constPool));
    fprintf(stderr, "packedSt local @%p\n", packedSt.stackThing());

    fprintf(stderr, "STACK SCAN!\n");
    // Scan the root set.
    HeapGraph graph;
    for (LocalBase *base = thrcx->locals();
         base != nullptr;
         base = base->next())
    {
        StackThing *stackThing = base->stackThing();
        StackNode *node = graph.addRoot(stackThing);
        Tracer<StackThing> tracer(node, &graph);
        GC::ScanStackThing<Tracer<StackThing>>(tracer, stackThing,
                                               nullptr, nullptr);
    }

    fprintf(stderr, "HEAP SCAN!\n");
    // Process remainings.
    while (!graph.remaining.empty()) {
        HeapNode *node = *graph.remaining.begin();
        Tracer<HeapThing> tracer(node, &graph);
        GC::ScanHeapThing<Tracer<HeapThing>>(tracer, node->ptr,
                                             nullptr, nullptr);
        graph.remaining.erase(node);
    }

    return 0;
}
