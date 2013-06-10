
#include <iostream>
#include "common.hpp"
#include "allocators.hpp"
#include "parser/code_source.hpp"

using namespace Whisper;

int main(int argc, char **argv) {
    std::allocator<int> alloc;
    WrapAllocator<int, std::allocator> walloc(alloc);
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

    SourceStream inputStream(inputFile);
    if (!inputStream.initialize()) {
        std::cerr << "Could not initialize input stream on " << argv[1]
                  << "." << std::endl;
        exit(1);
    }

    // Read data from input file.
    size_t extent = 4;
    size_t extLeft = extent;
    inputStream.mark();
    for (;;) {
        // Read next byte.
        int byte = inputStream.readByte();
        if (byte == SourceStream::Error) {
            std::cerr << "Error reading file." << std::endl;
            exit(1);
        }
        if (byte == SourceStream::End)
            break;
        std::cout << (char) byte;

        extLeft--;
        if (extLeft == 0) {
            size_t seekBack = extent / 2;
            std::cout << '|' << std::endl << "--- seekBack " << seekBack << std::endl;
            inputStream.rewindTo(inputStream.position() - seekBack);
            extent *= 2;
            extLeft = extent;
        }
    }
    std::cout << std::endl;

    return 0;
}
