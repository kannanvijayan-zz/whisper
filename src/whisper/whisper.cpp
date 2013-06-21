
#include <iostream>
#include "common.hpp"
#include "allocators.hpp"
#include "parser/code_source.hpp"
#include "parser/tokenizer.hpp"

using namespace Whisper;

int main(int argc, char **argv) {
    std::cout << "Whisper says hello." << std::endl;

    // Open input file.
    if (argc <= 1) {
        std::cerr << "No input file provided!" << std::endl;
        exit(1);
    }

    FileCodeSource inputFile(argv[1]);
    if (!inputFile.initialize()) {
        std::cerr << "Could not open input file " << argv[1]
                  << " for reading." << std::endl;
        exit(1);
    }
    BumpAllocator allocator;
    STLBumpAllocator<uint8_t> wrappedAllocator(allocator);
    Tokenizer tokenizer(wrappedAllocator, inputFile);

    return 0;
}
