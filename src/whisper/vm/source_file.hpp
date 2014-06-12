#ifndef WHISPER__VM__SOURCE_FILE_HPP
#define WHISPER__VM__SOURCE_FILE_HPP


#include "vm/core.hpp"
#include "vm/string.hpp"

#include <new>

namespace Whisper {
namespace VM {

//
// A SourceFile contains mappings from symbols to the location within the
// file that contains the symbol definition.
//
class SourceFile
{
    friend struct GC::TraceTraits<SourceFile>;

  private:
    HeapField<String *> path_;

  public:
    SourceFile(String *path) : path_(path) {}

    String *path() const {
        return path_;
    }
};


} // namespace VM
} // namespace Whisper

// Include array template specializations for describing arrays to
// the GC.
#include "vm/source_file_specializations.hpp"


#endif // WHISPER__VM__SOURCE_FILE_HPP
