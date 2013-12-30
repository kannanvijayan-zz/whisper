
#include <iostream>
#include "common.hpp"
#include "allocators.hpp"
#include "spew.hpp"
#include "runtime.hpp"
#include "gc.hpp"
#include "parser/code_source.hpp"
#include "parser/tokenizer.hpp"
#include "parser/syntax_tree_inlines.hpp"
#include "parser/parser.hpp"
#include "vm/array.hpp"
#include "vm/hash_table.hpp"
#include "vm/type.hpp"
#include "vm/module.hpp"

using namespace Whisper;

void PrintTokens(CodeSource &code, Tokenizer &tokenizer)
{
    // Read and print tokens.
    for (;;) {
        Token tok = tokenizer.readToken();
        char buf[20];
        if (tok.isLineTerminatorSequence() || tok.isWhitespace() ||
            tok.isEnd())
        {
            std::cerr << "Token " << tok.typeString() << std::endl;

        }
        else
        {
            int len = tok.length() < 19 ? tok.length()+1 : 20;
            snprintf(buf, len, "%s", tok.text(code));
            if (tok.length() >= 19) {
                buf[16] = '.';
                buf[17] = '.';
                buf[18] = '.';
            }

            std::cerr << "Token " << tok.typeString() << ":" << buf
                      << std::endl;
        }

        if (tok.isEnd())
            break;
    }
}

struct Printer {
    void operator ()(const char *s) {
        std::cerr << s;
    }
    void operator ()(const uint8_t *s, uint32_t len) {
        for (size_t i = 0; i < len; i++)
            std::cerr << static_cast<char>(s[i]);
    }
};

int main(int argc, char **argv) {
    std::cout << "Whisper says hello." << std::endl;

    InitializeSpew();
    // FIXME: re-enable
    //  Interp::InitializeOpcodeInfo();

    // Open input file.
    if (argc <= 1) {
        std::cerr << "No input file provided!" << std::endl;
        exit(1);
    }

    FileCodeSource inputFile(argv[1]);
    if (!inputFile.initialize()) {
        std::cerr << "Could not open input file " << argv[1]
                  << " for reading." << std::endl;
        std::cerr << inputFile.error() << std::endl;
        exit(1);
    }
    BumpAllocator allocator;
    STLBumpAllocator<uint8_t> wrappedAllocator(allocator);
    InitializeKeywordTable();
    InitializeQuickTokenTable();
    Tokenizer tokenizer(wrappedAllocator, inputFile);

    // PrintTokens(inputFile, tokenizer);

    Parser parser(tokenizer);

    FileNode *fileNode = parser.parseFile();
    if (!fileNode) {
        WH_ASSERT(parser.hasError());
        std::cerr << "Parse error: " << parser.error() << std::endl;
        return 1;
    }

    Printer pr;
    PrintNode(tokenizer.source(), fileNode, pr, 0);

    /*
    // Annotate the program.
    AST::SyntaxAnnotator annotator(wrappedAllocator, program, inputFile);
    if (!annotator.annotate()) {
        WH_ASSERT(annotator.hasError());
        std::cerr << "Syntax annotation failed: " << annotator.error()
                  << std::endl;
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

    // Generate bytecode.
    Interp::BytecodeGenerator bcgen(cx, wrappedAllocator, program, annotator,
                                    false);
    Root<VM::Bytecode *> bc(cx, bcgen.generateBytecode());
    if (bcgen.hasError()) {
        std::cerr << "Codgen error: " << bcgen.error() << "!" << std::endl;
        return 1;
    }
    WH_ASSERT(bc != nullptr);

    // Get constant pool tuple.
    Root<VM::Tuple *> constants(cx);
    if (!bcgen.constants(constants))
        return false;

    VM::Script::Config scriptCfg(false, VM::Script::TopLevel,
                                    bcgen.maxStackDepth());
    Root<VM::Script *> script(cx,
            cx->inHatchery().create<VM::Script>(bc, constants, scriptCfg));
    std::cerr << "Created script with max stack depth " <<
                 script->maxStackDepth() << std::endl;

    // Print memory contents.
    VM::SpewHeapThingSlab(cx->hatchery());

    // Print bytecode contents.
    VM::SpewBytecodeObject(bc);

    // Interpret the script.
    std::cerr << "Running script" << std::endl;
    bool interpResult = Interp::InterpretScript(cx, script);
    std::cerr << "Script result: " << interpResult << std::endl;

*/

    return 0;
}
