
#include "runtime_inlines.hpp"
#include "vm/source_file.hpp"

namespace Whisper {
namespace VM {


/* static */ Result<SourceFile *>
SourceFile::Create(AllocationContext acx, Handle<String *> path)
{
    return acx.create<SourceFile>(path);
}


} // namespace VM
} // namespace Whisper
