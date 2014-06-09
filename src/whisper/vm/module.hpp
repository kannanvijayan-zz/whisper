#ifndef WHISPER__VM__MODULE_HPP
#define WHISPER__VM__MODULE_HPP


#include "vm/core.hpp"
#include "vm/string.hpp"
#include "vm/array.hpp"
#include "vm/source_file.hpp"

#include <new>

namespace Whisper {
namespace VM {


//
// A Module contains all the information associated with a given
// module.  This includes a sorted array of symbols mapping to
// their respective definitions within the module, as well as
// the list of files which contain the contents of the module.
//
class Module
{
    friend class GC::TraceTraits<Module>;

  public:
    typedef Array<SourceFile *> SourceFileArray;

    class Entry
    {
      friend class GC::TraceTraits<Entry>;
      private:
        HeapField<String *> name_;
        HeapField<GC::AllocThing *> defn_;

      public:
        Entry(String *name) : name_(name), defn_(nullptr) {}

        String *name() const {
            return name_;
        }
    };
    typedef Array<Entry> BindingArray;

  private:
    HeapField<SourceFileArray *> sourceFiles_;
    HeapField<BindingArray *> bindings_;

  public:
    Module(SourceFileArray *sourceFiles, BindingArray *bindings)
      : sourceFiles_(sourceFiles),
        bindings_(bindings)
    {}

    SourceFileArray *sourceFiles() const {
        return sourceFiles_;
    }

    BindingArray *bindings() const {
        return bindings_;
    }
};


} // namespace VM
} // namespace Whisper


#include "vm/module_specializations.hpp"


#endif // WHISPER__VM__MODULE_HPP
